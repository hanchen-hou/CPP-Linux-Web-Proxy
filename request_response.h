struct Request{
  char type[8];
  char absoluteURI[1024];
  char host[256];
  char port[8];

  char cache_file[1024];
  
  int data_size;
  char* data; //8KB
};

struct Response{
  int header_size;
  char* header;
  int data_size;
  char* data; //5000kb
};

struct Request* new_request();
struct Response* new_response();
void free_request(struct Request* req);
void free_response(struct Response* resp);
