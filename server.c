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
int findClient(int clientNumber);

struct client* addClient(struct request structBuffer);

bool dropCilent(int clientNumber);

//bool addFile(struct client* client, char fileName[]);

bool writeLock(struct client* client, char fileName[]);

bool readLock(struct client* client, char fileName[]);

bool writeUnlock(struct client* client, char fileName[]);

bool readUnlock(struct client* client, char fileName[]);

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

		clientOffset = findClient(structBuffer.c);

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        printf("Client IP: %s \n", structBuffer.client_ip);
        printf("Machine Name: %s \n", structBuffer.m);
        printf("Client Number: %i \n", structBuffer.c);
        printf("Request Number: %i \n", structBuffer.r);
        printf("Incarnation Number: %i \n", structBuffer.i);
        printf("Operation: %s\n\n", structBuffer.operation);

		if(clientOffset == -1) {
			printf("Adding client... %s\n", addClient(structBuffer)->name);
			printf("Client at %i\n", findClient(structBuffer.c));
			printf("Client Table size: %i\n", clientTableSize);
		}
		


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

int findClient(int clientNumber){
	int pointerOffset = 0;
	while((clientHead + pointerOffset) != NULL && 
		clientNumber != (clientHead + pointerOffset)->clientNumber &&
		(pointerOffset <= clientTableSize)){
		pointerOffset++;
	}
	//check if client is found, if not return -1
	if(pointerOffset > clientTableSize)
		return -1;
	else
		return pointerOffset;
}

struct client* addClient(struct request structBuffer){
	struct client *newClient = (client*) malloc(sizeof(struct client));
	strcpy(newClient->ip, structBuffer.client_ip);
	strcpy(newClient->ip, structBuffer.m);
	newClient->clientNumber = structBuffer.c;
	newClient->incarnation = structBuffer.i;
	newClient->request = structBuffer.r;
	newClient->next = clientHead;
	clientHead = newClient;
	clientTableSize++;
	return newClient;
}

bool dropClient(int clientNumber){
	int pointerOffset = findClient(clientNumber);
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
