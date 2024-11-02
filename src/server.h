#pragma once
#include "commands.h"
#include "config.h"
#include "core.h"
#include "runtime.h"

std::string get_command(NamedPipe* pipeToServer, NamedPipe* pipeToClient);
void sendMsgToClient(const std::string& msg, NamedPipe* pipeToServer,
                     NamedPipe* pipeToClient);
void sendMsgToClient(const std::vector<std::string>* msg,
                     NamedPipe* pipeToServer, NamedPipe* pipeToClient);
void handle_command(std::vector<std::string>* args);
void server();