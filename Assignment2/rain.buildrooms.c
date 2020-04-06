/****************************************************************
 * Author: Navine Rai
 * Date: 1-13-20
 * Description: when run, this program will create a directory
 * 		called rain.rooms and fill it with 7 different
 * 		room files. The files will contain descriptions
 * 		of the rooms and how they are connected.
 ***************************************************************/

#include <stdio.h>			//printf
#include <stdlib.h>			//rand
#include <string.h>
#include <time.h>			//for the seed
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

struct room {
	int id;
	char* name;
	int numOutboundConnections;
	int availableConnections;
	int connectionsMade;
	int connectionsToMake;
	struct room* outboundConnections[6];
};

void showRooms();

int main() {
	srand(time(0));

	//create the rooms directory
	
	int pid = getpid();
	int i, j;
	int valFound;			//boolean to track if a name has been used
	char dirName[255];
	char fileName[255];
	sprintf(dirName, "%s%d", "rain.rooms.", pid);
	int dirToCreate = mkdir(dirName, 0755);

	//create a char* array of 10 possible room names
	//and an int array to store used room name indeces

	char* roomNames[10] = { "Foyer", "Living", 
		"Kitchen", "Dining", "Garage", 
       		"Nursery", "Guest", "Office",
		"Dungeon", "Alter"};
	
	/*for (i = 0; i < 10; i++) {
		printf("Room %i: ", (i + 1)); 
		printf("%s\n", roomNames[i]);
	}*/
	int usedNames[7];
	//initialize all values to an impossible index
	for (i = 0; i < 7; i++)
		usedNames[i] = -1;

	//randomly select indeces from 0 - 9, create the files using creat
	//naming them as the room names, store the files in the rooms directory
	//add the  name to the usedNames index

	//find random index, check presence in usedNames, if absent create file with that name
	//and add index to usedNames, ++ loop. if present, start over without incrementing the
	//iteration. Do this 7 times

	int fd, idx, savedIdx, outbound;
	struct room startRoom, endRoom;
	struct room midRoom[5];
	for (i = 0; i < 7;) {
		valFound = 0;
		idx = rand() % 10;
		for (j = 0; j < 7; j++) {
			if (idx == usedNames[j]) {
				valFound = 1;
			}
		}
		if (valFound == 0) {
			usedNames[i] = idx;
			struct room newRoom;
			int k;
			newRoom.name = calloc(32, sizeof(char));
			newRoom.name = roomNames[idx];
			newRoom.numOutboundConnections = (rand() % (6 - 3 + 1)) + 3;
			newRoom.availableConnections = newRoom.numOutboundConnections;
			newRoom.connectionsMade = 0;

			//set room types 1 = start, 2 = mid, 3 = end
			//initialize all pointer members to NULL
			for (k = 0; k < 6; k++)
				newRoom.outboundConnections[k] = NULL;

			if (i == 0) {
				newRoom.id = 1;
				startRoom = newRoom;
			}
			else if (i == 6) {
				newRoom.id = 3;
				endRoom = newRoom;
			}
			else{
				newRoom.id = 2;
				midRoom[i - 1] = newRoom;
				midRoom[i - 1].connectionsToMake = (rand() % (4 - 3 + 1)) + 3;
			}
			i++;
			//use sprintf to specify path to create files
			sprintf(fileName, "%s/%s", dirName, roomNames[idx]);
			//printf("Path: %s\n", fileName);
			//create files in the previously created directory
			fd = creat(fileName, 0644);
		}
	}

	//make room connections now that all rooms have been initialized

	/*********************		START ROOM CONNECTIONS		**********************/

	//connect startroom to a midroom then that midroom back to the start room
	int connectionsToMake;

	//make a 25% chance to connect the startRoom to the endRoom
	int endAdded = 0, endAdd = rand() % 4 + 1;

	int midAdd;
	connectionsToMake = (rand() % (startRoom.numOutboundConnections - 3 + 1)) + 3;

	do {
		for (i = 0; i < connectionsToMake; i++) {
			//randomize the index of midroom selection
			int mIdx = rand() % 5;
			int roomUsed = 0;
			
			//loop through the connections and check for duplicate rooms
			for (j = 0; j < startRoom.numOutboundConnections; j++) {
				if (startRoom.outboundConnections[j] != NULL && 
						strcmp(startRoom.outboundConnections[j]->name, 
						midRoom[mIdx].name) == 0) {
					roomUsed = 1;
				}
			}

			//add a connection to a mid room
			if (startRoom.outboundConnections[i] == NULL && roomUsed == 0) {
				startRoom.outboundConnections[i] = &midRoom[mIdx];
				startRoom.availableConnections--;
				startRoom.connectionsMade++;

				//add a connection back to the start room within the bounds of
				//the mid room's possible connections
				midAdd = 0;
				for (j = 0; j < midRoom[mIdx].numOutboundConnections; j++) {
					if (midRoom[mIdx].outboundConnections[j] == NULL && midAdd == 0) {
						midRoom[mIdx].outboundConnections[j] = &startRoom;
						midRoom[mIdx].availableConnections--;
						midRoom[mIdx].connectionsMade++;
						midAdd = 1;
					}
				}
			}

			//connect the endroom if it was rolled for
			if (endAdd == 1 && startRoom.numOutboundConnections < 6) {
				for (j = 0; j < connectionsToMake; j++) {
					if (startRoom.outboundConnections[j] == NULL) {
						startRoom.outboundConnections[j] = &endRoom;
						startRoom.availableConnections--;
						startRoom.connectionsMade++;
						endRoom.outboundConnections[0] = &startRoom;
						endRoom.availableConnections--;
						endRoom.connectionsMade++;
						break;
					}
				}
				endAdd = 0;
			}

			
			//add the end room if the number of possible start room connections is 6
			for (j = 0; j < startRoom.numOutboundConnections; j++) {
				if (startRoom.outboundConnections[j] == NULL && endAdded == 0) {
					if(startRoom.numOutboundConnections == 6) {
						startRoom.outboundConnections[j] = &endRoom;
						startRoom.availableConnections--;
						startRoom.connectionsMade++;
						endAdded = 1;
						endRoom.outboundConnections[0] = &startRoom;
						endRoom.availableConnections--;
						endRoom.connectionsMade++;
					}
				}
			}

		}
	} while (startRoom.connectionsMade < connectionsToMake);

	/****************		ENDROOM CONSIDERATION		***************/

	int midEnd = 0, endLoop = 0;
	//randomize the number of endroom connections
	connectionsToMake = (rand() % (endRoom.numOutboundConnections - 3 + 1)) + 3;

	do {
		for (i = 0; i < connectionsToMake; i++) {
			//randomize the index of midroom selection
			int mIdx = rand() % 5;
			int roomUsed = 0;

			//check for duplicate rooms
			for (j = 0; j < endRoom.numOutboundConnections; j++) {
				if (endRoom.outboundConnections[j] != NULL && 
						strcmp(endRoom.outboundConnections[j]->name, 
						midRoom[mIdx].name) == 0) {
					roomUsed = 1;
				}
			}

			if (endRoom.outboundConnections[i] == NULL && roomUsed == 0) {
				endRoom.outboundConnections[i] = &midRoom[mIdx];
				endRoom.availableConnections--;
				endRoom.connectionsMade++;
				midEnd = 0;
				midAdd = 0;
				for (j = 0; j < midRoom[mIdx].numOutboundConnections; j++) {
					if (midRoom[mIdx].outboundConnections[j] == NULL && midAdd == 0) {
						midRoom[mIdx].outboundConnections[j] = &endRoom;
						midRoom[mIdx].availableConnections--;
						midRoom[mIdx].connectionsMade++;
						midAdd = 1;
					}
				}
			}
			else {
				endLoop++;
			}
		}
		if (endLoop >= 10)
			break;

	} while (endRoom.connectionsMade < connectionsToMake);

	/*for (i = 0; i < 5; i++)
		midRoom[i].connectionsToMake = (rand() % (midRoom[i].availableConnections - 3 + 1)) + 3;*/


	/****************		MIDROOM CONNECTIONS		******************/

	//loop through all 5 midrooms repeatedly while empty connections exist
	int l, k, usedIndeces[5], indexUsed = 0, fullRooms = 0, count = 0, badJ = 0, loopCount = 0;
	for (i = 0; i < 5; i++)
		usedIndeces[i] = -1;
	for (i = 0; i < 5; i++) {
		int uIdx = 0;
		loopCount = 0;
		do {
			int roomUsed = 0;
			loopCount++;
			idx = rand() % 5;
			if (strcmp(midRoom[i].name, midRoom[idx].name) != 0) {
				//if midroom can't make more connections, i never gets incremented
				if (midRoom[i].connectionsMade < midRoom[i].connectionsToMake && 
						midRoom[idx].connectionsMade < midRoom[idx].connectionsToMake) {
					for (j = 0; j < midRoom[i].numOutboundConnections; j++) {
						if (midRoom[i].outboundConnections[j] != NULL && 
							       	strcmp(midRoom[i].outboundConnections[j]->name, midRoom[idx].name) == 0) {
							roomUsed = 1;
						}
					}
					//if no duplicate random room was found...
					if (roomUsed == 0) {
						//add the random connection at the first midroom's NULL connection
						for (j = 0; j < midRoom[i].numOutboundConnections; j++) {
							if (midRoom[i].outboundConnections[j] == NULL) {
								midRoom[i].outboundConnections[j] = &midRoom[idx];
								midRoom[i].connectionsMade++;
								midRoom[i].availableConnections--;
								break;
							}
						}
						for (j = 0; j < midRoom[idx].numOutboundConnections; j++) {
							if (midRoom[idx].outboundConnections[j] == NULL) {
								midRoom[idx].outboundConnections[j] = &midRoom[i];
								midRoom[idx].connectionsMade++;
								midRoom[idx].availableConnections--;
								break;
							}
						}
					}
				}
				//check for no rooms left to add to condition
				for (j = 0; j < 5; j++) {
					if (strcmp(midRoom[i].name, midRoom[j].name) != 0) {
						for (k = 0; k < midRoom[i].numOutboundConnections; k++) {
							if (midRoom[i].outboundConnections[k] != NULL) {
								if (strcmp(midRoom[i].outboundConnections[k]->name, midRoom[j].name) == 0) {
									for (l = 0; l < 5; l++) {
										if (usedIndeces[l] == j)
											indexUsed = 1;
									}
									if (indexUsed == 0) {
										usedIndeces[uIdx] = j;
										uIdx++;
									}
								}
							}
						}
					}
				}

				//if all others are full...try to add the start/end rooms
				for (j = 0; j < 5; j++) {
					badJ = 0;
					if (strcmp(midRoom[i].name, midRoom[j].name) != 0) {
						//if j does not equal any index within usedIndeces array...
						for (k = 0; k < 5; k++) {
							if (j == usedIndeces[k])
								badJ = 1;
						}
						if (badJ == 0) {
							for (k = 0; k < midRoom[j].numOutboundConnections; k++) {
								if (midRoom[j].outboundConnections[k] != NULL)
									count++;
							}
							if (count == midRoom[j].connectionsToMake)
								fullRooms++;
						}
					}
				}
			}
			//counter used to break out of an infinite loop
			if (loopCount >= 5)
				break;
		} while (midRoom[i].connectionsMade < midRoom[i].availableConnections);
	}

	//write the struct data from each room into the appropriate files
	
	ssize_t nwritten;
	char buffer[256];
	for (i = 0; i < 7; i++) {
		sprintf(fileName, "%s/%s", dirName, roomNames[usedNames[i]]);
		fd = open(fileName, O_WRONLY | O_TRUNC, 0600);
		if (roomNames[usedNames[i]] == startRoom.name) {
			sprintf(buffer, "%s: %s\n", "ROOM NAME", startRoom.name);
			nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
			for (j = 0; j < startRoom.numOutboundConnections; j++) {
				if (startRoom.outboundConnections[j] != NULL) {
					sprintf(buffer, "%s %d: %s\n", "CONNECTION", (j + 1), startRoom.outboundConnections[j]->name);
					nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
				}
			}
			sprintf(buffer, "%s: %s\n", "ROOM TYPE", "START_ROOM");
			nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
		}
		//cycle through the midrooms
	
		for (j = 0; j < 5; j++) {
			if (roomNames[usedNames[i]] == midRoom[j].name) {
				int k;
				//add midRoom data	
				sprintf(buffer, "%s: %s\n", "ROOM NAME", midRoom[j].name);
				nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
				//cycle through this midroom's outbound connections
				for (k = 0; k < midRoom[j].numOutboundConnections; k++) {
					if (midRoom[j].outboundConnections[k] != NULL) {
						sprintf(buffer, "%s %d: %s\n", "CONNECTION", (k + 1), midRoom[j].outboundConnections[k]->name);
						nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
					}
				}
				sprintf(buffer, "%s: %s\n", "ROOM TYPE", "MID_ROOM");
				nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
			}
		}

		if (roomNames[usedNames[i]] == endRoom.name) {
			sprintf(buffer, "%s: %s\n", "ROOM NAME", endRoom.name);
			nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
			for (j = 0; j < endRoom.numOutboundConnections; j++) {
				if (endRoom.outboundConnections[j] != NULL) {
					sprintf(buffer, "%s %d: %s\n", "CONNECTION", (j + 1), endRoom.outboundConnections[j]->name);
					nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
				}
			}
			sprintf(buffer, "%s: %s\n", "ROOM TYPE", "END_ROOM");
			nwritten = write(fd, buffer, strlen(buffer) * sizeof(char));
		}

		fd = close(fileName);
	}

	return 0;
}

/*****************		DEBUG			****************/

	void showRooms(struct room* room) {
		int i, j;
		if (room->id == 1) {
			printf("Start Room: %s, id: %i, connections: %i, remaining: %i\n", room->name, room->id, 
					room->numOutboundConnections, room->availableConnections);

			for (i = 0; i < room->numOutboundConnections; i++) {
				if (room->outboundConnections[i] == NULL)
					printf("No connection found\n");
				else
					printf("Connection %i: %s\n", (i + 1), room->outboundConnections[i]->name);
			}
		}

		else if (room->id == 2) {
			printf("Mid Room: %s, id: %i, max connections: %i, connections made: %i, remaining: %i, toMake: %i\n", room->name, 
					room->id, room->numOutboundConnections, room->connectionsMade, room->availableConnections, 
					room->connectionsToMake);

			for (j = 0; j < room->numOutboundConnections; j++) {
				if (room->outboundConnections[j] == NULL)
					printf("No connection found\n");
				else
					printf("Connection %i: %s\n", (j + 1), 
							room->outboundConnections[j]->name);
			}
		}

		else {
			printf("End Room: %s, id: %i, connections: %i, remaining: %i\n", room->name, room->id, 
					room->numOutboundConnections, room->availableConnections);
			for (i = 0; i < room->numOutboundConnections; i++) {
				if (room->outboundConnections[i] == NULL)
					printf("No connection found\n");
				else
					printf("Connection %i: %s\n", (i + 1), 
							room->outboundConnections[i]->name);
			}
		}
	}
