#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "config.h"
#include "fmt/core.h"

#define FATAL_ERROR -1
const std::string RUNTIME_PATH =
    fmt::format("{}/{}", std::getenv("HOME"), ".local/state/usic");
const std::string ERROR_LOG_FILE =
    fmt::format("{}/{}", RUNTIME_PATH, "error.log");
const std::string INFO_LOG_FILE =
    fmt::format("{}/{}", RUNTIME_PATH, "info.log");
const std::string PIPE_TO_SERVER =
    fmt::format("{}/{}", RUNTIME_PATH, "pipe_to_server");
const std::string PIPE_TO_CLIENT =
    fmt::format("{}/{}", RUNTIME_PATH, "pipe_to_client");

enum LogType {
  ERROR,
  INFO,
};

enum OpenMode {
  RD_ONLY = O_RDONLY,
  WR_ONLY = O_WRONLY,
};

class NamedPipe {
  const std::string pipePath;
  int fd = -1;

 public:
  NamedPipe(const std::string& _pipePath);
  ~NamedPipe();

  int setup();
  void open_pipe(OpenMode openMode);
  void close_pipe();
};

class Config {
  bool repetitive;
  bool random;
  std::string usicLibrary;
  std::string playListPath;

 public:
  Config(bool _repetitive, bool _random, const std::string& _usicLibrary,
         const std::string& _playListPath);
  bool is_repetitive();
  bool is_random();
  std::string get_usic_library();
  std::string get_playList_path();
};

bool server_is_running();
void setup_runtime();
void log(std::string_view message, LogType type);