#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>
#include <netdb.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */

void HandleHTTPClient(char *web_root, int clntSock, int mdbSock);   /* TCP client handling function */

static void DieWithError(const char *message)
{
    perror(message);
    exit(1); 
}




int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int mdbSock;                     /* Socket descriptor for mdb-lookup-server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in ServAddr;     /* Local address */
    struct sockaddr_in ClntAddr;     /* Client address */
    struct sockaddr_in mdbAddr;      /* mdb-lookup-server address */
    unsigned short ServPort;         /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    char *mdbIP;                     /* mdb-lookup-server IP address */

    
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
    


    // We first want to establish a connection with the mdb-lookup-server.

    
    // Get mdb IP from server name
    struct hostent *he;
    if ((he = gethostbyname(mdbHostname)) == NULL) {
      DieWithError("gethostbyname failed");
    }
    mdbIP = inet_ntoa(*(struct in_addr *)he->h_addr);
//    free(he);

    /* Create socket for mdb-lookup-server connection */
    if ((mdbSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct mdb-lookup-server address structure */
    memset(&mdbAddr, 0, sizeof(mdbAddr));   /* Zero out structure */
    mdbAddr.sin_family = AF_INET;           /* Internet address family */
    inet_aton(mdbIP, &(mdbAddr.sin_addr));  /* mdb-lookup-server IP address */
    mdbAddr.sin_port = htons(mdbPort);      /* mdb-lookup-server port */

    /* Establish the connection to mdb-lookup-server */
    if (connect(mdbSock, (struct sockaddr *) &mdbAddr, sizeof(mdbAddr)) < 0)
      DieWithError("connect() failed");



    // Now we are ready to establish a socket connection to listen for clients.



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

      HandleHTTPClient(webRoot, clntSock, mdbSock);
    }
    /* NOT REACHED */
    return 0;
}
