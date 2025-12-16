// blocking_client.c: A blocking HTTP client for testing concurrency
// 
// To run, try: 
//      blocking_client hostname portnumber
//
// Connects to the server but does NOT send any HTTP request.
// This is used to test if the server can handle concurrent connections.

#include "io_helper.h"
#include <unistd.h>

#define MAXBUF (8192)

int main(int argc, char *argv[]) {
    char *host;
    int port;
    int clientfd;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }
    
    host = argv[1];
    port = atoi(argv[2]);
    
    /* Open a connection to the specified host and port */
    clientfd = open_client_fd_or_die(host, port);
    
    printf("Connected to %s:%d (fd=%d)\n", host, port, clientfd);
    printf("Blocking indefinitely... Press Ctrl+C to exit.\n");
    
    /* Keep the connection open but don't send anything */
    while (1) {
        sleep(60); // Sleep forever, keeping the connection alive
    }
    
    // Never reached, but good practice
    close_or_die(clientfd);
    exit(0);
}