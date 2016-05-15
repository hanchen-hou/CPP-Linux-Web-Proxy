#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <multithread_server.h>

struct Request* new_request(){
  struct Request* req = malloc(sizeof(struct Request));
  //req->type = NULL;
  //req->filename = NULL;
  //req->host = NULL;
  //req->port = NULL;
  memset(req->type, '\0', sizeof(req->type));
  memset(req->absoluteURI, '\0', sizeof(req->absoluteURI));
  memset(req->host, '\0', sizeof(req->host));
  memset(req->port, '\0', sizeof(req->port));
  memset(req->cache_file, '\0', sizeof(req->cache_file));
  req->data_size = 0;
  req->data = NULL;
  return req;
}

struct Response* new_response(){
  struct Response* resp = malloc(sizeof(struct Response));
  resp->header_size = 0;
  resp->header = NULL;
  resp->data_size = 0;
  resp->data = NULL;
  return resp;
}

void free_request(struct Request* req){
  /*free(req->request_type);
  req->request_type = NULL;
  
  free(req->filename);
  req->filename = NULL;
  
  free(req->host);
  req->host = NULL;
  
  free(req->port);
  req->port = NULL;*/
  if(req->data != NULL){
    free(req->data);
    req->data = NULL;
  }
}

void free_response(struct Response* resp){
  if(resp != NULL){
    free(resp->data);
    resp->data = NULL;
  }
}
