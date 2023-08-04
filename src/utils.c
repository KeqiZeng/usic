#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#include "./config.h"
#include "./utils.h"

/*
 * Convert seconds(int) to time(string like mm:ss).
 */
static char* secondsToTimeStr(int seconds) {
	if (seconds < 0) {
		perror("Error: Input must be a non-negative integer");
		return NULL;
	}

	int min = seconds / 60;
	int sec = seconds % 60;

	// Calculate the required length for the result string
	int strLength = snprintf(NULL, 0, "%d:%02d", min, sec) + 1;

	// Allocate memory for the result string
	char *result = (char*) malloc(strLength * sizeof(char));
	if (result == NULL) {
		perror("Error: Memory allocation failed");
		return NULL;
	}
	
	// Convert
	snprintf(result, strLength, "%d:%02d", min, sec);

	return result;
}

/*
 * Convert time(string like mm:ss) to seconds(int).
 */
static int timeStrToSeconds(const char* timeString) {
	int minutes = 0;
	int seconds = 0;

	// Convert
	if (sscanf(timeString, "%d:%d", &minutes, &seconds) == 2) {

		// minitues should not be less than 0
		if (minutes < 0) {
			return -1;
		}

		// seconds should be 0-60
		if (seconds < 0 || seconds > 60) {
			return -1;
		}
		return minutes * 60 + seconds;
	} else {
			return -1; // indicates failed conversion
	}
}

int serverIsRunning() {
	char buffer[256];
	int running = 0;

	FILE* fp = popen("ps -e -o comm=", "r");
	if (fp == NULL) {
		perror("Failed to execute ps command");
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		// Remove the trailing newline character
		buffer[strcspn(buffer, "\n")] = '\0';

		if (strcmp(buffer, "usic server") == 0) {
				running = 1;
				break;
		}
	}

	pclose(fp);

	return running;
}

/*
 * Redirect the standard error stream
 */
void redirectStderr() {
	char* errorLog = (char*) malloc(strlen(RUNTIMEPATH) + 10);
	strcpy(errorLog, RUNTIMEPATH);
	strcat(errorLog, "error.log");
	strcat(errorLog, "\0");

	if (access(errorLog, F_OK) != -1) {
		if (remove(errorLog) != 0) {
        perror("Failed to delete the last errorLog");
				free(errorLog);
        return;
    }
	}

	FILE* errorLogFile = freopen(errorLog, "w", stderr);
	if (errorLogFile == NULL) {
		perror("Failed to open errorLogFile");
		free(errorLog);
		return;
	}
	free(errorLog);
}

/*
 * Setup the runtime environment
 */
void setupRuntime() {
	struct stat st;
	if (stat(RUNTIMEPATH, &st) == -1) {
		// runtime folder does not exist, create it
		if (mkdir(RUNTIMEPATH, 0777) == -1) {
			perror("Failed to create folder");
			return;
		}
	}

	fclose(stdin);
	fclose(stdout);
	
	redirectStderr();
}

/*
 * Set the path of the named pipe
 */
void setPipePath(const char* pipeName, char* pipePath) {
	strcpy(pipePath, RUNTIMEPATH);
	strcat(pipePath, pipeName);
	strcat(pipePath, "\0");
} 

static bool hasSpace(const char* str) {
    while (*str) {
        if (*str == ' ') {
            return true;
        }
        str++;
    }
    return false;
}

char* addQuotesIfNeeded(const char* str) {
	if (hasSpace(str)) {
    size_t len = strlen(str);
    char* newStr = (char*)malloc(len + 3); // Add 3 for the quotes and null terminator

    if (newStr == NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }

    newStr[0] = '"';
    strcpy(newStr + 1, str);
    newStr[len + 1] = '"';
    newStr[len + 2] = '\0';

    return newStr;
	} else {
		return (char*) str;
	}
}

/*
 * Parse a string, which separated by " ", into several arguments
 */
char** argsParser(const char* str, int* numArgs) {
	char** args = NULL;
	int argCount = 0;
	int maxArgs = 10; // Initial size for the tokens array, can be adjusted based on expected number of tokens

	args = (char**)malloc(maxArgs * sizeof(char*));
	if (args == NULL) {
		perror("Failed to allocate memory");
		return NULL;
	}

	int i = 0;
	int inQuote = 0; // State variable to track whether we are inside a quote

	while (*str != '\0') {
		if (*str == ' ' && !inQuote) {
			// Found a space outside of a quote, terminate the token and move to the next one
			if (i > 0) {
					args[argCount] = (char*)malloc(i + 1);
					strncpy(args[argCount], str - i, i);
					args[argCount][i] = '\0';
					argCount++;
					i = 0;
			}
		} else if (*str == '"') {
			// Toggle the inQuote state when we encounter a double quote
			inQuote = !inQuote;
			if (!inQuote && i > 0) {
					args[argCount] = (char*)malloc(i + 1);
					strncpy(args[argCount], str - i, i);
					args[argCount][i] = '\0';
					argCount++;
					i = 0;
			}
		} else {
			// Add the character to the token
			i++;
		}

		str++;
		
		// Resize the tokens array if needed
		if (argCount >= maxArgs) {
			maxArgs *= 2;
			args = (char**)realloc(args, maxArgs * sizeof(char*));
			if (args == NULL) {
					perror("Failed to allocate memory");
					return NULL;
			}
		}
	}

	// Process the last token
	if (i > 0) {
		args[argCount] = (char*)malloc(i + 1);
		strncpy(args[argCount], str - i, i);
		args[argCount][i] = '\0';
		argCount++;
	}

	// Resize the tokens array to the exact size
	args = (char**)realloc(args, argCount * sizeof(char*));
	if (args == NULL) {
		perror("Failed to allocate memory");
		return NULL;
	}
	
	
	for (int i = 0; i < argCount; i++) {
		char* arg = args[i];

		// Remove double quotes from the token, if present
		if (arg[0] == '"' && arg[strlen(arg) - 1] == '"') {
			arg[strlen(arg) - 1] = '\0'; // Remove the closing double quote
			arg++; // Move the pointer one position ahead to remove the opening double quote
		}
	}

	*numArgs = argCount;
	return args;
}

/*
 * Pause or resume the playing sound.
 */
ma_result playToggle(ma_sound* pSound) {
	ma_result result;
	if ((bool)ma_sound_is_playing(pSound)) {
		// if is_playing, pause
		result = ma_sound_stop(pSound);
		if (result != MA_SUCCESS) {
			return result;
		}
	} else {
		// else resume
		result = ma_sound_start(pSound);
		if (result != MA_SUCCESS) {
			return result;
		}
	}
	return result;
}

/*
 * Plot the current progress with bar and time, like:
 *
 * ----------------------o-------------------------- current/length
 *      ^                ^            ^
 *	has_been_played     now     will_be_played
 *
 * BAR_LENGTH is used to control the length of bar, default value is 100,
 * which means that an element ("-" or "o") indicates 2% of the playing progress.
 */
ma_result getCurrentProgress(ma_sound* pSound, char** progress) {
    ma_result result;

    float cursor = 0.0F;
    float length = 0.0F;

    // Get the current cursor position
    result = ma_sound_get_cursor_in_seconds(pSound, &cursor);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Get the total length of the sound
    result = ma_sound_get_length_in_seconds(pSound, &length);
    if (result != MA_SUCCESS) {
        return result;
    }

    cursor = round(cursor);
    length = round(length);

    char* cursor_str = secondsToTimeStr(cursor);
    char* length_str = secondsToTimeStr(length);

    // BAR_LENGTH should not be greater than 2^8 = 256
    ma_uint8 i = 0;
    size_t progressSize = BAR_LENGTH + strlen(cursor_str) + strlen(length_str) + 2;
    *progress = (char*) malloc(progressSize);
    if (*progress == NULL) {
        perror("Failed to allocate memory");
        free(cursor_str);
        free(length_str);
        return MA_OUT_OF_MEMORY;
    }

    // fill the has_been_played part of the bar
    float percent = cursor / length;
    int n_played = (int)(percent * 100) / (int)(100 / BAR_LENGTH);
    for (i = 0; i < n_played; ++i) {
        (*progress)[i] = '-';
    }

    // set the current cursor in the bar
    (*progress)[i] = 'o';
    i++;

    // fill the will_be_played part of the bar
    for (; i < BAR_LENGTH; ++i) {
        (*progress)[i] = '-';
    }

    (*progress)[i] = ' ';

    // Append the cursor_str and length_str to the progress
    strcat(*progress, cursor_str);
    strcat(*progress, "/");
    strcat(*progress, length_str);

    free(cursor_str);
    free(length_str);

    return MA_SUCCESS;
}

/*
 * Move the cursor in the playing progress, the parameter "seconds" indicates the step length. 
 * if seconds > 0, move cursor forward; else move cursor backward.
 */
ma_result moveCursor(ma_engine* pEngine, ma_sound* pSound, int seconds) {
	ma_result result;

	// Get current cursor and total length
	ma_uint64 cursor = 0;
	ma_uint64 length = 0;
	result = ma_sound_get_cursor_in_pcm_frames(pSound, &cursor);
	if (result != MA_SUCCESS) {
    return result;
	}
	result = ma_sound_get_length_in_pcm_frames(pSound, &length);
	if (result != MA_SUCCESS) {
    return result;
	}
	
	// Calculate the position of the cursor after moving
	ma_uint64 movedCursor = 0;
	if (seconds > 0) {
		int newCursor = cursor + (ma_engine_get_sample_rate(pEngine) * seconds);
		movedCursor = newCursor < length? newCursor : length;
	} else {
		seconds = -seconds;
		int newCursor = cursor - (ma_engine_get_sample_rate(pEngine) * seconds);
		movedCursor = newCursor > 0? newCursor : 0;
	}

	// Move cursor
	result = ma_sound_seek_to_pcm_frame(pSound, movedCursor);
	return result;
}

/*
 * Set the cursor in the playing progress, the parameter "time" is a string like "mm:ss",
 * which indicates the destination.
 */
ma_result setCursor(ma_engine* pEngine, ma_sound* pSound, const char* time) {
	ma_result result;
	float length = 0;

	// Convert the time string to seconds
	int destination = timeStrToSeconds(time);
	
	// Error handling
	if (destination == -1) {
		return MA_INVALID_ARGS;
	}

	// Get the total length of the sound
	result = ma_sound_get_length_in_seconds(pSound, &length);
	if (result != MA_SUCCESS) {
    return result;
	}

	int lenInt = (int) length; // must be rounded down

	// Calculate the destination
	destination = destination < 0? 0 : destination > lenInt? lenInt : destination;

	result = ma_sound_seek_to_pcm_frame(pSound, (ma_engine_get_sample_rate(pEngine) * destination));
	if (result != MA_SUCCESS) {
    return result;
	}

	return result;
}

/*
 * Adjust the volume of the engine, which should be 0.0-1.0.
 * The parameter "diff" indicates the step length.
 */
ma_result adjustVolume(ma_engine* pEngine, float diff) {
  ma_result result;
	ma_device* pDevice = ma_engine_get_device(pEngine);

	// Get current volume of the engine
	float currentVolume = 0.0;
	result = ma_device_get_master_volume(pDevice, &currentVolume);
	if (result != MA_SUCCESS) {
		return result;
	}

	// Calculate the new volume
	float newVolume = currentVolume + diff;
	newVolume = newVolume < 0.0F? 0.0F : newVolume > 1.0F? 1.0F : newVolume;

	// Set volume
	result = ma_device_set_master_volume(pDevice, newVolume);
	if (result != MA_SUCCESS) {
		return result;
	}
	return result;
}

/*
 * Get the volume of the engine and print it as 0-100%
 */
ma_result getVolume(ma_engine* pEngine, char** volume) {
  ma_result result;
	ma_device* pDevice = ma_engine_get_device(pEngine);
	float currentVolume = 0.0;
	result = ma_device_get_master_volume(pDevice, &currentVolume);
	if (result != MA_SUCCESS) {
		return result;
	}
	*volume = (char*) malloc(15 * sizeof(char*));
	// strcat(volume, )
	snprintf(*volume, 15, "Volume: %.1f%%", currentVolume * 100); // '%' need to escape in printf
	return result;
}

ma_result muteToggle(ma_engine* pEngine) {
  ma_result result;
	ma_device* pDevice = ma_engine_get_device(pEngine);
	float currentVolume = 0.0;
	result = ma_device_get_master_volume(pDevice, &currentVolume);
	if (result != MA_SUCCESS) {
		return result;
	}
	if (currentVolume == 0.0F) {
		result = ma_device_set_master_volume(pDevice, 1.0F);
		if (result != MA_SUCCESS) {
			return result;
		}
	} else {
		result = ma_device_set_master_volume(pDevice, 0.0F);
		if (result != MA_SUCCESS) {
			return result;
		}
	}
	return MA_SUCCESS;
}
