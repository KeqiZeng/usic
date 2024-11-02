#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "config.h"
#include "fmt/core.h"

#define FATAL_ERROR -1

// for named pipes communication
#define OVER "OVER"
#define GOT "GOT"
#define NO_MESSAGE "NO_MESSAGE"

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

enum class LogType {
  ERROR,
  INFO,
};

enum class OpenMode {
  RD_ONLY_BLOCK = O_RDONLY,
  WR_ONLY_BLOCK = O_WRONLY,
};

class NamedPipe {
  const std::string pipePath;
  OpenMode mode;
  int fd = -1;

 public:
  NamedPipe(const std::string& _pipePath);
  ~NamedPipe();

  int setup();
  void open_pipe(OpenMode openMode);
  void close_pipe();
  void delete_pipe();
  int get_fd();
  void writeIn(const std::string& msg);
  std::string readOut();
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
  bool* get_repetitive_p();
  bool* get_random_p();
  void toggle_random();
  void toggle_repetitive();
  std::string get_usic_library();
  std::string get_playList_path();
};

bool server_is_running();
void setup_runtime(NamedPipe* pipeToServer, NamedPipe* pipeToClient);
void log(std::string_view message, LogType type);