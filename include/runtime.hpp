#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include "fmt/core.h"
enum LogType {
  ERROR,
  INFO,
};
auto server_is_running() -> bool;
auto setup_runtime(Pipe* pipeToServer, Pipe* pipeToClient) -> void;
auto log(std::string_view message, LogType type) -> void;
class Pipe {
  const std::string pipePath;
  std::ifstream reader;
  std::ofstream writer;

 public:
  Pipe(const std::string& _pipePath) : pipePath(_pipePath) {}
  ~Pipe() { this->close_pipe(); }

  auto setup() -> int {
    if (access(this->pipePath.data(), F_OK) != -1) {
      // if named pipe exists, delete it for initialization
      if (unlink(this->pipePath.data()) == -1) {
        log(fmt::format("Failed to delete pipe {}", this->pipePath),
            LogType::ERROR);
        return -1;
      }
    }

    int err = mkfifo(this->pipePath.data(), 0666);
    if (err == -1) {
      log(fmt::format("Failed to create pipe {}", this->pipePath),
          LogType::ERROR);
      return -1;
    }
    return 0;
  }
};