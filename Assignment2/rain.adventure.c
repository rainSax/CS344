/****************************************************************
 * Author: Navine Rai
 * Date: 1-22-20
 * Description: Second file for assignment 2, which will read
 * 		data from the files created in part one.
 ***************************************************************/

#include <sys/types.h>		//stat
#include <sys/stat.h>		//stat
#include <dirent.h>		//DIR*
#include <stdio.h>		//printf
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>		//memset
#include <pthread.h>		//mutex and pthreads
#include <time.h>		//time_t
#include <assert.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct file_and_time {
	int descriptor;
	char* str;
};

int countOccurrences(char*, char*);
void* show_time(void*);

void main() {
	//define a stat structure like: struct stat recentDir
	struct stat recentDir;			//dir attributes
	struct dirent* dir;			//subdirectory being scanned
	DIR* folder;				//parent directory
	int newestDirTime = -1;
	int fd, td;				//file descriptor for reading
	int numCharsEntered = -5, endRoomReached = 0;
	char targetDirPrefix[32] = "rain.rooms.";
	char connectionPrefix[32] = "CONNECTION";
	char newestDirName[256];
	char filePath[256];
	char readBuffer[255];
	char playerLocation[255];
	char roomConnections[500];
	char outstr[200];
	char timeFile[200];
	ssize_t nread;
	char* lineEntered;
	size_t bufferSize = 0;
	pthread_t thread;
	int result_code;
	time_t t;
	struct tm* tmp;

	lineEntered = NULL;

	memset(newestDirName, '\0', sizeof(newestDirName));
	

	folder = opendir(".");

	if (folder > 0) {
		while ((dir = readdir(folder)) != NULL) {
			if (strstr(dir->d_name, targetDirPrefix) != NULL) {
				//printf("Found the prefix: %s\n", dir->d_name);
				stat(dir->d_name, &recentDir);

				if((int)recentDir.st_mtime > newestDirTime) {
					newestDirTime = (int)recentDir.st_mtime;
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, dir->d_name);
					//printf("Newer subdir: %s, new time: %d\n", 
							//dir->d_name, newestDirTime);
				}
			}
		}
	}

	//newest subdirectory name is now stored in newestDirName array
	
	//printf("The newest subdirectory is: %s\n", newestDirName);

	folder = opendir(newestDirName);
	memset(targetDirPrefix, '\0', sizeof(targetDirPrefix));
	strcpy(targetDirPrefix, ".");
	if (folder > 0) {
		while((dir = readdir(folder)) != NULL) {
			//find all files *without* the set prefix of . (excludes . and ..)
			if(strstr(dir->d_name, targetDirPrefix) == NULL) {
				//printf("File: %s\n", dir->d_name);
				//find a way to read the contents of a file
				memset(filePath, '\0', sizeof(filePath));
				strcpy(filePath, "./");
				strcat(filePath, newestDirName);
				strcat(filePath, "/");
				strcat(filePath, dir->d_name);

				//printf("Filepath: %s\n", filePath);
				fd = open(filePath, O_RDONLY);
				if (fd < 0)
					printf("Could not open file\n");
				//read the contents of a file into readBuffer
				memset(readBuffer, '\0', sizeof(readBuffer));
				nread = read(fd, readBuffer, sizeof(readBuffer));
				//find the StartRoom
				if (strstr(readBuffer, "START_ROOM") != NULL) {
					//printf("Start room is: %s\n", dir->d_name);
					memset(playerLocation, '\0', sizeof(playerLocation));
					strcpy(playerLocation, dir->d_name);
				}
			}
		}
	}

	//printf("Player starting location: %s\n", playerLocation);
	char connectionName[50];
	char catConnections[500];
	char path[500];
	char* token;
	int count, steps = 0, numConnections = 0;

	memset(path, '\0', sizeof(path));

	while(endRoomReached == 0) {
		count = 1;
		//open the file of the player's current location
		memset(filePath, '\0', sizeof(filePath));
		strcpy(filePath, "./");
		strcat(filePath, newestDirName);
		strcat(filePath, "/");
		strcat(filePath, playerLocation);
		//printf("current file path: %s\n", filePath);
		fd = open(filePath, O_RDONLY);
		if (fd < 0) {
			printf("Could not open room file\n");
			perror("in open()");
			exit(1);
		}

		//clear the read buffer before reading a new room file
		memset(readBuffer, '\0', sizeof(readBuffer));
		nread = read(fd, readBuffer, sizeof(readBuffer));

		//clear the catConnections array - used to list room connections
		memset(catConnections, '\0', sizeof(catConnections));

		//clear the room connections array before each use
		memset(roomConnections, '\0', sizeof(roomConnections));
		strcat(roomConnections, strstr(readBuffer, "CONNECTION"));
		//printf("Room connections contents:\n%s\n", roomConnections);

		//check that current room is the END_ROOM
		if (strstr(roomConnections, "END_ROOM") != NULL) {
			printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
			printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
			printf("%s\n", path);
			endRoomReached = 1;
		}

		else {

			//count the occurrences of a Connection in each file
			int occurrences = countOccurrences(roomConnections, "CONNECTION");
			//printf("Connections Present: %d\n", occurrences);
			
			token = strtok(roomConnections, "\n");
			sscanf(token, "%*s %*d %*s %s", connectionName);
			//printf("Connection name contents: %s\n", connectionName);
			while (token != NULL && count < occurrences) {
				if (count <= 6)
					strcat(connectionName, ", ");
				strcat(catConnections, connectionName);
				//printf("About to cut a token\n");
				token = strtok(NULL, "\n");
				//printf("Connection name: %s\n", connectionName);
				//printf("About to sscanf\n");
				sscanf(token, "%*s %*d %*s %s", connectionName);
				count++;
				//printf("Count is: %d\n", count);
				//printf("Connection name contents: %s\n", connectionName);
			}
			strcat(catConnections, connectionName);
			strcat(catConnections, ".");
			printf("CURRENT LOCATION: %s\n", playerLocation);
			printf("POSSIBLE CONNECTIONS: %s\n", catConnections);
			printf("WHERE TO? >");
			numCharsEntered = getline(&lineEntered, &bufferSize, stdin);

			//get rid of the \n at the end so directory search will not fail
			lineEntered[numCharsEntered - 1] = '\0';

			strcpy(roomConnections, strstr(readBuffer, "CONNECTION"));
			//printf("Room connections contents: %s\n", roomConnections);
			if (strcmp(lineEntered, "time") == 0) {
				//implement time using mutexes
				//printf("Time is being requested...\n");
				t = time(NULL);
				tmp = localtime(&t);
				if (tmp == NULL) {
					perror("localtime");
					exit(EXIT_FAILURE);
				}

				if (strftime(outstr, sizeof(outstr), "%I:%M%P, %A, %B %d, %Y\n", tmp) == 0) {
					fprintf(stderr, "strftime returned 0");
					exit(EXIT_FAILURE);
				}
				//printf("outstr contents: %s\n", outstr);

				memset(timeFile, '\0', sizeof(timeFile));
				strcpy(timeFile, "currentTime.txt");
				td = open(timeFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				struct file_and_time ft = { td, outstr };

				result_code = pthread_create(&thread, NULL, show_time, (void*) &ft);
				assert(0 == result_code);

				result_code = pthread_join(thread, NULL);
				assert(0 == result_code);

				td = open(timeFile, O_RDONLY);

				memset(readBuffer, '\0', sizeof(readBuffer));

				pthread_mutex_lock(&lock);

				read(td, readBuffer, sizeof(readBuffer));
				printf("\n%s", readBuffer);
				
				pthread_mutex_unlock(&lock);

				printf("\nWHERE TO? >");
				numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
				lineEntered[numCharsEntered - 1] = '\0';

				close(td);
			}

			if (strstr(roomConnections, lineEntered) != NULL){
				strcpy(playerLocation, lineEntered);
				//printf("New player Location: %s\n", playerLocation);
				//add playerLocation to a char array delimited by spaces
				strcat(path, playerLocation);
				strcat(path, "\n");
				//increment steps taken
				steps++;
				printf("\n");
			}
			else {
				printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
				//printf("Player Location: %s\n", playerLocation);
			}

			free(lineEntered);
			lineEntered = NULL;
		}
	}

	closedir(folder);
	exit(0);
}

int countOccurrences(char* str, char* toSearch) {
	int i, j, found, count;
	int stringLen, searchLen;

	stringLen = strlen(str);
	searchLen = strlen(toSearch);

	count = 0;

	for (i = 0; i <= stringLen - searchLen; i++) {
		found = 1;
		for (j = 0; j < searchLen; j++) {
			if (str[i + j] != toSearch[j]) {
				found = 0;
				break;
			}
		}

		if (found == 1) {
			count ++;
		}
	}

	return count;
}

void* show_time(void* time) {
	struct file_and_time* myFt = (struct file_and_time*)time;
	//lock the mutex before accessing the shared resource
	pthread_mutex_lock(&lock);

	//printf("About to write to time file\n");
	if (write(myFt->descriptor, myFt->str, strlen(myFt->str) * sizeof(char)) == -1) {
		printf("write encountered an error!\n");
		perror("in write()");
	}

	//unlock the mutex after the shared resource has been used
	pthread_mutex_unlock(&lock);
	close(myFt->descriptor);

	return NULL;
}
