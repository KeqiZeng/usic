#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "./config.h"
#include "./utils.h"
#include "./client.h"

/*
 * Concatenate all arguments into a string
 */
static char* concatenateArgs(int argc, char* argv[]) {
	size_t total_length = 0;
	for (int i = 1; i < argc; i++) {
		total_length += strlen(argv[i]) + 1; // plus 1 for separator
	}

	char* concatenated_args = (char*)malloc(total_length);
	if (concatenated_args == NULL) {
		perror("Memory allocation failed");
		return NULL;
	}

	concatenated_args[0] = '\0';
	for (int i = 1; i < argc; i++) {
		char* str = addQuotesIfNeeded(argv[i]);
		strcat(concatenated_args, str);
		strcat(concatenated_args, " "); // set separator
	}

	// remove the last space
	concatenated_args[strlen(concatenated_args) - 1] = '\0';
	return concatenated_args;
}

int client(int argc, char* argv[]) {
	// redirectStderr();
	
	char* toServer = (char*) malloc((strlen(RUNTIMEPATH) + strlen(PIPE_TO_SERVER) + 1) * sizeof(char));
	setPipePath(PIPE_TO_SERVER, toServer);
	int fd_toServer = open(toServer, O_WRONLY);
	if (fd_toServer == -1) {
		perror("Failed to open named pipe");
		return 1;
	}

	char* fromServer = (char*) malloc((strlen(RUNTIMEPATH) + strlen(PIPE_FROM_SERVER) + 1) * sizeof(char));
	setPipePath(PIPE_FROM_SERVER, fromServer);
	int fd_fromServer = open(fromServer, O_RDONLY);
	if (fd_fromServer == -1) {
		perror("Failed to open named pipe");
		return 1;
	}

	char* concatenated_args = concatenateArgs(argc, argv);
	if (concatenated_args == NULL) {
		perror("Failed to concatenate arguments");
		return 1;
	}

  // write the concatenated_args to named pipe
	ssize_t bytes_written = write(fd_toServer, concatenated_args, strlen(concatenated_args));
	if (bytes_written == -1) {
		perror("Failed to write to named pipe");
		return 1;
	}

	char buf[256];
	ssize_t bytes_readed = read(fd_fromServer, buf, sizeof(buf));
	if (bytes_readed > 0 && strcmp(buf, NO_MESSAGE) != 0) {
		printf("%s\n", buf);
	}

	// close the named pipes and free memory
	close(fd_toServer);
	close(fd_fromServer);
	free(concatenated_args);

	return 0;
}
