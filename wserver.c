#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>
#include <string.h>  
#include <sys/stat.h>

char default_root[] = ".";
// int buffer[MAXBUF]; // fd
int num_threads = 1; 
int buffer_size = 1;
char schedType[16] = "FIFO"; // default
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;     
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;       // sleep/wake up worker
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;   // Condition for the Master thread to be called
int occupied = 0; 
int nextin = 0; 
int nextout = 0; 
// Similar to Consumer / Producer problem in FIFO thread pool


// typedef struct {
//     int conn_fd;           
//     char filename[256];    // The requested URI/filename
//     int filesize;          // Size in bytes (-1 for dynamic)
//     int is_static;         // 1 = static, 0 = dynamic
// 	char http_buf[8192];  // The HTTP request buffer because in SFF we need to read it first so this will be read latter when tryintg to handle the request.
// } request_info_t;

request_info_t *buffer; // Buffer to store tasks

// Consumer
void* thread_function_fifo() {
	//Initialize mutex and condition variable and put them  to sleep inside the pool
	// Infinite loop because they are in a pool not going anywhere after doing one task
	while (1) {
		pthread_mutex_lock(&lock);
		// if empty buffer then wait until there's something to consume
		while (occupied == 0) {
			pthread_cond_wait(&cond, &lock);
		}
		// Get the connection fd and do it
		int conn_fd = buffer[nextout].conn_fd;
		nextout = (nextout + 1) % buffer_size;
		occupied--;
		pthread_cond_signal(&not_full); // Wake up Master signalling there's space in buffer (in the rare case that they are asleep)
		pthread_mutex_unlock(&lock);
		request_handle(conn_fd); 
		close_or_die(conn_fd);
	}
}
// SFF
void* thread_function_sff() {
	while (1) {
		pthread_mutex_lock(&lock);
		// if empty buffer then wait until there's something to consume
		while (occupied == 0) {
			pthread_cond_wait(&cond, &lock);
		}
		// int conn_fd = buffer[0].conn_fd; // Doesn't exist anymore because it's read in the main thread
		// Get the first request in the buffer because this one is guaratee to be smallest with static, dynamic priority (sorted before putting in buffer)
		request_info_t req = buffer[0];
		for (int i = 0; i < occupied - 1; i++) {
			buffer[i] = buffer[i + 1];
		}
		occupied--;
		pthread_cond_signal(&not_full); // Wake up Master signalling there's space in buffer (in the rare case that they are asleep)
		pthread_mutex_unlock(&lock);
		request_handle_sff(&req); // Handle the request differently because we already read the HTTP request so this one read the request via the copy of buffer in fd
		close_or_die(req.conn_fd);
	}
}

// Insert into index function for SFF to get fd in correct order
void insert_at(int index, request_info_t new_item) {
    // shift elements to the right
    for (int i = occupied; i > index; i--) {
        buffer[i] = buffer[i - 1];
    }
    buffer[index] = new_item;
    occupied++;
}
//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	// thread_pool = malloc(sizeof(pthread_t) * 10);

    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	case 't':
	    num_threads = atoi(optarg);
	    break;
	case 'b':
	    buffer_size = atoi(optarg);
	    break;
	// FIFO or SFF
	case 's':
	    strcpy(schedType, optarg);
	    break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]\n");
	    exit(1);
	}

	buffer = malloc(sizeof(request_info_t) * buffer_size);
	pthread_t *thread_pool;
	thread_pool = malloc(sizeof(pthread_t) * num_threads);
    // run out of this directory
    chdir_or_die(root_dir);
    
    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    
	// pthread_mutex_init(&lock, NULL);
    // pthread_cond_init(&cond, NULL);
	// create thread pool
	// FIFO
	if (strcmp(schedType, "FIFO") == 0) {
		// FIFO thread pool, just put in the order they come	
		for (int i = 0; i < num_threads; i++) {
			pthread_create(&thread_pool[i], NULL, thread_function_fifo, NULL);
		}
		while (1) {
			struct sockaddr_in client_addr;
			int client_len = sizeof(client_addr);
			int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
			// Producer
			pthread_mutex_lock(&lock);
			while (occupied == buffer_size) // buffer full so wait for the consumer to consume (master waits)
				pthread_cond_wait(&not_full, &lock);
			// add to buffer
			buffer[nextin].conn_fd = conn_fd;
			nextin = (nextin + 1) % buffer_size;
			occupied++;
			pthread_cond_signal(&cond); // signal the consumer
			pthread_mutex_unlock(&lock);
		}
	}
	else { // For SFF case
		for (int i = 0; i < num_threads; i++) {
			pthread_create(&thread_pool[i], NULL, thread_function_sff, NULL);
		}
		while (1) {
			// Read the HTTP for static or dynamic and filesize to put them in the correct order
			struct sockaddr_in client_addr;
			int client_len = sizeof(client_addr);
			int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
			// Read HTTP req, this will make http req empty 
			char buf[8192];
			readline_or_die(conn_fd, buf, sizeof(buf));
			// Parse the URI
			char method[256], uri[256], version[256];
			sscanf(buf, "%s %s %s", method, uri, version);
			// Filter:
			// static or dynamic
			int is_static = 1;
			if (strstr(uri, ".cgi")){ // It's dynamic if uri contains .cgi
				is_static = 0;
			}
			//Get the size of it, if dynamic then the size became -1
			int filesize = -1;
			char *filename = uri + 1; 
			// Get the filesize if static so you know where to put it in the buffer
			if (is_static) {
				struct stat file_stat; 				
				if (stat(filename, &file_stat) == 0) {
					filesize = file_stat.st_size;
				}
			}
			// Now we have weather the file is static or dynamic and able to put them in buffer appropriately
			// Filling out the struct to send into the buffer
			request_info_t toSend; 
			toSend.conn_fd = conn_fd;
			strcpy(toSend.filename, uri);
			strcpy(toSend.http_buf, buf); // The whole HTTP request so that when handling the request we can read from this instead of reading from conn_fd again because it's already read
			toSend.filesize = filesize;
			toSend.is_static = is_static;
			pthread_mutex_lock(&lock);
			while (occupied == buffer_size) // buffer full so wait for the consumer to consume (master waits)
				pthread_cond_wait(&not_full, &lock);
			if (is_static == 0) { // It's dynamic so just add it at the end of the buffer
				insert_at(occupied, toSend);
			}
			else{
				// Else go through the buffer and find the correct spot to insert (the smaller the size the higher the priority)
				int insertAt = occupied;
				for(int i = 0; i < occupied; i ++){
					if (toSend.filesize < buffer[i].filesize && buffer[i].is_static == 1) {
						insertAt = i;
						break;
					}
					else if (buffer[i].is_static == 0){
						insertAt = i;
						break;
					}
				}
				insert_at(insertAt, toSend);
				// printf("Inserted %s (size=%d, static=%d) at pos %d\n", uri, filesize, is_static, insertAt);
				// printf("Buffer: ");
				// for (int i = 0; i < occupied; i++) {
				// 	printf("[%s:%d] ", buffer[i].filename, buffer[i].filesize);
				// }
				// printf("\n\n");
			}
			pthread_cond_signal(&cond); // signal the consumer
			pthread_mutex_unlock(&lock);
		}
	}
    return 0;
}


    


 
