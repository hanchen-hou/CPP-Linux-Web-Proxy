#include <request_response.h>

int socket_listener(int, int);
void* connection_handler(void *);
void get_request_from_client(int client_sock ,struct Request* req);
int is_blocked(int client_sock, struct Request* req);
int has_cache(int client_sock, struct Request* req);
int connect_to_server(struct Request* req);
void parse_request(struct Request* req);
void send_request_to_server(int socket_to_server, struct Request* req);
void fetch_server_data(int socket_to_server, struct Response* resp);
void save_cache(struct Request* req, struct Response* resp);
void send_reponse_to_client(int client_sock, struct Response* resp);
