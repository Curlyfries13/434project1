#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "client.h"

#define FILENAMEMAX 24     /* Longest string to echo */

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    char *scriptFileName;            /* File name of script. */
    char fileNameBuffer[FILENAMEMAX+1];  /* Buffer for receiving file name string */
    int scriptFileNameStringLen;     /* Length of string to echo */
    int respStringLen;               /* Length of received response */
    char *machineName;				 /* Machine name of the client */
    int clientNumber;				 /* Number of the client */


    if ((argc < 3) || (argc > 4))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];           /* First arg: server IP address (dotted quad) */
    scriptFileName = argv[2];       /* Second arg: string to echo */


    if ((scriptFileNameStringLen = strlen(scriptFileName)) > FILENAMEMAX)  /* Check input length */
        DieWithError("Echo word too long");

    if (argc == 4)
        echoServPort = atoi(argv[3]);  /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServPort);     /* Server port */

    /* Send the string to the server */
    if (sendto(sock, scriptFileName, scriptFileNameStringLen, 0, (struct sockaddr *)
               &echoServAddr, sizeof(echoServAddr)) != scriptFileNameStringLen)
        DieWithError("sendto() sent a different number of bytes than expected");
  
    /* Recv a response */
    fromSize = sizeof(fromAddr);
    if ((respStringLen = recvfrom(sock, fileNameBuffer, FILENAMEMAX, 0,
         (struct sockaddr *) &fromAddr, &fromSize)) != scriptFileNameStringLen)
        DieWithError("recvfrom() failed");

    if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
    {
        fprintf(stderr,"Error: received a packet from unknown source.\n");
        exit(1);
    }

    /* null-terminate the received data */
    fileNameBuffer[respStringLen] = '\0';
    printf("Received: %s\n", fileNameBuffer);    /* Print the script file name arg */
    
    close(sock);
    exit(0);
}
