#ifndef SERVER_H
#define SERVER_H

	struct request {
		char   client_ip[16];   /* Holds client IP address in dotted decimal */
		char   m[24];           /* Name of machine on which client is running */
		int    c;               /* Client number */
		int    r;               /* Request number of client */
		int    i;               /* Incarnation number of client’s machine */
		char   operation[80];  /* File operation client sends to server */
	};
	
	struct file {
		char fileName[24];
		char ownerName[24];
		bool readLock;
		bool writeLock;
		struct file* next;
	};

	struct client {
		char ip[16];
		char name[24];
		int incarnation;
		int request;
		struct file* files;
		struct client* next;
	};

	struct responseOpen {
		int fileDescriptor;
	};

	struct responseClose {
		int fileDescriptor;
	};

	struct responseRead {
		int numberOfBytes;
		char readBytes[80];
	};

	struct responseWrite {
		int numberOfBytes;
		char writenBytes[80];
	};

	struct responseLseek {
		int position;
	};

#endif
