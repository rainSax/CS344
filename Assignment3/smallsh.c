/***********************************************************
  * Author: Navine Rai
  * Date: 2-7-20
  * Description: Assignment 3 for class CS344. Creating a
  * 		 shell using C with functionality of cd,
  * 		 exit, and status implemented.
***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

int bgPIDs[500], fgOnly = 0, exitTermStatus = 0;
char exitTermStr[200];
struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}, SIGUSR2_action = {0}, ignore_action = {0};

void startShell() {
	printf("Welcome to my shell %s. Use with caution!\n", getenv("USER"));
}

void catchSIGINT(int signo) {
	char* message = "Caught SIGINT, killing pid \n";
	write(STDOUT_FILENO, message, 28);
	printf("%d\n", getpid());
	fflush(stdout);
	kill(getpid(), 2);
	return;
}

void catchSIGTSTP(int signo) {
	if (fgOnly == 0) {
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
		fgOnly = 1;
	}
	else {
		char* message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
		fgOnly = 0;
	}
	write(STDOUT_FILENO, ": ", 2);
}

void execute(char** argv, int numArgs) {
	int spawnPid = -5;
	int childExitMethod = -5;
	int fdOut, fdIn, i, redirInput = 0, redirOutput = 0, redirOutLoc = -5, redirInLoc = -5, bg = -5;

	if (strcmp(*argv, "cd") == 0) {
		char* path = NULL;
		if(numArgs > 1)
			path = argv[1];
		else
			path = getenv("HOME");
		if(chdir(path) < 0) {
			printf("Hull breach!\n");
			perror("chdir");
			fflush(stdout);
			exit(2);
		}
		else {
			char* dirBuf = NULL;
			size_t size = 500;
			path = NULL;
			return;
		}
	}

	//spawn a child to handle executing the command
	spawnPid = fork();

	//check to see if command provided is a background or foreground and set the bg flag accordingly
	if (strstr(argv[numArgs - 1], "&")) {
		//set the last argument to NULL & decrement the number of arguments in argv so exec works
		argv[numArgs - 1] = NULL;
		numArgs--;
		//if foreground-only mode is off, treat command like a background process
		if (fgOnly == 0)
			bg = 1;
		else
			bg = 0;
	}
	else {
		bg = 0;
	}
	switch(spawnPid) {
		case -1:
			printf("Hull Breach!\n");
			perror("fork failed");
			fflush(stdout);
			exit(1);
			break;
		case 0:

			//register foreground processes only to SIGINT_action function
			if (bg == 0)
				sigaction(SIGINT, &SIGINT_action, NULL);

			//look for input/output redirection operators and store their location(s)
			for (i = 0; i < numArgs; i++) {
				if (strcmp(argv[i], "<") == 0) {
					redirInput = 1;
					redirInLoc = i;
				}
				if (strcmp(argv[i], ">") == 0) {
					redirOutput = 1;
					redirOutLoc = i;
				}
			}

			int devNullWr = open("/dev/null", O_WRONLY);
			int devNullRd = open("/dev/null", O_RDONLY);

			if (redirOutput == 1 && redirInput == 0) {
				fdOut = open(argv[redirOutLoc + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdOut < 0) {
					printf("Hull breach!\n");
					perror("in redirOutput open");
					fflush(stdout);
					exit(3);
				}
				dup2(fdOut, 1);
				
				//if it is a background process, redirect input from /dev/null
				if (bg == 1) {
					dup2(devNullRd, 0);
				}

				if (execlp(*argv, *argv, NULL) < 0) {
					perror("exec failed in redirOutput");
					fflush(stdout);
				}
			}

			else if (redirInput == 1 && redirOutput == 0) {
				fdIn = open(argv[redirInLoc + 1], O_RDONLY);
				if (fdIn < 0) {
					printf("Hull breach!\n");
					perror("in redirInput open");
					fflush(stdout);
					exit(4);
				}
				dup2(fdIn, 0);

				//if it is a background process, redirect output to /dev/null
				if (bg == 1) {
					dup2(devNullWr, 1);
				}

				if (execlp(*argv, *argv, NULL) < 0) {
					perror("exec failed in redirInput");
					fflush(stdout);
				}
			}

			else if (redirOutput == 1 && redirInput == 1) {
				fdIn = open(argv[redirInLoc + 1], O_RDONLY);
				if (fdIn < 0) {
					printf("Hull breach!\n");
					perror("in in redirIn+Out open");
					fflush(stdout);
					exit(4);
				}
				dup2(fdIn, 0);
				fdOut = open(argv[redirOutLoc + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdOut < 0) {
					printf("Hull breach!\n");
					perror("in out redirIn+Out open");
					fflush(stdout);
					exit(3);
				}
				dup2(fdOut, 1);
				if (execlp(*argv, *argv, NULL) < 0) {
					perror("exec failed in redirIn+Out");
					fflush(stdout);
					exit(5);
				}
			}

			//redirect all non-directed bg processes to and from /dev/null
			if (redirOutput == 0 && redirInput == 0 && bg == 1) {
				dup2(devNullWr, 1);
				dup2(devNullRd, 0);
			}

			if (execvp(*argv, argv) < 0) {
				perror("exec failed!");
				fflush(stdout);
				exit(1);
			}
			break;
		default:

			//call waitpid with no options for a foreground process
			if (bg == 0) {
				waitpid(spawnPid, &childExitMethod, 0);

				//determine and report how the foreground child process exited
				//change the return value of status for foreground processes only
				if(WIFEXITED(childExitMethod)) {
					exitTermStatus = WEXITSTATUS(childExitMethod);
					sprintf(exitTermStr, "exit status %d", exitTermStatus);
					fflush(stdout);
				}
				if (WIFSIGNALED(childExitMethod)) {
					printf("The child was terminated by signal: ");
					printf("%d\n", WTERMSIG(childExitMethod));
					exitTermStatus = WTERMSIG(childExitMethod);
					sprintf(exitTermStr, "terminated by signal %d", exitTermStatus);
					fflush(stdout);
				}
			}

			//if child is a background process, report pid and add to the array
			else {
				for (i = 0; i < 500; i++) {
					if (bgPIDs[i] < 0) {
						bgPIDs[i] = spawnPid;
						printf("background pid is %d\n", spawnPid);
						fflush(stdout);
						break;
					}
				}
				
			}
	};
}

//if execvp is to be used, readCommand will return a non-zero positive value which will
//lead to execution of the above function in main(). The non-zero value is the number
//of elements in the array passed to exec

int readCommand(char* args[]) {
	char* dirBuf = NULL;
	size_t size = 500;
	//char* args[512];
	printf(": ");
	fflush(stdout);
	
	char* cmd = NULL;
	char* token = NULL;
	char* pidToken = NULL;
	size_t bufferSize = 0;
	ssize_t nread = -5;
	int i, isBlank = 1;

	//store the entire command in a string
	if ((nread = getline(&cmd, &bufferSize, stdin)) < 0) {
		printf("Hull breach!\n");
		perror("in getline");
		fflush(stdout);
		exit(6);
	}

	//print comments to the terminal but do nothing with the command entered
	if (cmd[0] == '#') {
		printf("%s", cmd);
		fflush(stdout);
		return 0;
	}
	
	if (strstr(cmd, "$$")) {
		char pid[30];

		//convert the pid into a string
		sprintf(pid, "%d", getpid());

		//retrieve the entire string up to the $$ using strtok
		pidToken = strtok(cmd, "$$");

		//concatenate the string pid where $$ used to be
		strcat(pidToken, pid);

		//copy this new command into the cmd variable
		strcpy(cmd, pidToken);

		//add a newline so the next '\0' replacement command works properly
		strcat(cmd, "\n");
	}

	//replace \n with a null terminator
	cmd[strlen(cmd) - 1] = '\0';

	//check if the command has any content
	for (i = 0; i < strlen(cmd); i++) {
		if (cmd[i] != ' ') {
			isBlank = 0;
		}
	}

	//if the command is blank, return and reprompt
	if (isBlank == 1) {
		return 0;
	}

	//retrieve the first token
	token = strtok(cmd, " ");
	i = 0;

	//retrieve all tokens until the end is reached, adding each to an array so that
	//execvp can be properly used
	while (token != NULL) {
		args[i] = token;
		token = strtok(NULL, " ");
		i++;
	}

	//make the last element in the command array NULL (for use with exec)
	args[i + 1] = NULL;

	//exit the shell if the command is exit
	if (strcmp(args[0], "exit") == 0) {
		exit(0);
	}

	//display the exit/term status of the last command if the command is status
	if (strcmp(args[0], "status") == 0) {
		if ((strcmp(args[0], "cd") == 0) || (strcmp(args[0], "exit") == 0))
			return i;
		return -5;
	}

	return i;
}

void main() {

	startShell();

	//set the initial value of status in case it is entered before any processes are run
	strcpy(exitTermStr, "exit status 0");

	SIGINT_action.sa_handler = catchSIGINT;
	SIGTSTP_action.sa_handler = catchSIGTSTP;

	sigfillset(&SIGINT_action.sa_mask);
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGINT_action.sa_flags = 0;

	//set SIGTSTP flag to restart so that getline does not throw interrupted signal call error
	SIGTSTP_action.sa_flags = SA_RESTART;

	ignore_action.sa_handler = SIG_IGN;

	sigaction(SIGINT, &ignore_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	int i, childExitMethod = -5;

	//initialize all entries in background PIDs array to an impossible value
	for (i = 0; i < 500; i++) {
		bgPIDs[i] = -5;
	}

	//start an infinite while loop
	while(1) {
		char* args[512];
		int bgPID, count = 0;

		//loop through array of bg PIDs every iteration, printing status before command prompt
		for (i = 0; i < 500; i++) {

			//if one is found, call waitpid with WNOHANG flag
			if (bgPIDs[i] > 0) {

				//if waitpid terminates a child, reset the array value to -5
				if (waitpid(bgPIDs[i], &childExitMethod, WNOHANG) < 0 && bgPIDs[i] > 0) {
					if(WIFEXITED(childExitMethod)) {
						printf("background pid %d is done: exit value %d\n", 
								bgPIDs[i], WEXITSTATUS(childExitMethod));
								
						fflush(stdout);
					}
					if (WIFSIGNALED(childExitMethod)) {
						printf("background pid %d is done: terminated by signal %d\n",
							       	bgPIDs[i], WTERMSIG(childExitMethod));
						fflush(stdout);
					}
					bgPIDs[i] = -5;
				}
			}
		}

		for (i = 0; i < 512; i++)
			args[i] = NULL;
		if ((count = readCommand(args)) > 0)
			execute(args, count);
		else if (count == -5) {
			//print out return value of last successfully completed foreground non-built in cmd
			//-5 will be the value returned if status is entered as a command
			printf("%s\n", exitTermStr);
			fflush(stdout);
		}
	}
}
