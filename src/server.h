#pragma once
#include <string>
#include <vector>

#include "music_list.h"
#include "runtime.h"

void initMusicList(MusicList* music_list);
std::string getCommand(NamedPipe* pipe_to_server, NamedPipe* pipe_to_client);
void sendMsgToClient(std::string_view msg, NamedPipe* pipe_to_server, NamedPipe* pipe_to_client);
void sendMsgToClient(const std::vector<std::string>* msg, NamedPipe* pipe_to_server, NamedPipe* pipe_to_client);
void handleCommand(std::vector<std::string>* args);
void server();