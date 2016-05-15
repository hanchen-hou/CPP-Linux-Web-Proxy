# CPP-Linux-Web-Proxy
This is a web proxcy written in CPP and uses linux socket. 

Extra features: blocklist and cache.

*** How to contribute ***

terminal->make


*** How to use ***

Proxy address: 127.0.0.1:8000 ( can be changed in proxy.c file)

Blocklist: Add URL into block_list.txt, one address each line.

Cache: The cache will be stored automatically in the same path as the execute file. Each cache folder name is the URL name.
If you want to clean the cache, just remove the cache folder.


*** Files ***

- proxy.c: 
  - main() method
  - port number and backlog number

- request_response.h: 
  - Request Structure & Response Structure
  
- request_response.c:
  - create and free Request and Response Structure
  
- multithread_server.c and multithread_server.h
  - procedure:
    - open socket
    - bind to port
    - listen the port
    - accept a connection from client
    - get the request and parse it (i.e. get what the request wants)
    - check blocklist and cache
    - connect to server and send the request to server
    - obtain the response from server
    - store the response as cache
    - send the response back to client
    - close server and clinet socket
