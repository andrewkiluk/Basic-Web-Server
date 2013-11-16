#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>

const int BUF_SIZE = 4096;

void HandleHTTPClient(char *web_root, char *mdb_lookup_host, int mdb_lookup_port, int clntSock){
  char buffer[BUF_SIZE];
  FILE *pipe = fdopen(clntSock,"r+");
  while(fgets(buffer,BUF_SIZE- 1 ,pipe) != NULL){
    
    char *protocol;
        // A valid HTTP request should have the protocol here in the following position:
    if(strchr(buffer, '\r')){
      protocol = strrchr(buffer, '\r') - 8; 
    }
    else{
      protocol = strrchr(buffer, '\n') - 8;
    }
    
    char *file_request = strchr(buffer, '/'); // The requested filename should be here.

    // This server only supports GET requests, so we throw out anything else.
    if (strncmp("GET ", buffer, 4) != 0 ) {
      fprintf(pipe, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n\r\n");
      fflush(pipe);
      close(clntSock);
      return;
    }

    // We check that the request line is terminated by a valid HTTP protocol name.
    if ((strlen(protocol) < 8) || ((strncmp("HTTP/1.0", protocol, 8) != 0) && (strncmp("HTTP/1.1", protocol, 8)) != 0) ) {
      fprintf(pipe, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n\r\n");
      fflush(pipe);
      close(clntSock);
      return;
    }
    // Here we check that GET is requesting a file, beginning with a /, and that there are 
    // no references to parent directories for security reasons.
    if ((strncmp("GET /", buffer, 5) != 0 ) || (strstr(buffer, "..")) ) {
      fprintf(pipe, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n\r\n");
      fflush(pipe);
      close(clntSock);
      return;
    }
    // If the request has passed all of our tests, we should be ready to provide the file to
    // the client. 
    else{
      // We 
      buffer[protocol - buffer - 1] = '\0';
      printf("%s\n\n\n\n\n\n", file_request + 1);
      char file_path[300];
      strcpy(file_path, web_root);
      strcat(file_path, file_request);
      FILE *reqFile;
      if ((reqFile = fopen(file_path, "rb"))){
        fprintf(pipe, "HTTP/1.0 200 OK\r\n\r\n");
      }
      else{
        fprintf(pipe, "HTTP/1.0 404 Not Found\r\n\r\n<html><body>\r\n<h1>404 Not Found</h1>\r\n</body></html>\r\n\r\n");        
	fflush(pipe);
        close(clntSock);
        return;
      }
      
      // This loop writes the requested file into the pipe.
      int bytes_read = 0;
      while((bytes_read = fread(buffer, 1, BUF_SIZE, reqFile))){
	if(bytes_read < 0){
          fprintf(pipe, "HTTP/1.0 500 Internal Server Error\r\n\r\n<html><body>\r\n<h1>500 Internal Server Error</h1>\r\n</body></html>\r\n\r\n");
          fflush(pipe);
          close(clntSock);
          return;
        }
	if(fwrite(buffer, 1, bytes_read, pipe) < 0){
	  break;
	}
      }
      
      fflush(pipe);
      close(clntSock);
      return;
    }
  }
}















