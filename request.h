#ifndef __REQUEST_H__
// New struct to handle the SFF scheduling due to the need of pre reading before putting into the buffer and prioritizing based on type + size
typedef struct {
    int conn_fd;
    char filename[256]; 
    int filesize;          // Size in bytes (-1 for dynamic)
    int is_static;         // 1 = static, 0 = dynamic
	char http_buf[8192];  // The HTTP request buffer because in SFF we need to read it first so this will be read latter when tryintg to handle the request.
} request_info_t;

void request_handle(int fd);
void request_handle_sff(request_info_t *req);

#endif // __REQUEST_H__
