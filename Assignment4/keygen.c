/***********************************************************
 * Author: Navine Rai
 * Date: 2-26-20
 * Description: this program takes one parameter (keylength)
 * 		and creates a key file of the specified
 * 		length.
 **********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void error(char* msg) { perror(msg); exit(0); }

int main(int argc, char* argv[]) {
	if (argc != 2) { fprintf(stderr, "USAGE: %s keylength\n", argv[0]); exit(0); }

	srand(time(NULL));		//generate a new seed every time this program is run

	int i, index, length, allowedVals[27], asciiChar = 65;
	length = atoi(argv[1]);		//convert the keylength argument to an integer
	//printf("length is: %d\n", length);
	char generatedKey[length];

	memset(generatedKey, '\0', sizeof(generatedKey));

	//store the allowed ascii character values in an array
	for (i = 0; i < 26; i++, asciiChar++) {
		allowedVals[i] = asciiChar;
	}
	allowedVals[i] = 32;

	//call rand() with a max bound of the number of elements in the allowedVals array
	for (i = 0; i < length; i++) {
		index = rand() % 27;
		generatedKey[i] = allowedVals[index];
	}

	generatedKey[i] = '\0';

	//print to stdout
	printf("%s\n", generatedKey);

	return 0;
}
