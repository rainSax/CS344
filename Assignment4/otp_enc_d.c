/***********************************************************
 * Author: Navine Rai
 * Date: 2-26-20
 * Description: Encryption daemon program which will run
 * 		in the background and encrypt the plaintext
 * 		through child processes spawned with fork().
 **********************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

//function to exit program (status 1) and report issues
void error(char* msg) { perror(msg); exit(1); }

//encryption function
void encrypt(char* plaintext, int textLength, char* key, char* encryptionBuffer) {
	char charVals[27], encryptedText[textLength];
	int i, j, messageVals[textLength], keyVals[textLength], encryptedVals[textLength];
	//printf("About to encrypt message: %s\nOf length: %d\n", plaintext, textLength);
	//printf("Using key: %s\n", key);

	//printf("first letter: %c\n", plaintext[0]);

	//store acceptable characters in an array where A is 0, B is 1 and so on
	for (i = 0, j = 65; i < 26; i++, j++)
		charVals[i] = j;
	//index 26 is the space character
	charVals[i] = 32;

	//for each letter in plaintext and key, find their corresponding numeric indeces (charVals)
	//and store them in messageVals and keyVals, respectively
	for (i = 0; i < textLength; i++) {
		for (j = 0; j < 27; j++) {
			if (plaintext[i] == charVals[j]) {
				messageVals[i] = j;
				break;
			}
		}

		for(j = 0; j < 27; j++) {
			if(key[i] == charVals[j]) {
				keyVals[i] = j;
				break;
			}
		}
	}

	/*for (i = 0; i < textLength; i++)
		printf("%d ", messageVals[i]);

	printf("\n\n");

	for (i = 0; i < textLength; i++)
		printf("%d ", keyVals[i]);

	printf("\n\n");
	*/

	for (i = 0; i < textLength; i++) {
		encryptedVals[i] = messageVals[i] + keyVals[i];
		if (encryptedVals[i] > 26)
			encryptedVals[i] -= 27;
		//printf("%d ", encryptedVals[i]);
	}

	//use each value of encryptedVals as an index of charVals and add to the encrypted message array
	for (i = 0; i < textLength; i++)
		encryptionBuffer[i] = charVals[encryptedVals[i]];

	return;
}

int main(int argc, char* argv[]) {
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, textCharsRead, keyCharsRead;
	int spawnPid, childExitMethod;
	socklen_t sizeOfClientInfo;
	char plaintextBuffer[100000], chunkBuffer[2], keyBuffer[100000], encryptedMsg[100000];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1); }  //check usage & args

	//set up the address struct for this process (encryption daemon)
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[1]);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	//create the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocketFD < 0) error("ERROR opening socket");

	//bind socket to a port
	if (bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("ERROR on binding");

	//turn on the socket and allow it to queue up to 5 connections
	listen(listenSocketFD, 5);

	while(1) {
		sizeOfClientInfo = sizeof(clientAddress); //get the size of the address for connecting client
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*)&clientAddress, 
				&sizeOfClientInfo);

		if (establishedConnectionFD < 0) error("ERROR on accept");

		//call fork here to implement one fork/client connection
		spawnPid = fork();
		switch(spawnPid) {
			case -1:
				error("Failed to create child");
				break;
			case 0:
				printf("I am the child %d\n", getpid());
				
				//clear the buffer for the whole message to encrypt
				memset(plaintextBuffer, '\0', 100000);
				
				//fill a very small chunk buffer repeatedly until the sequence @@ is found
				//which signifies the end of the message
				while(strstr(plaintextBuffer, "@@") == NULL) {
					memset(chunkBuffer, '\0', 10);
					textCharsRead = recv(establishedConnectionFD, chunkBuffer, 
						sizeof(chunkBuffer) - 1, 0);
					strcat(plaintextBuffer, chunkBuffer);
					/*printf("SERVER: received chunk: \"%s\", total: \"%s\"\n", 
							chunkBuffer, plaintextBuffer);*/
					if (textCharsRead == -1) { printf("r == -1\n"); break; }
					if (textCharsRead == 0) { printf("r == 0\n"); break; }
				}

				int terminalLocation = strstr(plaintextBuffer, "@@") - plaintextBuffer;
				plaintextBuffer[terminalLocation] = '\0';

				//call recv again to receive the key
				memset(keyBuffer, '\0', 100000);
				keyCharsRead = recv(establishedConnectionFD, keyBuffer, 199999, 0);
				if (keyCharsRead < 0) error("ERROR reading from client socket");
				//printf("SERVER: I received this key from the client: \"%s\"\n", keyBuffer);

				//have this child process encrypt the plaintext using modulo 27
				memset(encryptedMsg, '\0', 100000);
				encrypt(plaintextBuffer, strlen(plaintextBuffer), keyBuffer, encryptedMsg);

				printf("Encrypted text: %s\n", encryptedMsg);
				encryptedMsg[strlen(encryptedMsg)] = '\n';

				//send the encrypted text back to the client process
				charsRead = send(establishedConnectionFD, encryptedMsg, 
						strlen(encryptedMsg), 0);

				if (charsRead < 0) error("ERROR sending success message to client");
				close(establishedConnectionFD);
				exit(0);
				break;
			default:
				printf("I am the parent %d\n", getpid());

				//wait for the child to terminate
				waitpid(spawnPid, &childExitMethod, 0);

				//report how the child died
				if (WIFEXITED(childExitMethod)) {
					printf("The child %d exited with status: ", spawnPid);
					printf("%d\n", WEXITSTATUS(childExitMethod));
				}
				if (WIFSIGNALED(childExitMethod)) {
					printf("The child %d was terminated by signal: ", spawnPid);
					printf("%d\n", WTERMSIG(childExitMethod));
				}
				break;
		};
	}

	close(listenSocketFD);
	return 0;
}
