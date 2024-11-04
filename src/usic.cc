#include <iostream>
#include <string>

#include "client.h"
#include "commands.h"
#include "config.h"
#include "core.h"
#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"
#include "server.h"

/* marcos */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_COREAUDIO
#define MA_ENABLE_ALSA
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

// miniaudio should be included after MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

int main(int argc, char* argv[]) {
  bool serverRunning = server_is_running();
  if (argc == 1) {
    if (serverRunning) {
      fmt::print("Server is already running\n");
      return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
      // Forking failed
      log("forking child process failed", LogType::ERROR, __func__);
      return 1;
    }
    if (pid > 0) {
      // Parent process, exit
      return 0;
    }

    // change the name of server process
    strncpy(argv[0], "usic server", sizeof("usic server"));
    server();
    return 0;
  }
  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      // TODO: print help
      fmt::print("Usage: usic [options]\n");
      return 0;
    }

    if (!serverRunning) {
      fmt::print("Server is not running\n");
      return 1;
    }
    client(argc, argv);
    return 0;
  }
}