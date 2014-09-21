#include <stdio.h>      /* for printf() and fprintf() and file access*/
#include <sys/socket.h> /* for socket() and bind() */
#include <sys/file.h>	/* for flock() */
#include <fcntl.h>		/* for open() and read() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close(), write(), and lseek */
#include <stdbool.h>    /* for boolean types */
#include "server.h"

#define ECHOMAX 225     /* Longest string to echo */

static struct client* clientHead;
static struct client* currentClient;
static int clientTableSize;

//forward declarations
struct client* findClient(int clientNumber);

struct client* addClient(struct request structBuffer);

bool dropCilent(int clientNumber);

int closeFile(int fileDescriptor);

int openFile(char * fileName, char * mode);

char * readFile(int fileDescriptor, char* buffer, int readBytes);

int writeFile(int fileDescriptor, char* string); 

int seekFile(int fileDescriptor, int offset);

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
	currentClient = NULL;
	char *ownerPrefix =(char*) malloc(sizeof(char[24]));//holds machine name

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

		currentClient = findClient(structBuffer.c);

		//printf("Client: %i\n", currentClient);
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        printf("Client IP: %s \n", structBuffer.client_ip);
        printf("Machine Name: %s \n", structBuffer.m);
        printf("Client Number: %i \n", structBuffer.c);
        printf("Request Number: %i \n", structBuffer.r);
        printf("Incarnation Number: %i \n", structBuffer.i);
        printf("Operation: %s\n\n", structBuffer.operation);

		if(currentClient == NULL) {
			printf("Adding client... %s\n", addClient(structBuffer)->name);
			//printf("Client at %i\n", findClient(structBuffer.c));
			printf("Client Table size: %i\n", clientTableSize);
			currentClient = findClient(structBuffer.c);
		}
		else{
			//printf("Current file number: % \n", currentClient->file);
		}

		/*DEBUG
		printf("Comparing client. . .\n");
		printf("\n\n--Client Table Data--\n");
		printf("Client IP: %s \n", currentClient->ip);
        printf("Machine Name: %s \n", currentClient->name);
		printf("Client Number: %i \n", currentClient->clientNumber);
        printf("Request Number: %i \n", currentClient->request);
        printf("Incarnation Number: %i \n", currentClient->incarnation);
		printf("File #: %i\n", currentClient->file);
		*/

		//ignore request: already fulfilled
			if(structBuffer.r < currentClient->request)
				continue;
		//check for incarnation problems
			if(structBuffer.i > currentClient->incarnation){
				//client has failed: remove all locks/close files for abandoned clients
				struct client* incarnationScrub = clientHead;
				while(incarnationScrub != NULL){
					if(strcmp(incarnationScrub->name, structBuffer.m)==0 && incarnationScrub->file != -1){
						closeFile(incarnationScrub->file);
						incarnationScrub->file = -1;
					}
				}
			}

		//simulate random noise roll a 99 sided dice
		//int roll = rand() % 99;
		//if(roll < 33){
		//	printf("dropping request (oops!)\n");
		//	continue;
		//}	
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
		strcpy(ownerPrefix, currentClient->name);
		strcpy(tempstr, operationFileName);
		fileName = strtok(tempstr, " ,");
		strcpy(ownerPrefix, currentClient->name);

		printf("The instruction in the operation received is: %s\n", instruction);
		printf("The file name received from client is: %s\n", fileName);
			

		char paramTemp[fileNamePlusAdditional+1];
		strcpy(paramTemp, operationFileName);
		char * param = paramTemp;
		//Check if request from client will have a 3rd parameter
		if (strcmp(instruction, "open") == 0 || strcmp(instruction, "read") == 0 ||
				strcmp(instruction, "write") == 0 || strcmp(instruction, "lseek") == 0) {

			//format file name for access	
			fileName = strcat(strcat(ownerPrefix, ":"), fileName);
			printf("The machine file name is: %s\n", fileName);

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
			    //printf("Do I get here in newline?");
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

		//loosing in transit
		//if (roll > 66){
		//	printf("loosing request (oops!)\n");
		//	continue;
		//}

		//Decide which operation to perform and the type of struct to send back to the client
		//based on the request.
		if (strcmp(instruction, "open") == 0) {
			struct responseOpen open;

			//Make a call to the open function and save the returned data to the struct
			currentClient->file = openFile(fileName, param);
			open.fileDescriptor = currentClient->file;
			printf("Open File Report: %d\n", open.fileDescriptor);

			//Send the struct back to the client
			if (sendto(sock, &open, sizeof(struct responseOpen), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseOpen)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}

		} else if (strcmp(instruction, "close") == 0) {
			struct responseClose close;

			//Make a call to the close function
			close.fileDescriptor = closeFile(currentClient->file);

			//Send the struct back to the client
			if (sendto(sock, &close, sizeof(struct responseClose), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseClose)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "read") == 0) {
			struct responseRead read;

			readFile(currentClient->file, read.readBytes, atoi(param));

			//Save the returned data to the struct
			read.numberOfBytes = atoi(param);

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
			writeFile(currentClient->file, param);

			//Save the returned data to the struct
			write.numberOfBytes = sizeof(param);
			strcpy(write.writenBytes, param);

			//Send the struct back to the client
			if (sendto(sock, &write, sizeof(struct responseWrite), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseWrite)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else if (strcmp(instruction, "lseek") == 0) {
			struct responseLseek seek;

			//Make a call to the lseek function and save returned data to struct
			seek.position = seekFile(currentClient->file, atoi(param));

			//Send the struct back to the client
			if (sendto(sock, &seek, sizeof(struct responseLseek), 0,(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != sizeof(struct responseLseek)) {
				DieWithError("sendto() sent a different number of bytes than expected");
			}
		} else {
			//Unexpected instruction. Do nothing.
		}
		/* END OF CONDITIONAL STATEMENTS TO DETEMINE WHICH STRUCT TO SEND BACK TO CLIENT */

		//update request number
		currentClient->request = structBuffer.r;
		printf("finished command\n");
    }
    /* NOT REACHED */
}

struct client* findClient(int clientNumber){
	//DEBUG
	//printf("find client..\n");
	struct client* clientPointer = clientHead;
	while((struct client*)(clientPointer) != NULL && 
		(clientNumber != (clientPointer)->clientNumber)){
			clientPointer = clientPointer->next;
	}
	return clientPointer;
}

struct client* addClient(struct request structBuffer){
	struct client *newClient = (struct client*) malloc(sizeof(struct client));
	strcpy(newClient->ip, structBuffer.client_ip);
	strcpy(newClient->name, structBuffer.m);
	newClient->clientNumber = structBuffer.c;
	newClient->incarnation = structBuffer.i;
	newClient->request = structBuffer.r;
	newClient->next = clientHead;
	newClient->file = -1;
	clientHead = newClient;
	clientTableSize++;
	return newClient;
}

bool dropClient(int clientNumber){
	struct client* clientPointer = findClient(clientNumber);
	struct client* target;
	//check to see if client exists
	if (clientPointer == NULL)
		return false;
	//the client is at the head of the list
	else if(clientPointer == clientHead){
		target = clientHead;
		clientHead = clientHead -> next;
		free(target);
	}
	else{
		target = (clientPointer);
		clientPointer = clientHead;
		while(clientPointer != NULL && clientPointer->next != target){
			clientPointer = clientPointer->next;
		}
		clientPointer = target->next;
		free(target);
	}
	
	return true;
}

/*
 * open function for the client, placing the correct lock on the file
 * returns a filed descriptor int
 */
int openFile(char * fileName, char * mode) {
	int fd;
	printf("Mode: %s\n", mode);
	if (strcmp(mode, "read") == 0) {
		//Client wants to read
		printf("File name to open: %s\n", fileName);
		fd = open(fileName, O_RDONLY);
		//try to give a shared lock to the file
		if (flock(fd, LOCK_SH) == -1) {
		   //file might not exist yet!
			fd = open(fileName, O_RDONLY | O_CREAT);
			if(flock(fd, LOCK_SH) == -1){
				printf("Error: could not open %s\n", fileName);
			 //release file descriptor
			   close(fd);
			   return -1;
			}
		}
	}
	else {
		//Client wants to write
		printf("File name to open: %s\n", fileName);
		fd = open(fileName, O_RDWR);
		//try to give an exclusive lock to the file
		if (flock(fd, LOCK_EX) == -1) {
			//file might not exist yet!
			fd = open(fileName, O_RDWR | O_CREAT);
			if(flock(fd, LOCK_EX) == -1){
				printf("Error: could not open %s\n", fileName);
		   //release file descriptor
			   close(fd);
			   return -1;
			}
		}
	}
	return fd;
}

//close user access to the file(also releases user locks)
int closeFile(int fileDescriptor){
	if(close(fileDescriptor) == -1){
		printf("Could not close file\n");
		return -1;
	}
	else return 1;
}

//Read a specified number of bits from a file, return the string out
char * readFile(int fileDescriptor, char* buffer, int readBytes) {
	read(fileDescriptor, buffer, readBytes);
	//Read from file and return it
	return buffer;
}

//writes specified string to file, returns # of bytes written
int writeFile(int fileDescriptor, char* string){
	return write(fileDescriptor, string, strlen(string));
}

//seeks within a file, returns how many bytes were offset (from the begining of the file).
int seekFile(int fileDescriptor, int offset){
	//DEBUG
	//printf("seeking!\n");
	return lseek(fileDescriptor, offset, SEEK_SET);
}
