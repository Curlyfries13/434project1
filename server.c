#include <stdio.h>      /* for printf() and fprintf() and file access*/
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <stdbool.h>    /* for boolean types */
#include "server.h"

#define ECHOMAX 225     /* Longest string to echo */

static struct client* clientHead;
static int clientOffset;
static int clientTableSize;

//forward declarations
int findClient(char clientName[]);

struct client* addClient(struct request structBuffer);

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
/*
		clientOffset = findClient(structBuffer.m);

		//check to see if client exists; if not add them.
		if (clientHead == NULL || clientOffset == -1){
			addClient(structBuffer);
		}
*/
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        printf("Client IP: %s \n", structBuffer.client_ip);
        printf("Machine Name: %s \n", structBuffer.m);
        printf("Client Number: %i \n", structBuffer.c);
        printf("Request Number: %i \n", structBuffer.r);
        printf("Incarnation Number: %i \n", structBuffer.i);
        printf("Operation: %s\n\n", structBuffer.operation);
/*
		if(findClient(structBuffer.m) == -1) {
			printf("Adding Machine... %s", addClient(structBuffer)->name);
		}
		printf("Client Table size: %i", clientTableSize);
*/

        /* Send a struct back to the client with requested information */

        //Get the first word of the operation from the received struct. This will help us determine what type of struct to send back to the client.
		size_t operationLength = strlen(structBuffer.operation);
		char localstr[operationLength+1];
		char * instruction;
		strcpy(localstr, structBuffer.operation);
		instruction = strtok(localstr, " ,");

		printf("The instruction in the operation received is: %s\n", instruction);
		//Decide which operation to perform and the type of struct to send back to the client
		//based on the request.
		/********ALL STRUCTS ARE CURRENTLY RETURNING TEMPORARY FAKE DATA SINCE FUNCTIONS DON'T EXIST TO SEND REAL DATA BACK TO CLIENT***********/
		if (strcmp(instruction, "open") == 0) {
			struct responseOpen open;

			//Make a call to the open function


			//Save the returned data to the struct
			open.fileDescriptor = 2;

			//Send the struct back to the client
			if (sendto(sock, &open, sizeof(struct responseOpen), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseOpen)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "close") == 0) {
			struct responseClose close;

			//Make a call to the close function


			//Save the returned data to the struct
			close.fileDescriptor = 1;

			//Send the struct back to the client
			if (sendto(sock, &close, sizeof(struct responseClose), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseClose)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "read") == 0) {
			struct responseRead read;

			//Make a call to the read function


			//Save the returned data to the struct
			read.numberOfBytes = 24;
			strcpy(read.readBytes, "testing");

			//Send the struct back to the client
			if (sendto(sock, &read, sizeof(struct responseRead), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseRead)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "write") == 0) {
			struct responseWrite write;

			//Make a call to the write function


			//Save the returned data to the struct
			write.numberOfBytes = 16;
			strcpy(write.writenBytes, "write test");

			//Send the struct back to the client
			if (sendto(sock, &write, sizeof(struct responseWrite), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseWrite)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "lseek") == 0) {
			struct responseLseek lseek;

			//Make a call to the lseek function


			//Save the returned data to the struct
			lseek.position = 9;

			//Send the struct back to the client
			if (sendto(sock, &lseek, sizeof(struct responseLseek), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseLseek)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else {
			//Unexpected instruction. Do nothing.
		}
		/* END OF CONDITIONAL STATEMENTS TO DETEMINE WHICH STRUCT TO SEND BACK TO CLIENT */
    }
    /* NOT REACHED */
}

int findClient(char clientName[]){
	struct client* searchPointer = clientHead;
	int pointerOffset = 0;
	//This products a segfault. You are trying to access searchPointer->name but its NULL because clientHead is NULL
	//When you made the assignment two lines above this comment.
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

struct client* addClient(struct request structBuffer){
	struct client *newClient = malloc(sizeof(struct client));
	//newClient->ip = structBuffer.client_ip;
	strcpy(newClient->ip, structBuffer.client_ip);
	//newClient->name = structBuffer.m;

	//This line is going to overwrite what you did in line 130. Did you mean to do this?
	strcpy(newClient->ip, structBuffer.m);
	newClient->incarnation = structBuffer.i;
	newClient->request = structBuffer.r;
	newClient->files = NULL;
	newClient->next = clientHead;
	clientHead = newClient;
	clientTableSize++;
	return newClient;
}

bool dropClient(char clientName[]){
	int pointerOffset = findClient(clientName);
	struct client* target;
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
	struct client* currentClient = (clientHead + findClient(clientName));
	struct file* currentFiles = currentClient->files;
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
