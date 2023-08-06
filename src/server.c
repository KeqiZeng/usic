#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "miniaudio.h"

#include "./config.h"
#include "./utils.h"
#include "./server.h"

static ma_result pipeInit(const char* pipeFolder, const char* pipePath) {
	if (access(pipePath, F_OK) != -1) {
		// if named pipe exists, delete it for initializing
		if (unlink(pipePath) == -1) {
			perror("Failed to delete named pipe");
			return MA_ERROR;
		}
	}

	int err = mkfifo(pipePath, 0666);
	if (err == -1) {
		perror("Failed to create named pipe");
		return MA_ERROR;
	}
	return MA_SUCCESS;
}

static void sendMessageToClient(int fd_fromServer, char* message) {
	ssize_t bytes_written = write(fd_fromServer, message, 256 * sizeof(char));
	if (bytes_written == -1) {
		perror("Failed to send message to client");
	}
}

/*
 * Manage the command from usic client
 */
ma_result cmdManager(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs, int fd_fromServer, bool* loopFlag) {
	ma_result result;
	if (strcmp(args[0], "quit") == 0) {
		// usic quit
		sendMessageToClient(fd_fromServer, "The server of usic quit successfully");
		*loopFlag = false;
	} else if (strcmp(args[0], "play") == 0) {
		// usic play
		result = play(pEngine, pSound, args, numArgs);
		if (result != MA_SUCCESS) {
			perror("Failed to play the music");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "play-list") == 0){
		result = playList(pEngine, pSound, args, numArgs);
		if (result != MA_SUCCESS) {
			perror("Failed to play the list of musics");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "progress") == 0) {
		// usic progress
		char* progress;
		result = getCurrentProgress(pSound, &progress);
		if (result != MA_SUCCESS) {
			perror("Failed to get current playing progress");
			free(args);
			return result;
		}
		ssize_t bytes_written = write(fd_fromServer, progress, 256 * sizeof(char));
		if (bytes_written == -1) {
			perror("Failed to write to named pipe");
			free(args);
			return MA_ERROR;
		}
		free(progress);
	} else if (strcmp(args[0], "play-toggle") == 0) {
		// usic toggle
		result = playToggle(pSound);
		if (result != MA_SUCCESS) {
			perror("Failed to toggle the playing music");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "forward") == 0) {
		// usic forward
		result = moveCursor(pEngine, pSound, CURSOR_DIFF);
		if (result != MA_SUCCESS) {
			perror("Failed to move cursor forward");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "backward") == 0) {
		// usic backward
		result = moveCursor(pEngine, pSound, -CURSOR_DIFF);
		if (result != MA_SUCCESS) {
			perror("Failed to move cursor forward");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "set-cursor") == 0) {
		// usic set-cursor
		if (numArgs < 2) {
			perror("Not enough arguments");
			free(args);
			return MA_INVALID_ARGS;
		}

		result = setCursor(pEngine, pSound, args[1]);
		if (result != MA_SUCCESS && result != MA_INVALID_ARGS) {
			perror("Failed to set cursor");
			free(args);
			return result;
		}
		if (result == MA_INVALID_ARGS) {
			sendMessageToClient(fd_fromServer, "Invalid time");
			result = MA_SUCCESS; // reset result to avoid server collapse
		} else{
			// setCursor successfully
			sendMessageToClient(fd_fromServer, NO_MESSAGE);
		}
	} else if (strcmp(args[0], "volume-up") == 0) {
		// usic volume-up
		result = adjustVolume(pEngine, VOLUME_DIFF);
		if (result != MA_SUCCESS) {
			perror("Failed to up volumen");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "volume-down") == 0) {
		// usic volume-down
		result = adjustVolume(pEngine, -VOLUME_DIFF);
		if (result != MA_SUCCESS) {
			perror("Failed to up volumen");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	} else if (strcmp(args[0], "get-volume") == 0) {
		// usic get-volume
		char* volume;
		result = getVolume(pEngine, &volume);
		if (result != MA_SUCCESS) {
			perror("Failed to get volumen");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, volume);
		free(volume);
	} else if (strcmp(args[0], "single-loop-on") == 0) {
		// usic single-loop-on
		ma_sound_set_looping(pSound, (ma_bool32)true);
		sendMessageToClient(fd_fromServer, "single-loop: on");
	} else if (strcmp(args[0], "single-loop-off") == 0) {
		// usic single-loop-off
		ma_sound_set_looping(pSound, (ma_bool32)false);
		sendMessageToClient(fd_fromServer, "single-loop: off");
	} else if (strcmp(args[0], "mute-toggle") == 0) {
		result = muteToggle(pEngine);
		if (result != MA_SUCCESS) {
			perror("Failed to toggle mute");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, NO_MESSAGE);
	}
	else {
		char* message = (char*) malloc((strlen(args[0])+strlen("Unknown command")+1) * sizeof(char));
		if (message == NULL) {
			perror("Failed to allocate memory for message");
			free(args);
			return MA_OUT_OF_MEMORY;
		}
		message[0] = '\0';
		strcat(message, "Unknown command: ");
		strcat(message, args[0]);
		strcat(message, "\0");
		sendMessageToClient(fd_fromServer, message);
		free(message);
	}
	free(args);
	return MA_SUCCESS;
}

/*
 * Play a music immediately
 */
ma_result play(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs) {
	ma_result result;
	if (numArgs < 2) {
		perror("Not enough arguments");
		return MA_INVALID_ARGS;
	}
	char* music = args[1];
	ma_sound_uninit(pSound);
	result = ma_sound_init_from_file(pEngine, music, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, pSound);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize sound");
		return result;
	}
	ma_sound_start(pSound);
	if (numArgs >= 3 && strcmp(args[2], "--single-loop") == 0) {
		ma_sound_set_looping(pSound, (ma_bool32)true);
	}
	return MA_SUCCESS;
}

/*
 * Play a list of musics one by one
 */
ma_result playList(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs) {
	ma_result result;
	if (numArgs < 2) {
		perror("Not enough arguments");
		return MA_INVALID_ARGS;
	}
	const char* musicList = args[1];
	FILE* fp = fopen(musicList, "r");
	if (fp == NULL) {
		perror("Failed to open music list file");
		return MA_ERROR;
	}
	char music[512];
	if (fgets(music, sizeof(music), fp) == NULL || music == NULL) {
		perror("Failed to read a music from the list file");
		fclose(fp);
		return MA_ERROR;
	}
	size_t newLinePos = strcspn(music, "\n");
	music[newLinePos] = '\0';
	ma_sound_uninit(pSound);
	result = ma_sound_init_from_file(pEngine, music, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, pSound);
	if (result != MA_SUCCESS) {
		perror(music);
		perror("Failed to initialize sound");
		fclose(fp);
		return result;
	}
	ma_sound_start(pSound);
	fclose(fp);
	return MA_SUCCESS;
}

ma_result server(ma_engine* pEngine, ma_sound* pSound) {
	ma_result result;

	char* toServer = (char*) malloc((strlen(RUNTIMEPATH) + strlen(PIPE_TO_SERVER) + 1) * sizeof(char));
	if (toServer == NULL) {
		perror("Failed to allocate memory for toServer");
		return MA_OUT_OF_MEMORY;
	}
	setPipePath(PIPE_TO_SERVER, toServer);

	// Initialize the toServer pipe
	result = pipeInit(RUNTIMEPATH, toServer);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize to_server pipe");
		free(toServer);
		return result;
	}
	
	// Open the toServer as O_NONBLOCK
	int fd_toServer = open(toServer, O_RDONLY | O_NONBLOCK);
	if (fd_toServer == -1) {
		perror("Failed to open to_server pipe");
		free(toServer);
		return MA_ERROR;
	}

	char* fromServer = (char*) malloc((strlen(RUNTIMEPATH) + strlen(PIPE_FROM_SERVER) + 1) * sizeof(char));
	if (fromServer == NULL) {
		perror("Failed to allocate memory for fromServer");
		free(toServer);
		close(fd_toServer);
		return MA_OUT_OF_MEMORY;
	}
	setPipePath(PIPE_FROM_SERVER, fromServer);

	// Initialize the fromServer pipe
	result = pipeInit(RUNTIMEPATH, fromServer);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize from_server pipe");
		free(toServer);
		close(fd_toServer);
		free(fromServer);
		return result;
	}
	
	// Open the fromServer
	int fd_fromServer = open(fromServer, O_WRONLY);
	if (fd_fromServer == -1) {
		perror("Failed to open from_server pipe");
		free(toServer);
		close(fd_toServer);
		free(fromServer);
		return MA_ERROR;
	}

	result = ma_engine_init(NULL, pEngine);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize engine");
		free(toServer);
		close(fd_toServer);
		free(fromServer);
		close(fd_fromServer);
		return result;
	}

	result = ma_sound_init_from_data_source(pEngine, NULL, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, pSound);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize sound");
		free(toServer);
		close(fd_toServer);
		free(fromServer);
		close(fd_fromServer);
		return result;
	}

	char buf[256];
	ssize_t bytes_readed = 0;
	bool loopFlag = true;
	while (loopFlag) {
		bytes_readed = read(fd_toServer, buf, sizeof(buf));
		if (bytes_readed > 0) {
			buf[bytes_readed] = '\0';
			
			int numArgs = 0;
			char** args = argsParser(buf, &numArgs);

			result = cmdManager(pEngine, pSound, args, numArgs, fd_fromServer, &loopFlag);
			if (result != MA_SUCCESS) {
				perror("Failed to manage command from client");
				free(toServer);
				close(fd_toServer);
				free(fromServer);
				close(fd_fromServer);
				return result;
			}
		} else {
			if (pSound->pDataSource != NULL) {
				if ((bool)ma_sound_is_playing(pSound)) {
					if ((bool)ma_sound_at_end(pSound)) {
						ma_sound_uninit(pSound);
						result = ma_sound_init_from_file(pEngine, "/Users/ketch/Music/Music/Media.localized/Music/Unknown Artist/Unknown Album/赵雷-我记得.wav", MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, pSound);
						if (result != MA_SUCCESS) {
							free(toServer);
							close(fd_toServer);
							free(fromServer);
							close(fd_fromServer);
							return result;
						}
						ma_sound_start(pSound);
					}
				}
			}
		}
	}

	close(fd_toServer);
	close(fd_fromServer);

	unlink(toServer);
	unlink(fromServer);

	free(toServer);
	free(fromServer);

	ma_engine_uninit(pEngine);
	ma_sound_uninit(pSound);

	return MA_SUCCESS;
}
