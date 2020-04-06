/***********************************************************
 * Author: Navine Rai
 * Date: 2-26-20
 * Description: This program connects to daemon otp_dec_d
 * 		and asks it to decrpyt a cyphertext message
 * 		that it sends using a key that is also sent.
 **********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

void error(char* msg) { perror(msg); exit(0); }

int main(int argc, char* argv[]) {
	int socketFD, fileFD, keyFD, portNumber, charsWritten, charsRead, keyLength, cyphertextLength;
	int allowedVals[27], i, j, goodChar, asciiChar = 65;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[100000], fileBuffer[100000], keyBuffer[100000], filePath[100], keyPath[100];
	ssize_t nCypherRead, nKeyRead;

	if (argc < 4) { fprintf(stderr, "USAGE: %s file key port\n", argv[0]); exit(0); }

	//setup the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[3]); //convert the port number from a string to an integer
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber); //store the port number
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, 
			serverHostInfo->h_length);	//copy in the address

	//setup the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); //create the socket for TCP connection
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	//connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("CLIENT: ERROR connecting");

	//read the contents of the passed in cyphertext file
	strcpy(filePath, "./");
	strcat(filePath, argv[1]);
	if ((fileFD = open(filePath, O_RDONLY)) < 0) { error("ERROR in opening fileFD\n"); }
	//clear the cyphertext buffer 
	memset(fileBuffer, '\0', sizeof(fileBuffer));
	//read the contents of the cyphertext file 
	nCypherRead = read(fileFD, fileBuffer, sizeof(fileBuffer));
	cyphertextLength = nCypherRead - 1;
	//replace the newline with a null terminator
	fileBuffer[strcspn(fileBuffer, "\n")] = '@';
	close(fileFD);

	//initialize the array to hold allowed characters in their ascii form
	for (i = 0; i < 26; i++, asciiChar++) {
		allowedVals[i] = asciiChar;
	}
	allowedVals[i] = 32;
	
	//determine if the cyphertext file has bad characters
	for (i = 0; i < cyphertextLength; i++) {
		goodChar = 0;
		for (j = 0; j < 27; j++) {
			if (fileBuffer[i] == allowedVals[j]) {
				//printf("%c is a valid character\n", fileBuffer[i]);
				goodChar = 1;
				break;
			}
		}
		if (goodChar == 0) { error("ERROR: found bad characters in cyphertext file\n"); }
	}

	strcat(fileBuffer, "@");

	//read the contents of the passed in key file
	strcpy(keyPath, "./");
	strcat(keyPath, argv[2]);
	if ((keyFD = open(keyPath, O_RDONLY)) < 0) { error("ERROR in opening keyFD\n"); }
	memset(keyBuffer, '\0', sizeof(keyBuffer));
	nKeyRead = read(keyFD, keyBuffer, sizeof(keyBuffer));
	keyLength = nKeyRead - 1;
	//keyBuffer[strcspn(keyBuffer, "\n")] = '\0';
	close(keyFD);

	//determine if the key is long enough
	//printf("Key length: %d\n", keyLength);
	//printf("Cyphertext length: %d\n", cyphertextLength);
	if (keyLength < cyphertextLength) { error("CLIENT: Key is not long enough\n"); }

	//send cyphertext to server
	charsWritten = send(socketFD, fileBuffer, strlen(fileBuffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(fileBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//determine if the key has bad characters
	for (i = 0; i < keyLength; i++) {
		goodChar = 0;
		for (j = 0; j < 27; j++) {
			if (keyBuffer[i] == allowedVals[j]) {
				//printf("%c is a valid character\n", keyBuffer[i]);
				goodChar = 1;
				break;
			}
		}
		if (goodChar == 0) { error("ERROR: found bad characters in key file\n"); }
	}

	//send key to server
	charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0);
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(keyBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//get return message from server
	memset(buffer, '\0', sizeof(buffer));
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);

	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	printf("%s\n", buffer);

	//close the connection to the server
	close(socketFD);
	return 0;
}
