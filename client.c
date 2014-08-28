#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "client.h"

#define FILENAMEMAX 24     /* Longest string to echo */
FILE *readFile;

void DieWithError(const char *errorMessage) /* External error handling function */
{
    perror(errorMessage);
    exit(1);
}

void sendRequest(char client_ip[16], char m[24], int c, int r, int i, char *operation, char *servIP, unsigned short serverPort)
{
	int sock;                        /* Socket descriptor */
	struct sockaddr_in echoServAddr; /* Echo server address */
	struct sockaddr_in fromAddr;     /* Source address of echo */
	unsigned int fromSize;           /* In-out of address size for recvfrom() */
	char fileNameBuffer[FILENAMEMAX+1];  /* Buffer for receiving file name string */
	int respStringLen;               /* Length of received response */
    int operationStringLen;     /* Length of string to echo */

    operationStringLen = strlen(operation);

    if (operationStringLen > FILENAMEMAX)  /* Check input length */
        DieWithError("Echo word too long");

    printf("Client IP: %s \n", client_ip);
    printf("Machine Name: %s \n", m);
    printf("Client Number: %i \n", c);
    printf("Request Number: %i \n", r);
    printf("Incarnation Number: %i \n", i);

	/* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	echoServAddr.sin_port   = htons(serverPort);     /* Server port */

	/* Send the string to the server */
	if (sendto(sock, operation, operationStringLen, 0, (struct sockaddr *)
			   &echoServAddr, sizeof(echoServAddr)) != operationStringLen)
		DieWithError("sendto() sent a different number of bytes than expected");

	/* Recv a response */
	fromSize = sizeof(fromAddr);
	if ((respStringLen = recvfrom(sock, fileNameBuffer, FILENAMEMAX, 0,
		 (struct sockaddr *) &fromAddr, &fromSize)) != operationStringLen)
		DieWithError("recvfrom() failed");

	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
	{
		fprintf(stderr,"Error: received a packet from unknown source.\n");
		exit(1);
	}

	/* null-terminate the received data */
	fileNameBuffer[respStringLen] = '\0';
	printf("Received: %s\n\n", fileNameBuffer);    /* Print the script file name arg */

	close(sock);
}

int main(int argc, char *argv[])
{
	unsigned short serverPort;     /* Echo server port */
    char *servIP;                    /* IP address of server */
    char *scriptFileName;            /* File name of script. */
    char *machineName;				 /* Machine name of the client */
    int clientNumber;				 /* Number of the client */

    if ((argc < 5) || (argc > 6))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> <Machine Name> [<Client Number>] [<Echo Port>] <Script File Name> \n", argv[0]);
        exit(1);
    }

    servIP = argv[1];       /* First arg: server IP address (dotted quad) */
    machineName = argv[2];
    clientNumber = atoi(argv[3]);
    scriptFileName = argv[5];       /* Second arg: string to echo */
    serverPort = atoi(argv[4]);  /* Use given port */

    //Read from the script file
    char command[100];

    readFile = fopen(scriptFileName, "r");
	if (readFile == NULL) {
	   printf("Error: could not open %s\n", scriptFileName);
	   exit(1);
	}

	while (fgets(command, sizeof command, readFile) != NULL) /* Read a line */
	{
		//Get the first word of the command
		size_t commandLength = strlen(command);
		char localstr[commandLength+1];
		char * instruction;
		strcpy(localstr, command);
		instruction = strtok(localstr, " ,");

		//Remove newline character from instruction to compare.
		size_t ln = strlen(instruction) - 1;
		if (instruction[ln] == '\n') {
			instruction[ln] = '\0';
		}

		//printf("The instruction of the line is: %s\n", instruction);

		if (strcmp(instruction, "fail") == 0) {
			printf("Command Failed. Will not send to server. \n\n");
		} else {
			sendRequest("192.168.1.1", machineName, clientNumber, 1, 1, command, servIP, serverPort);
		}

	}

	fclose(readFile);

    exit(0);
}
