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

int openFile(char * fileName, char * mode);
int closeFile(char * fileName);
void readFile(char * fileName, int readBytes, char *buffer);
void writeFile(char * fileName, char *writeBuffer);
void lseekFile(char * fileName, int position);

void removeQuotes(char * string);

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

		//Remove the instruction keyword from the string
		char fileNameTemp[operationLength+1];
		strcpy(fileNameTemp, structBuffer.operation);
		char * operationFileName = fileNameTemp;
		while (*operationFileName != 0 && *(operationFileName++) != ' ') {}

		//Get the first word of the left over instuction string which gives us the file name the client wants to access
		size_t fileNamePlusAdditional = strlen(operationFileName);
		char tempstr[fileNamePlusAdditional+1];
		char * fileName;
		strcpy(tempstr, operationFileName);
		fileName = strtok(tempstr, " ,");

		printf("The instruction in the operation received is: %s\n", instruction);
		printf("The file name received from client is: %s\n", fileName);

		char paramTemp[fileNamePlusAdditional+1];
		strcpy(paramTemp, operationFileName);
		char * param = paramTemp;
		//Check if request from client will have a 3rd parameter
		if (strcmp(instruction, "open") == 0 || strcmp(instruction, "read") == 0 ||
				strcmp(instruction, "write") == 0 || strcmp(instruction, "lseek") == 0) {
			//Get the last parameter in the clients request

			while (*param != 0 && *(param++) != ' ') {}
/*
			//Remove carriage return
			char *paramCleanReturn;
			if ((paramCleanReturn=strchr(param, '\r')) != NULL) {
			    *paramCleanReturn = '\0';
			    printf("Do I get here in r?");
			}
*/
			//Remove newline
			char *paramClean;
			if ((paramClean=strchr(param, '\n')) != NULL) {
			    *paramClean = '\0';
			    printf("Do I get here in newline?");
			}

/*
			for(int i = 0; param[i] != '\0'; i++) {

				if (param[i] == '\0') {
					printf("End of string found!");
				}
				if (param[i] == ' ') {
					printf("Space found!");
				}
				if (param[i] == '\r') {
					printf("Carriage return found!");
				}
				if (param[i] == '\n') {
					printf("Newline found!");
				}
			    printf("Char %d: %c\n", i, param[i]);

			}
*/

			printf("3rd parameter of request is: %s\n", param);
		}

		//Decide which operation to perform and the type of struct to send back to the client
		//based on the request.
		/********ALL STRUCTS ARE CURRENTLY RETURNING TEMPORARY FAKE DATA SINCE FUNCTIONS DON'T EXIST TO SEND REAL DATA BACK TO CLIENT***********/
		if (strcmp(instruction, "open") == 0) {
			struct responseOpen open;

			//Make a call to the open function and save the returned data to the struct
			open.fileDescriptor = openFile(fileName, param);
			printf("Open File Report: %d\n", open.fileDescriptor);

			//Send the struct back to the client
			if (sendto(sock, &open, sizeof(struct responseOpen), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseOpen)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "close") == 0) {
			struct responseClose close;

			//Make a call to the close function
			//Save the returned data to the struct
			close.fileDescriptor = closeFile(fileName);

			//Send the struct back to the client
			if (sendto(sock, &close, sizeof(struct responseClose), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseClose)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "read") == 0) {
			struct responseRead read;

			//Make a call to the read function
			char *readBuffer = malloc(atoi(param));
			readFile(fileName, atoi(param), readBuffer);

			//Save the returned data to the struct
			read.numberOfBytes = atoi(param);
			strcpy(read.readBytes, readBuffer);

			//Free malloc assigned memory
			free(readBuffer);

			//Send the struct back to the client
			if (sendto(sock, &read, sizeof(struct responseRead), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseRead)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "write") == 0) {
			struct responseWrite write;

			//Remove quotes from param
			//char removeCharacter = '"';
			//removeQuotes(param);

			//Make a call to the write function
			writeFile(fileName, param);

			//Save the returned data to the struct
			write.numberOfBytes = sizeof(param);
			strcpy(write.writenBytes, param);

			//Send the struct back to the client
			if (sendto(sock, &write, sizeof(struct responseWrite), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseWrite)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "lseek") == 0) {
			struct responseLseek lseek;

			//Make a call to the lseek function
			lseekFile(fileName, param);

			//Save the returned data to the struct
			lseek.position = param;

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
	struct client *newClient = (struct client*) malloc(sizeof(struct client));
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

/*
 * open function for the client
 */
int openFile(char * fileName, char * mode) {
	FILE *fp;
	printf("Mode: %s\n", mode);
	if (strcmp(mode, "read") == 0) {
		//Client wants to read
		printf("File name to open: %s\n", fileName);
		fp = fopen(fileName, "r");
		if (fp == NULL) {
		   printf("Error: could not open %s\n", fileName);
		   return -1;
		}
	} else {
		//Client wants to write
		printf("File name to open: %s\n", fileName);
		fp = fopen(fileName, "r+");
		if (fp == NULL) {
		   printf("Error: could not open %s\n", fileName);
		   return -1;
		}
	}
	fclose(fp);
	return 1;
}

/*
 * close file
 */
int closeFile(char * fileName) {
	printf("Close file %s", fileName);
	return 1;
}

/*
 * readFile
 * Params:
 * fileName: the name of the file to read from
 * readBytes: the number of chars (bytes) to read
 * buffer: the char array to save the read bytes to
 */
void readFile(char * fileName, int readBytes, char *buffer) {
	FILE *fp;
	fp = fopen(fileName, "r");
	//Read from file and return it
	if (fp != NULL) {
		size_t newLen = fread(buffer, sizeof(char), readBytes, fp);
		if (newLen == 0) {
			fputs("Error reading file", stderr);
		} else {
			buffer[++newLen] = '\0'; /* Just to be safe. */
		}
	}
	fclose(fp);
}

/*
 * writeFile
 *
 *
 */
void writeFile(char * fileName, char *writeBuffer) {
	FILE *fp;
	fp = fopen(fileName, "r+");
	if (fp != NULL) {
		fwrite(writeBuffer, 1, sizeof(writeBuffer), fp);
	}
	fclose(fp);
}

void lseekFile(char * fileName, int position) {

}

/*
 * Helper function to remove quotes from string
 */
void removeQuotes(char * string) {
	size_t len = strlen(string);
	memmove(string, string+1, len-3);
}
