# Concurrent Web Server Project

This project implements a **concurrent web server** capable of handling multiple HTTP requests simultaneously. A basic **single-threaded web server** is provided, and the goal is to extend it to a **multi-threaded server** with different request scheduling policies.

---

## Project Overview

- Make a single-threaded web server **multi-threaded**.  
- Handle multiple requests concurrently using **thread pools**.  
- Implement **producer-consumer synchronization** with **mutexes** and **condition variables**.  
- Implement **request scheduling policies** for worker threads.

---

## HTTP Background

- The server communicates with clients using **HTTP 1.0**.  
- Supports **static content** (read a file) and **dynamic content** (execute CGI programs).  
- Requests include a URL pointing to a file or program, possibly with arguments:  

http://localhost:8000/test.cgi?x=10&y=20

- Server listens on a specified **port**; default ports below 1024 are reserved.  

---

## Multi-threaded Server

- **Master thread**: Accepts new connections and places them into a **fixed-size buffer**.  
- **Worker threads**: Take requests from the buffer and handle static or dynamic content.  
- Threads synchronize via **mutexes** and **condition variables**.  
- Supports a **thread pool** to avoid overhead of creating threads per request.  
- Maintains **producer-consumer relationship** between master and worker threads.  

---

## Scheduling Policies

- **FIFO**: Handle requests in the order they arrive in the buffer.  
- **Smallest File First (SFF)**: Handle the smallest static file first; static files take priority over dynamic content.  
- The scheduling policy is determined at runtime via a command-line argument.

---

## Command-Line Parameters

```bash
./wserver [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]

basedir – Root directory for the server (default: current directory).
port – Port number to listen on (default: 10000).
threads – Number of worker threads (default: 1).
buffers – Maximum number of pending connections (default: 1).
schedalg – Scheduling algorithm (FIFO or SFF, default: FIFO).

./wserver -d . -p 8003 -t 8 -b 16 -s SFF
```

## Source Code Overview
- wserver.c – Main server and connection loop.
- request.c – Handles HTTP requests.
- io_helper.h/c – System call wrappers (_or_die functions).
- wclient.c – Test client for sending multiple requests.
- spin.c – Example CGI program for testing long-running requests.
- Makefile – Build rules for server, client, and CGI programs.

## Testing

1. Concurrent Requests
  Use curl to send multiple requests at once:
  curl -0 --parallel localhost:10000/spin.cgi?30 localhost:10000/favorite.html
  The server should handle up to thread pool size requests simultaneously.

2. Producer-Consumer Blocking
  Test buffer limits by sending more requests than the buffer size; the master thread should block if full until a worker thread consumes a request.

3. Scheduling Policies
  Verify FIFO or SFF behavior by sending multiple requests of varying sizes and content types.

## Learning Outcomes
- Implement a multi-threaded server with POSIX threads.
- Synchronize threads using mutexes and condition variables.
- Apply producer-consumer principles in a practical system.
- Implement basic request scheduling policies.
- Understand server concurrency, buffering, and request handling.
