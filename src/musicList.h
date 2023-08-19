#ifndef USIC_MUSICLIST_H
#define USIC_MUSICLIST_H

typedef struct MusicList {
	// 3 situations:
	// 1. musicList->head == musicList->tail == NULL; no musicNode in List
	// 2. musicList->head == musicList->tail != NULL; only one musicNode in List
	// 3. musicList->head != musicList->tail, head != NULL && tail != NULL, head->prev == NULL, tail->next == NULL
	struct MusicNode* head;
	struct MusicNode* tail;
	int numMusics;
} MusicList;

typedef struct MusicNode {
	char* music;
	struct MusicNode* prev;
	struct MusicNode* next;
} MusicNode;

MusicList* createMusicList();
MusicNode* createMusicNode(char* music);
void queueMusic(MusicList* musicList, MusicNode* music);
MusicNode* dequeueMusic(MusicList* musicList);
MusicNode* popMusic(MusicList* musicList);
void headInsertMusic(MusicList* musicList, MusicNode* music);
void printMusicList(MusicList* musicList);
void freeMusicList(MusicList* musicList);
void resetMusicList(MusicList* musicList);

#endif // !USIC_MUSICLIST_H
