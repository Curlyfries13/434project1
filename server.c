#include <stdio.h>      /* for printf() and fprintf() and file access*/
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "server.h"

#define ECHOMAX 225     /* Longest string to echo */

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
}

int findClient(char clientName[]);

bool addClient(char clientName[]);

bool dropCilent(char clientname[]);

int findFileStatus(char clientName[], char fileName[]);

bool writeLock(char clientName[], char fileName[]);

bool readLock(char clientName[], char fileName[]);

bool writeUnlock(char clientName[], char fileName[]);

bool readUnlock(char clientName[], char fileName[]);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    struct request structBuffer;        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
	client* clients;

    if (argc != 2)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");
  
    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, &structBuffer, sizeof(struct request), 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        printf("Client IP: %s \n", structBuffer.client_ip);
        printf("Machine Name: %s \n", structBuffer.m);
        printf("Client Number: %i \n", structBuffer.c);
        printf("Request Number: %i \n", structBuffer.r);
        printf("Incarnation Number: %i \n", structBuffer.i);
        printf("Operation: %s\n\n", structBuffer.operation);


        /* Send received datagram back to the client */
        if (sendto(sock, &structBuffer, recvMsgSize, 0,
             (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize)
            DieWithError("sendto() sent a different number of bytes than expected");
    }
    /* NOT REACHED */
}
