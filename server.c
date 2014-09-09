#include <stdio.h>      /* for printf() and fprintf() and file access*/
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <stdbool.h>    /* for boolean types */
#include "server.h"

#define ECHOMAX 225     /* Longest string to echo */

static client* clientHead;
static int clientOffset;
static int clientTableSize;

//forward declarations
int findClient(char clientName[]);

bool addClient(request structBuffer);

bool dropCilent(char clientName[]);

int findFileStatus(char clientName[], char fileName[]);

bool writeLock(char clientName[], char fileName[]);

bool readLock(char clientName[], char fileName[]);

bool writeUnlock(char clientName[], char fileName[]);

bool readUnlock(char clientName[], char fileName[]);

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    struct request structBuffer;        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
	clientHead = NULL;
	clientOffset = 0;

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

		clientOffset = findClient(structBuffer.m);
		if (clientHead == NULL || clientOffset == -1){
			clientOffset = addClient(structBuffer);
		}
		
		
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        printf("Client IP: %s \n", structBuffer.client_ip);
        printf("Machine Name: %s \n", structBuffer.m);
        printf("Client Number: %i \n", structBuffer.c);
        printf("Request Number: %i \n", structBuffer.r);
        printf("Incarnation Number: %i \n", structBuffer.i);
        printf("Operation: %s\n\n", structBuffer.operation);

		if(findClient(structBuffer.m) == -1)
			printf("Adding Machine... %s", addClient(structBuffer));
		printf("Client Table size: %i", clientTableSize);
		
		
        /* Send received datagram back to the client */
        if (sendto(sock, &structBuffer, recvMsgSize, 0,
             (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize)
            DieWithError("sendto() sent a different number of bytes than expected");
    }
    /* NOT REACHED */
}

int findClient(char clientName[]){
	client* searchPointer = clientHead;
	int pointerOffset = 0;
	while(strcmp(clientName, (searchPointer->name)) != 0 && (pointerOffset <= clientTableSize)){
		searchPointer++;
		pointerOffset++;
	}
	//check if client is found, if not return -1
	if(pointerOffset > clientTableSize)
		return -1;
	else
		return pointerOffset;
}

bool addClient(request structBuffer){
	client* newClient = (client*) malloc(sizeof(client));
	//newClient->ip = structBuffer.client_ip;
	strcpy(newClient->ip,structBuffer.client_ip);
	//newClient->name = structBuffer.m;
	strcpy(newClient->ip,structBuffer.m);
	newClient->incarnation = structBuffer.i;
	newClient->request = structBuffer.r;
	newClient->files = NULL;
	newClient->next = clientHead;
	clientHead = newClient;
	clientTableSize++;
	return true;
}

bool dropClient(char clientName[]){
	int pointerOffset = findClient(clientName);
	client* target;
	//check to see if client exists
	if (pointerOffset == -1)
		return false;
	//the client is at the head of the list
	else if(pointerOffset == 0){
		target = clientHead;
		clientHead = clientHead -> next;
		free(target);
	}
	else{
		target = (clientHead + pointerOffset);
		(target-1)->next = target->next;
		free(target);
	}
	
	return true;
}

//if file has no locks on it, then return its pointer offset
int findFileStatus(char clientName[],  char fileName[]){
	client* currentClient = (clientHead + findClient(clientName));
	file* currentFiles = currentClient->files;
	int fileOffset = 0;

	while((currentFiles + fileOffset) != NULL && strcmp(fileName, (currentFiles + fileOffset)->fileName) != 0){
		fileOffset++;
	}

	//return various codes for files that are not accessable
	if((currentFiles + fileOffset) == NULL){
		return -1;
	}
	else if((currentFiles + fileOffset)->writeLock){
		return -3;
	}
	else if((currentFiles + fileOffset)->readLock){
		return -2;
	}
	else{
		return fileOffset;
	}
}
