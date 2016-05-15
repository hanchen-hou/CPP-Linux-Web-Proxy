#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>      // Needed for the socket functions

#include <sys/stat.h>   // create dir
#include <sys/types.h>  // dir flag

#include <multithread_server.h>

#define THREAD_NUM 10
#define MAX_FILE_NAME_SIZE 255
#define BLOCK_LIST_FILE "block_list.txt"

// Create a new socket and bind to target port.
// Listen the target port and accept connections in multi-threads
int socket_listener(int port_num, int backlog) {

  // Socket 
  int socket_desc;

  //AF_INET -> use Ipv4
  // SOCK_STREAM -> socket type
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
    return 1;
  }
    
  // Bind
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY; // inet_addr("0.0.0.0")
  server.sin_port = htons(port_num);
  
  if ( bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
    perror("bind failed\n");
    return 1;
  }
    
  // Listen
  listen(socket_desc, backlog);        
  printf("waiting for incoming connection\n");
    
  // Create threads
  pthread_t tid[THREAD_NUM];
  int err;
  for (int i = 0; i < THREAD_NUM; i++) {
    err = pthread_create(&tid[i], NULL, &connection_handler, (void*) &socket_desc);
  }

  // Wait threads to be done
  for (int i = 0; i < THREAD_NUM; i++) {  
    pthread_join(tid[i], NULL);
  }
}

// Call by thread
// Accept from socket listener
// Note: the whole request & response procedure are executed in this method
void* connection_handler(void* socket_desc) {
  // Accept
  int client_sock, c;
  struct sockaddr_in client;
  while ((client_sock = accept( *(int*) socket_desc, (struct sockaddr *) &client, (socklen_t*) &c))) {
    
    struct Request* req = new_request();
    struct Response* resp = new_response();

    // Get Request From Client
    get_request_from_client(client_sock, req);

    // Check block list
    if(!is_blocked(client_sock, req)){

      // Check cache
      if(!has_cache(client_sock, req)){
	  // Connect To Server
	  int socket_to_server = connect_to_server(req);

	  // Send Request To Server 
	  send_request_to_server(socket_to_server, req);
    
	  // Get Response From Server
	  fetch_server_data(socket_to_server, resp);

	  // Save cache
	  save_cache(req, resp);
	  
	  // Send Response To Client
	  send_reponse_to_client(client_sock, resp);

	  close(socket_to_server);
	}
    }
    close(client_sock);

    free_request(req);
    free_response(resp);
    free(req);
    free(resp);
  }
}

void get_request_from_client(int client_sock ,struct Request* req){
  char buf[10240] = {0};
  int recv_size = 0;
  recv_size = recv(client_sock, buf, sizeof buf, 0);
  if(recv_size == -1){
    perror("accept\n");
  }
  req->data_size = recv_size + 1; //do not remove "+1", may be the extra '\0' is a signal?
  req->data = malloc(req->data_size);
  memset(req->data, '\0', req->data_size);
  memcpy(req->data, buf, recv_size);
  //printf("%s\n", req->data);

  //parse request
  parse_request(req);
}

void parse_request(struct Request* req){
  char* strptr = req->data;

  //request type
  for(int i = 0; *strptr != ' ' && i < sizeof req->type; i++){
    req->type[i] = *strptr;
    strptr++;
  }
  strptr++;
  //printf("%s\n", req->type);
  
  //absoluteURI
  for(int i = 0; *strptr!=' ' && i < sizeof(req->absoluteURI); i++){
    req->absoluteURI[i] = *strptr;
    strptr++;
  }
  //printf("%s\n", req->absoluteURI);

  strptr = req->absoluteURI;
  //host name
  char temp_c[10] = {0};
  strncpy(temp_c, strptr, 8);
  if(strstr(temp_c, "http://") || strstr(temp_c, "HTTP://")){
    strptr+=7; // skip "http://"
    strcpy(req->port, "80"); // HTTP uses 80 port 
  }else if(strstr(temp_c, "https://") || strstr(temp_c, "HTTPS://")){
    strptr+=8; //skip "https://"
    //strcpy(req->port, "443"); // HTTPS uses 443 port
  }
  for(int i = 0; *strptr!='/' && *strptr!=':' && *strptr!=' ' && i < sizeof(req->absoluteURI); i++){
    req->host[i] = *strptr;
    strptr++;
  }
  //printf("%s\n", req->host);

  //port number
  if(*strptr == ':'){
    strptr++;
    for(int i = 0; *strptr != '/' && *strptr!=' '; i++){
      req->port[i] = *strptr;
      //req->port[i+1] = '\0';
      strptr++;
    }
  }else{
    strcpy(req->port, "80");
  }
  //printf("%s\n", req->port);
}

// If find the address in the blocklist, return 404
int is_blocked(int client_sock, struct Request* req){
  FILE* fp;
  char* line = NULL;
  size_t len = 0;

  int is_blocked = 0;
  
  fp = fopen(BLOCK_LIST_FILE, "r");
  if(fp == NULL) perror("cannot open block list.\n");

  // read each line
  while (getline(&line, &len, fp) != EOF) {
    char keyword[100];
    memset(keyword, '\0', sizeof(keyword)); // init 'keyword[100]'

    //copy one 'line' to 'keyword' except \n(line feed) and \r(return)
    for(int i = 0; line[i] != '\0' && line[i] != '\n' && line[i] != '\r'; i++){
      keyword[i] = line[i];
    }

    // search 'keyword' in the 'req->host'
    if(strlen(keyword)>0 && strstr(req->host, keyword) != NULL){
      printf("block -> %s\n", keyword); // print log
      char not_found_response[] = "HTTP/1.1 404 Not Found\n\n";
      //find it
      write(client_sock, not_found_response, sizeof(not_found_response));
      is_blocked = 1;
      break;
    }
  }
  fclose(fp);
  free(line);
  return is_blocked;
}

int has_cache(int client_sock, struct Request* req){
  // URL chars can be these
  // A-Z a-z 0-9 ._\/~%-+&#?!=()@:

  printf("check cache\n");
  int i = 0;
  char* strptr = req->absoluteURI;
  if(strstr(strptr, "http://") || strstr(strptr, "HTTP://")){
    i+=7; // skip "http://"
  }else if(strstr(strptr, "https://") || strstr(strptr, "HTTPS://")){
    i+=8; // skip "https://"
  }
  
  for(int j = 0; j < sizeof(req->cache_file) && i < strlen(req->absoluteURI); j++){
    if(req->absoluteURI[i] == '/') {
      mkdir(req->cache_file, 0777);
    }
    req->cache_file[j] = req->absoluteURI[i];
    i++;
  }

  // e.g. URI = www.example.com/test/
  // website return www.example.com/test/index.html automatically
  int cache_strlen = strlen(req->cache_file);
  if(req->cache_file[cache_strlen - 1] == '/'){
    strcpy(req->cache_file + cache_strlen, "index\0");
  }
  printf("%s\n", req->cache_file);
  
  if(access(req->cache_file, R_OK) == -1){
    return 0; // cache file does not exit
  }else{
    FILE* fp = fopen(req->cache_file, "r");
    if(fp == NULL) perror("cannot write cache.\n");
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp); //go back to the beginning

    char* cache = malloc(fsize + 1);
    fread(cache, fsize, 1, fp);
    fclose(fp);

    printf("send cache\n");
    write(client_sock, cache, fsize + 1);
    return 1;
  }
}

int connect_to_server(struct Request* req){
  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
 
  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." 
  // Init 'host_info', set all bytes to 0
  memset(&host_info, 0, sizeof(host_info));

  // ai_family: the type of IP version we want the server to return
  // IP version not specified. Can be both. (AF_INET -> IPv4 & AF_INET6 -> IPv6)
  host_info.ai_family = AF_UNSPEC;
  
  // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
  host_info.ai_socktype = SOCK_STREAM; 
  
  // Get address info
  status = getaddrinfo(req->host, req->port, &host_info, &host_info_list);
  // getaddrinfo returns 0 on succes, or some other value when an error occured.
  // (translated into human readable text by the gai_strerror function).
  if(status != 0){
    printf("Cannot get target address info. Error:%s\n", gai_strerror(status));
    exit(1);
  }

  // open a new socket
  int socket_to_server = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socket_to_server == -1) {
    perror("socket error\n");
  }

  // connect to server
  status = connect(socket_to_server, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    perror("connect error\n");
  }
  
  return socket_to_server;
}

void send_request_to_server(int socket_to_server, struct Request* req){ 
  write(socket_to_server, req->data, req->data_size);
}

void fetch_server_data(int socket_to_server, struct Response* resp){
  char recv_data_buf[800*1024]; // 800 kb 
  memset(recv_data_buf, '\0',sizeof(recv_data_buf));
  int recv_size = 0;
  
  do{
    recv_size = recv(socket_to_server, recv_data_buf, sizeof(recv_data_buf), MSG_WAITALL);

    if(resp->data == NULL){
      resp->data_size = recv_size + 1;
      resp->data = malloc(resp->data_size);
      memset(resp->data,'\0',resp->data_size);
      memcpy(resp->data, recv_data_buf, recv_size);
    }else{
      //expand
      resp->data = realloc(resp->data, resp->data_size+recv_size);
      memset(resp->data+resp->data_size, '\0', recv_size);
      memcpy(resp->data+resp->data_size, recv_data_buf, recv_size);
      resp->data_size += recv_size;
    }
  } while(recv_size > 0);

  char* c_ptr = resp->data;
  for(int i = 0; i < resp->data_size; i++){
    if(c_ptr[0] == '\r' && c_ptr[1] == '\n' && c_ptr[2] == '\r' && c_ptr[3] == '\n'){
      break;
    }
    resp->header_size++;
    c_ptr++;
  }

  resp->header_size++; //for the last extra 0 
  resp->header = malloc(resp->header_size);
  memset(resp->header, '\0', resp->header_size); //the response header is a char array, leave the last byte to be \0
  memcpy(resp->header, resp->data, resp->header_size - 1); //keep the last byte be \0, need "-1"
}

void save_cache(struct Request* req, struct Response* resp){
  FILE* fp = fopen(req->cache_file, "w");
  if(fp == NULL) perror("cannot open cache.\n");

  fwrite(resp->data, resp->data_size, 1, fp);
  fclose(fp);
}

void send_reponse_to_client(int client_sock, struct Response* resp){
  //printf("\n*** send_size=%d ***\n", resp->data_size);
  //printf("%s\n", resp->header);
  write(client_sock, resp->data, resp->data_size);
}

