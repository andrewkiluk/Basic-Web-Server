#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>
#include <netdb.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */

void HandleHTTPClient(char *web_root, int clntSock, FILE *mdbpipe);   /* TCP client handling function */

static void DieWithError(const char *message)
{
    perror(message);
    exit(1); 
}


int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in ServAddr;     /* Local address */
    struct sockaddr_in ClntAddr;     /* Client address */
    unsigned short ServPort;         /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */


    
    // ignore SIGPIPE so that we don’t terminate when we call
    // send() on a disconnected socket.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
      DieWithError("signal() failed");


    if (argc != 5)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <server-port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }
    
    ServPort = atoi(argv[1]);  /* First arg:  local port */
    char *webRoot = argv[2];
    char *mdbHostname = argv[3];
    unsigned short mdbPort = atoi(argv[4]);
    

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(ServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
      /* Set the size of the in-out parameter */
      clntLen = sizeof(ClntAddr);

      /* Wait for a client to connect */
      if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, 
                             &clntLen)) < 0){
        fprintf(stderr,"failed accept() attempt\n");
        break;
      }

      /* clntSock is connected to a client! */

      printf("%s ", inet_ntoa(ClntAddr.sin_addr));

      HandleHTTPClient(webRoot, clntSock, mdbpipe);
    }
    /* NOT REACHED */
    return 0;
}
