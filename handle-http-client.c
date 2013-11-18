#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const int BUF_SIZE = 4096;

void HandleHTTPClient(char *web_root, int clntSock, int mdbSock){
  char buffer[BUF_SIZE];
  char first_line[BUF_SIZE];
  char initial_request[BUF_SIZE];
  FILE *pipe = fdopen(clntSock,"r+");
  if (pipe == NULL)
    printf("Error opening clntSock as a file\n");
  FILE *mdbpipe = fdopen(mdbSock,"r+");
  if (mdbpipe == NULL)
    printf("Error opening mdbSock as a file\n");
  const char *mdb_lookup_form =
  "<h1>mdb-lookup</h1>\n"
  "<p>\n"
  "<form method=GET action=/mdb-lookup>\n"
  "lookup: <input type=text name=key>\n"
  "<input type=submit>\n"
  "</form>\n"
  "<p>\n";

  if(fgets(buffer, BUF_SIZE - 1 , pipe) != NULL){
    
    strncpy(first_line, buffer, sizeof(first_line));

    // We make a copy of the request we were given so we can reference it later. We replace 
    // line breaks with null characters so it will print more nicely.
    strncpy(initial_request, buffer, sizeof(initial_request));
    int i = 0;
    for(i=0; i < strlen(initial_request); i++){
      if(initial_request[i] == '\n'){
	initial_request[i] = '\0';
      }
    }
  }

  while(fgets(buffer, BUF_SIZE - 1 , pipe) != NULL){
    if ( (strcmp(buffer, "\n") && strcmp(buffer, "\r\n")) == 0){


      // Here we break up the request line into a few useful strings.
      char *token_separators = "\t \r\n"; // tab, space, new line
      char *method = strtok(first_line, token_separators);
      char *requestURI = strtok(NULL, token_separators);
      char *httpVersion = strtok(NULL, token_separators);
     
      // Now we check that none of these pointers are NULL.
      if ( !( method && requestURI && httpVersion) ) {
	printf("\"%s\" 400 Bad Request\n", initial_request);
	fprintf(pipe, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n");
	fflush(pipe);
	close(clntSock);
	return;
      }

      // This server only supports GET requests, so we throw out anything else.
      if ( (strlen(method) < 3) || (strncmp("GET", method, 4) != 0) ) {
	printf("\"%s %s %s\" 501 Not Implemented\n", method, requestURI, httpVersion);
	fprintf(pipe, "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n");
	fflush(pipe);
	close(clntSock);
	return;
      }
      // We check that the request line is terminated by a valid HTTP version.
      if ((strlen(httpVersion) < 8) || ((strncmp("HTTP/1.0", httpVersion, 8) != 0) && (strncmp("HTTP/1.1", httpVersion, 8)) != 0) ) {
	printf("\"%s %s %s\" 400 Bad Request\n", method, requestURI, httpVersion);
	fprintf(pipe, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n");
	fflush(pipe);
	close(clntSock);
	return;
      }
      // Here we check that GET is requesting a file, beginning with a /, and that there are 
      // no references to parent directories for security reasons.
      if ((strncmp("/", requestURI, 1) != 0 ) || (strstr(requestURI, "..")) ) {
	printf("\"%s %s %s\" 400 Bad Request\n", method, requestURI, httpVersion);
	fprintf(pipe, "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n");
	fflush(pipe);
	close(clntSock);
	return;
      }
      // If the request has passed all of our tests, we should be ready to provide what the
      // client requested.

      // We first check if they have requested mdb-lookup.
      if ( (strncmp(requestURI, "/mdb-lookup", 12) == 0) 
	|| (strncmp(requestURI, "/mdb-lookup/", 12) == 0) ){
	
	// Looks like an initial mdb-lookup request. We send a header saying that's cool, 
	// and we log the event on the server.

	fprintf(pipe, "HTTP/1.0 200 OK\r\n");
	fprintf(pipe, "\r\n");
	printf("\"%s %s %s\" 200 OK\n", method, requestURI, httpVersion);

	// Next we send them the form to do mdb-lookup requests.
	fprintf(pipe, "<html><body>\n%s</html></body>", mdb_lookup_form);
	fflush(pipe);
	close(clntSock);
	return;
      }
      if ( strncmp(requestURI, "/mdb-lookup?key=", 16) == 0) {

	// Ok, looks like they're submitting an mdb-lookup query. Let's point to the 
	// actual query:

	char *mdb_query = strchr(requestURI, '=') + 1;
	
	// We now send a header saying that's cool, and log the event on the server.

	fprintf(pipe, "HTTP/1.0 200 OK\r\n");
	fprintf(pipe, "\r\n");
	
	printf(" -> looking up [%s] \"%s %s %s\" 200 OK\n", mdb_query, method, requestURI, httpVersion);

        // Now we send the query to mdb-lookup-server.
	fprintf(mdbpipe, "%s\n", mdb_query);

	// We are now ready to return the results to the client. We first send the beginning:

	fprintf(pipe, "<html><body>\n%s\n", mdb_lookup_form);
	fprintf(pipe, "<p><table border>\n");

	// Now we recieve one row at a time from mdb-lookup-server, and send it to the client.
        
	int row_number = 0;
	while( (fgets(buffer, BUF_SIZE - 1, mdbpipe) != NULL) 
	    && (strcmp(buffer, "\n") != 0 ) ){
	  if(row_number % 3 == 0){
	    fprintf(pipe, "<tr><td bgcolor=C4D8E2>  %s", buffer);
	  }
	  if(row_number % 3 == 1){
	    fprintf(pipe, "<tr><td>  %s", buffer);
	  }
	  if(row_number % 3 == 2){
	    fprintf(pipe, "<tr><td bgcolor=DAA520>  %s", buffer);
	  }
          row_number++;
        }

	// Now we send the end of the HTML document, flush the output pipe, and 
	// close the connection.

	fprintf(pipe, "</table>\n</body></html>");
	fflush(pipe);
	close(clntSock);
	return;
      }

      // At this point we know the user didn't request mdb-lookup services, so we next assume
      // they want to download a file.

      // We first concatenate the server and user file paths.
      char file_path[300];
      strcpy(file_path, web_root);
      strcat(file_path, requestURI);

      // Now we need to check if the file requested is a directory. If so, we append index.html.
      struct stat statinfo;
      stat(file_path, &statinfo);
      if (S_ISDIR(statinfo.st_mode)){
	strcat(file_path, "index.html");
      }
      FILE *reqFile;
      
      // Here we actually open the file, sending a 404 if we can't do that.
      if ((reqFile = fopen(file_path, "rb"))){
	printf("\"%s %s %s\" 200 OK\n", method, requestURI, httpVersion);
	fprintf(pipe, "HTTP/1.0 200 OK\r\n");
      }
      else{
	printf("\"%s %s %s\" 404 Not Found\n", method, requestURI, httpVersion);
	fprintf(pipe, "HTTP/1.0 404 Not Found\r\n\r\n<html><body>\r\n<h1>404 Not Found</h1>\r\n</body></html>\r\n\r\n");        
	fflush(pipe);
	close(clntSock);
	return;
      }
      
      // Here we add some headers.
      if(strstr(file_path, ".html")){
	fprintf(pipe, "Content-Type: text/html\r\n");
      }

      if(strstr(file_path, ".jpg")){
	fprintf(pipe, "Content-Type: image/jpg\r\n");
      }

      fseek(reqFile, 0, SEEK_END);
      long fileSize = ftell(reqFile);
      fseek(reqFile, 0, SEEK_SET);
      fprintf(pipe, "Content-Length: %ld\r\n", fileSize);

      fprintf(pipe, "\r\n");

      // This loop writes the requested file into the pipe.
      int bytes_read = 0;
      while((bytes_read = fread(buffer, 1, BUF_SIZE, reqFile))){
	if(bytes_read < 0){
	  printf("Error in file access\n");
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
