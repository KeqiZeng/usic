#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "miniaudio.h"

#include "./config.h"
#include "./utils.h"
#include "./server.h"
#include "./client.h"

int main(int argc, char* argv[]) {
	if (argc == 1) {
		ma_engine* pEngine = (ma_engine*) malloc(sizeof(ma_engine));
		ma_sound* pSound = (ma_sound*) malloc(sizeof(ma_sound));
		
		// Fork a new process and run child process in background
		pid_t pid = fork();
		if (pid < 0) {
			// Forking failed
			perror("Forking child process failed");
			return 1;
		} else if (pid > 0) {
			// Parent process, exit
			return 0;
		}

		setupRuntime();

		int err = server(pEngine, pSound);
		if (err != 0) {
			perror("Server shuts down due to RuntimeError");
		}
		free(pEngine);
		free(pSound);
	} else {
		int err = client(argc, argv);
		if (err != 0) {
			perror("Failed to communicate with server");
		}
	}
  return 0;
}
