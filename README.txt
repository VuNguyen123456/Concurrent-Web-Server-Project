Vu Nguyen, G01390056

Files Modified or Added:

wserver.c
-Implemented concurrency handling using multiple worker threads and a bounded buffer.
Added support for SFF (Shortest File First) scheduling policy in addition to FIFO.

request.h
-Added a new request_info_t struct used for passing parsed request data between wserver.c and request.c.

request.c
-Modified to include a new request handler function that processes requests from a request_info_t struct instead of reading directly from the file descriptor.

Makefile
Modified the wserver build rule to include the -pthread flag for proper thread support

blocking_client.c
-Added a simple blocking client program used for testing concurrency behavior (connects to the server and does not send any requests).

favorite.html, small.html, large.html
-Added HTML files for testing static file serving and scheduling order.