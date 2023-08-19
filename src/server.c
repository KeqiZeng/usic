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
	ssize_t bytes_written = write(fd_fromServer, message, 512 * sizeof(char));
	if (bytes_written == -1) {
		perror("Failed to send message to client");
	}
}

/*
 * Manage the command from usic client
 */
ma_result cmdManager(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs,
                     int fd_fromServer, bool* loopFlag, MusicNode* currentMusicNode,
                     MusicList* defaultMusicList, MusicList* laterPlayed) {
	ma_result result;
	if (strcmp(args[0], "quit") == 0) {
		// usic quit
		sendMessageToClient(fd_fromServer, "The server of usic quit successfully");
		*loopFlag = false;
	} else if (strcmp(args[0], "play") == 0) {
		// usic play
		if (numArgs < 2) {
			sendMessageToClient(fd_fromServer, "Need an audio file to play");
		} else {
			if (pSound->pDataSource != NULL) {
        MusicNode* finishedMusicNode = createMusicNode(currentMusicNode->music);
        if (finishedMusicNode == NULL) {
          perror("Failed to change the music");
          free(args);
          return MA_ERROR;
        }
        queueMusic(defaultMusicList, finishedMusicNode);
        // free(finishedMusicNode);
      }
			result = play(pEngine, pSound, args[1], currentMusicNode);
			if (result != MA_SUCCESS) {
				perror("Failed to play the music");
				free(args);
				return result;
			} else {
				sendMessageToClient(fd_fromServer, NO_MESSAGE);
			}
		}
	} else if (strcmp(args[0], "play-list") == 0){
		if (numArgs < 2) {
			sendMessageToClient(fd_fromServer, "Need a musicList file to play");
		} else {
			result = playList(pEngine, pSound, args, numArgs, currentMusicNode, defaultMusicList, laterPlayed);
			if (result != MA_SUCCESS) {
				perror("Failed to play the list of musics");
				free(args);
				return result;
			} else {
				sendMessageToClient(fd_fromServer, NO_MESSAGE);
			}
		}
	} else if (strcmp(args[0], "progress") == 0) {
		// usic progress
		char* progress;
		result = getCurrentProgress(pSound, &progress);
		if (result != MA_SUCCESS) {
			perror("Failed to get current playing progress");
			free(args);
			return result;
		}
		sendMessageToClient(fd_fromServer, progress);
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
			sendMessageToClient(fd_fromServer, "Need a time to set the cursor");
		} else {
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
	} else if (strcmp(args[0], "next") == 0) {
		result = next(pSound);
		if (result != MA_SUCCESS) {
			perror("Failed to jump to the next music");
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

ma_result play(ma_engine* pEngine, ma_sound* pSound, char* music, MusicNode* currentMusicNode) {
	ma_result result;
	currentMusicNode->music = strdup(music);
	ma_sound_uninit(pSound);
	result = ma_sound_init_from_file(pEngine, music, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, pSound);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize the sound");
		return result;
	}
	result = ma_sound_start(pSound);
	if (result != MA_SUCCESS) {
		perror("Failed to play the sound");
		return result;
	}
	return MA_SUCCESS;
}

// ma_result playNext(ma_engine* pEngine, ma_sound* pSound, ma_sound* pSoundNext, char* music, MusicNode* currentMusicNode) {
// 	ma_result result;
// 	currentMusicNode->music = strdup(music);
// 	ma_sound_uninit(pSound);
// 	result = ma_sound_init_copy(pEngine, pSoundNext, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, pSound);
// 	if (result != MA_SUCCESS) {
// 		perror("Failed to copy pSoundNext to pSound");
// 		return result;
// 	}
// 	result = ma_sound_start(pSound);
// 	if (result != MA_SUCCESS) {
// 		perror("Failed to play the music");
// 		return result;
// 	}
// 	return MA_SUCCESS;
// }

// initialize nextMusicNode and pSoundNext
MusicNode* loadNextMusic(MusicList* defaultMusicList, MusicList* laterPlayed) {
	MusicNode* nextMusicNode = NULL;
	if (laterPlayed->numMusics == 0) {
    // load next music from defaultMusicList
		nextMusicNode = dequeueMusic(defaultMusicList);
		if (nextMusicNode == NULL) {
			perror("Failed to get the next music from defaultMusicList");
		}
	} else {
    // load next music from laterPlayed
		nextMusicNode = dequeueMusic(laterPlayed);
		if (nextMusicNode == NULL) {
			perror("Failed to get the next music from laterPlayed musicList");
		}
	}
  return nextMusicNode;
}

/*
 * Play a list of musics one by one, from a specific one
 * play-list playList specificOne.
 */
ma_result playList(ma_engine* pEngine, ma_sound* pSound, char** args, int numArgs, MusicNode* currentMusicNode, MusicList* defaultMusicList, MusicList* laterPlayed) {
	ma_result result;
	if (numArgs < 2) {
		perror("Not enough arguments");
		return MA_INVALID_ARGS;
	}

	// Load the playList to defaultMusicList
	resetMusicList(defaultMusicList);
	const char* playList = args[1];
	FILE* fp = fopen(playList, "r");
	if (fp == NULL) {
		perror("Failed to open music list file");
		return MA_ERROR;
	}

	// check if the musicToPlay argument is given
	char* musicToPlay = NULL;
	if (numArgs > 2) {
		musicToPlay = args[2];
	}
	char music[512];
	while (fgets(music, sizeof(music), fp)) {
		size_t newLinePos = strcspn(music, "\n");
		music[newLinePos] = '\0';
		if (musicToPlay != NULL && strcmp(music, musicToPlay) == 0) {
			continue;
		} 
		MusicNode* musicNode = createMusicNode(music);
		if (musicNode == NULL) {
			perror("Failed to allocate memory for MusicNode while initializing the defaultMusicList");
			fclose(fp);
			return MA_OUT_OF_MEMORY;
		}
		queueMusic(defaultMusicList, musicNode);
	}

	fclose(fp);
	if (musicToPlay == NULL) {
    MusicNode* musicToPlayNode = dequeueMusic(defaultMusicList);
		if (musicToPlayNode == NULL) {
			perror("Failed to dequeue a music from the defaultMusicList");
			return MA_ERROR;
		}
		musicToPlay = musicToPlayNode->music;
		free(musicToPlayNode);
	}
	result = play(pEngine, pSound, musicToPlay, currentMusicNode);
	if (result != MA_SUCCESS) {
		perror("Failed to play the music");
		return result;
	}
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
	
	// Open the toServer pipe as O_NONBLOCK
	int fd_toServer = open(toServer, O_RDONLY | O_NONBLOCK);
	if (fd_toServer == -1) {
		perror("Failed to open to_server pipe");
		free(toServer);
		unlink(toServer);
		return MA_ERROR;
	}

	char* fromServer = (char*) malloc((strlen(RUNTIMEPATH) + strlen(PIPE_FROM_SERVER) + 1) * sizeof(char));
	if (fromServer == NULL) {
		perror("Failed to allocate memory for fromServer");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		return MA_OUT_OF_MEMORY;
	}
	setPipePath(PIPE_FROM_SERVER, fromServer);

	// Initialize the fromServer pipe
	result = pipeInit(RUNTIMEPATH, fromServer);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize from_server pipe");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		return result;
	}
	
	// Open the fromServer
	int fd_fromServer = open(fromServer, O_WRONLY);
	if (fd_fromServer == -1) {
		perror("Failed to open from_server pipe");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		return MA_ERROR;
	}

	// Initialize ma_engine
	result = ma_engine_init(NULL, pEngine);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize engine");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		close(fd_fromServer);
		return result;
	}

	// Initialize a blank ma_sound
	result = ma_sound_init_from_data_source(pEngine, NULL, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, pSound);
	if (result != MA_SUCCESS) {
		perror("Failed to initialize initial sound");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		close(fd_fromServer);
		ma_engine_uninit(pEngine);
		return result;
	}

	// Initialize a default musicList
	MusicList* defaultMusicList = createMusicList();
	if (defaultMusicList == NULL) {
		perror("Failed to allocate memory for defaultMusicList");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		close(fd_fromServer);
		ma_engine_uninit(pEngine);
		ma_sound_uninit(pSound);
		return MA_OUT_OF_MEMORY;
	}

	// Initialize a laterPlayed musicList
	MusicList* laterPlayed = createMusicList();
	if (laterPlayed == NULL) {
		perror("Failed to allocate memory for laterPlayed musicList");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		close(fd_fromServer);
		ma_engine_uninit(pEngine);
		ma_sound_uninit(pSound);
		freeMusicList(defaultMusicList);
		return MA_OUT_OF_MEMORY;
	}

	// Initialize a musicNode for the current played music
	MusicNode* currentMusicNode = createMusicNode("\0");
	if (currentMusicNode == NULL) {
		perror("Failed to allocate memory for current played music");
		free(toServer);
		unlink(toServer);
		close(fd_toServer);
		free(fromServer);
		unlink(fromServer);
		close(fd_fromServer);
		ma_engine_uninit(pEngine);
		ma_sound_uninit(pSound);
		freeMusicList(defaultMusicList);
		freeMusicList(laterPlayed);
		free(currentMusicNode);
		return MA_OUT_OF_MEMORY;
	}

	char buf[512];
	ssize_t bytes_readed = 0;
	bool loopFlag = true;
	MusicNode* finishedMusicNode = NULL;
	while (loopFlag) {
    // get command from client
		bytes_readed = read(fd_toServer, buf, sizeof(buf));
		if (bytes_readed > 0) {
			buf[bytes_readed] = '\0';
			
			int numArgs = 0;
			char** args = argsParser(buf, &numArgs);

			result = cmdManager(pEngine, pSound, args, numArgs, fd_fromServer, &loopFlag, currentMusicNode, defaultMusicList, laterPlayed);
			if (result != MA_SUCCESS) {
				perror("Failed to manage command from client");
				free(toServer);
				unlink(toServer);
				close(fd_toServer);
				free(fromServer);
				unlink(fromServer);
				close(fd_fromServer);
				ma_engine_uninit(pEngine);
				ma_sound_uninit(pSound);
				freeMusicList(defaultMusicList);
				freeMusicList(laterPlayed);
				free(currentMusicNode);
				return result;
			}
		} else {
			if (pSound->pDataSource != NULL) {
				if ((bool)ma_sound_at_end(pSound)) {
					finishedMusicNode = createMusicNode(currentMusicNode->music);
					if (finishedMusicNode == NULL) {
						perror("Failed to allocate memory for finishedMusicNode");
						free(toServer);
						unlink(toServer);
						close(fd_toServer);
						free(fromServer);
						unlink(fromServer);
						close(fd_fromServer);
						ma_engine_uninit(pEngine);
						ma_sound_uninit(pSound);
						freeMusicList(defaultMusicList);
						freeMusicList(laterPlayed);
						free(currentMusicNode);
						return MA_OUT_OF_MEMORY;
					}
					printf("finishedMusic: %s\n", finishedMusicNode->music);
					queueMusic(defaultMusicList, finishedMusicNode);
					// printMusicList(defaultMusicList);
					MusicNode* nextMusicNode = loadNextMusic(defaultMusicList, laterPlayed);
          if (nextMusicNode == NULL) {
            perror("Failed to load next music");
						free(toServer);
						unlink(toServer);
						close(fd_toServer);
						free(fromServer);
						unlink(fromServer);
						close(fd_fromServer);
						ma_engine_uninit(pEngine);
						ma_sound_uninit(pSound);
						freeMusicList(defaultMusicList);
						freeMusicList(laterPlayed);
						free(currentMusicNode);
            return MA_ERROR;
          }
					printf("nextMusic %s\n", nextMusicNode->music);
					result = play(pEngine, pSound, nextMusicNode->music, currentMusicNode);
					if (result != MA_SUCCESS) {
						perror("Failed to play the next music");
						free(toServer);
						unlink(toServer);
						close(fd_toServer);
						free(fromServer);
						unlink(fromServer);
						close(fd_fromServer);
						ma_engine_uninit(pEngine);
						ma_sound_uninit(pSound);
						freeMusicList(defaultMusicList);
						freeMusicList(laterPlayed);
						free(currentMusicNode);
						free(finishedMusicNode);
            free(nextMusicNode);
						return result;
					}
          free(nextMusicNode);
				}
        // else {
					// if (loadNext) {
					// 	ma_sound_uninit(pSoundNext);
					// 	nextMusicNode = loadNextMusic(defaultMusicList, laterPlayed);
					// 	if (result != MA_SUCCESS) {
					// 		perror("Failed to load the next music");
					// 		free(toServer);
					// 		unlink(toServer);
					// 		close(fd_toServer);
					// 		free(fromServer);
					// 		unlink(fromServer);
					// 		close(fd_fromServer);
					// 		ma_engine_uninit(pEngine);
					// 		ma_sound_uninit(pSound);
					// 		ma_sound_uninit(pSoundNext);
					// 		free(pSoundNext);
					// 		freeMusicList(defaultMusicList);
					// 		freeMusicList(laterPlayed);
					// 		free(currentMusicNode);
					// 		if (finishedMusicNode != NULL) {
					// 			free(finishedMusicNode);
					// 		}
					// 		return result;
					// 	}
					// 	loadNext = false;
					// }
				// }
			}
		}
	}

	free(toServer);
	unlink(toServer);
	close(fd_toServer);
	free(fromServer);
	unlink(fromServer);
	close(fd_fromServer);
	ma_engine_uninit(pEngine);
	ma_sound_uninit(pSound);
	freeMusicList(defaultMusicList);
	freeMusicList(laterPlayed);
	free(currentMusicNode);
	free(finishedMusicNode);
	return result;
}
