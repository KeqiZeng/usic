#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "musicList.h"

MusicList* createMusicList() {
	MusicList* newMusicList = (MusicList*) malloc(sizeof(MusicList));
	if (newMusicList == NULL) {
		return NULL;
	}
	newMusicList->head = NULL;
	newMusicList->tail = NULL;
	newMusicList->numMusics = 0;
	return newMusicList;
}

MusicNode* createMusicNode(char* music) {
	MusicNode* newMusicNode = (MusicNode*) malloc(sizeof(MusicNode));
	if (newMusicNode == NULL) {
		return NULL;
	}
	newMusicNode->music = strdup(music);
	newMusicNode->prev = NULL;
	newMusicNode->next = NULL;
	return newMusicNode;
}

void queueMusic(MusicList* musicList, MusicNode* music) {
	if (musicList->head == NULL && musicList->tail == NULL) {
		musicList->head = music;
		musicList->tail = music;
		musicList->numMusics += 1;
	} else {
		musicList->tail->next = music;
		music->prev = musicList->tail;
		musicList->tail = music;
		musicList->numMusics += 1;
	}
}

MusicNode* dequeueMusic(MusicList* musicList) {
	if (musicList->head == NULL && musicList->tail == NULL) {
		return NULL;
	}
	MusicNode* result = musicList->head;
	musicList->head = musicList->head->next;
	if (musicList->head == NULL) {
		musicList->tail = NULL;
	} else {
		result->next = NULL;
	}
	musicList->numMusics -= 1;
	return result;
}

MusicNode* popMusic(MusicList* musicList) {
	if (musicList->head == NULL && musicList->tail == NULL) {
		return NULL;
	}
	MusicNode* result = musicList->tail;
	musicList->tail = musicList->tail->prev;
	if (musicList->tail == NULL) {
		musicList->head = NULL;
	} else {
		musicList->tail->next = NULL;
	}
	musicList->numMusics -= 1;
	return result;
}

void headInsertMusic(MusicList* musicList, MusicNode* music) {
	if (musicList->head == NULL && musicList->tail == NULL) {
		musicList->head = music;
		musicList->tail = music;
		musicList->numMusics += 1;
	} else {
    music->next = musicList->head;
    musicList->head->prev = music;
    musicList->head = music;
		musicList->numMusics += 1;
  }
}

void printMusicList(MusicList* musicList) {
	MusicNode* current = musicList->head;
	while (current != NULL) {
		printf("%s\n", current->music);
		current = current->next;
	}
	printf("Total: %d\n", musicList->numMusics);
}

void freeMusicList(MusicList* musicList) {
	MusicNode* current = musicList->head;
	while (current != NULL) {
		MusicNode* temp = current;
		current = current->next;
		free(temp);
	}
	free(musicList);
}

void resetMusicList(MusicList* musicList) {
	MusicNode* current = musicList->head;
	while (current != NULL) {
		MusicNode* temp = current;
		current = current->next;
		free(temp);
	}
	musicList->head = NULL;
	musicList->tail = NULL;
	musicList->numMusics = 0;
}
