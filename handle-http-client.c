#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>


void HandleHTTPClient(char *web_root, char *mdb_lookup_host, int mdb_lookup_port, int clntSock){
  char buffer[4000];
  FILE *pipe = fdopen(clntSock,"r+");
  while(fgets(buffer,3999,pipe) != NULL){
    fprintf(stderr, "go\n");
  }
}
