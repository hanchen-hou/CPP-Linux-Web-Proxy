#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <multithread_server.h>

int main(int argc, char* argv[]) {
  
  int port_num = 8000;
  if(argc >1){
    port_num = atoi(argv[1]);
  }
  int backlog = 10;
  socket_listener(port_num, backlog);
  return 0;
}
