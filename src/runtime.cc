#include "runtime.h"

#include <future>

#include "utils.h"

NamedPipe::NamedPipe(const std::string& _pipePath) : pipePath(_pipePath) {}
NamedPipe::~NamedPipe() { this->close_pipe(); }

int NamedPipe::setup() {
  if (access(this->pipePath.data(), F_OK) != -1) {
    // if named pipe exists, delete it for initialization
    if (unlink(this->pipePath.data()) == -1) {
      log(fmt::format("Failed to delete named pipe {}", this->pipePath),
          LogType::ERROR);
      return FATAL_ERROR;
    }
  }

  if (mkfifo(this->pipePath.data(), 0666)) {
    log(fmt::format("Failed to create named pipe {}", this->pipePath),
        LogType::ERROR);
    return FATAL_ERROR;
  }
  log(fmt::format("Created named pipe {}", this->pipePath), LogType::INFO);
  return 0;
}

void NamedPipe::open_pipe(OpenMode openMode) {
  this->fd = open(this->pipePath.data(), static_cast<int>(openMode));
  if (this->fd == -1) {
    log(fmt::format("Failed to open named pipe {}", this->pipePath),
        LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  this->mode = openMode;
}

void NamedPipe::close_pipe() {
  if (this->fd != -1) close(this->fd);
}

void NamedPipe::delete_pipe() {
  unlink(this->pipePath.data());
  log(fmt::format("Deleted named pipe {}", this->pipePath), LogType::INFO);
}

int NamedPipe::get_fd() { return this->fd; }

void NamedPipe::writeIn(const std::string& msg) {
  if (this->mode != OpenMode::WR_ONLY_BLOCK) {
    log(fmt::format("Named pipe {} is not writable", this->pipePath),
        LogType::ERROR);
    return;
  }

  ssize_t bytesWritten = write(this->fd, msg.data(), msg.size());
  if (bytesWritten == -1) {
    log(fmt::format("Failed to write to named pipe {}", this->pipePath),
        LogType::ERROR);
  }
}

std::string NamedPipe::readOut() {
  if (this->mode != OpenMode::RD_ONLY_BLOCK) {
    log(fmt::format("Named pipe {} is not readable", this->pipePath),
        LogType::ERROR);
    return "";
  }
  std::string msg;
  msg.resize(1024);
  ssize_t bytesRead = read(this->fd, msg.data(), 1024);
  if (bytesRead == -1) {
    log(fmt::format("Failed to read from named pipe {}", this->pipePath),
        LogType::ERROR);
  } else {
    msg.resize(bytesRead);
  }
  return msg;
}

Config::Config(bool _repetitive, bool _random, const std::string& _usicLibrary,
               const std::string& _playListPath)
    : repetitive(_repetitive), random(_random) {
  if (_usicLibrary.empty()) {
    log("usicLibrary can not be empty", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  if (_usicLibrary.back() != '/') {
    this->usicLibrary = fmt::format("{}{}", _usicLibrary, '/');
  } else {
    this->usicLibrary = _usicLibrary;
  }

  if (_playListPath.empty()) {
    log("playListPath is empty, use USIC_LIBARY as default", LogType::INFO);
    this->playListPath = this->usicLibrary;
  }
  if (_playListPath.back() != '/') {
    this->playListPath =
        fmt::format("{}{}{}", this->usicLibrary, _playListPath, '/');
  } else {
    this->playListPath = fmt::format("{}{}", this->usicLibrary, _playListPath);
  }
}

bool Config::is_repetitive() { return this->repetitive; }
bool Config::is_random() { return this->random; }
bool* Config::get_random_p() { return &this->random; }
bool* Config::get_repetitive_p() { return &this->repetitive; }
void Config::toggle_random() { this->random = !this->random; }
void Config::toggle_repetitive() { this->repetitive = !this->repetitive; }
std::string Config::get_usic_library() { return this->usicLibrary; }
std::string Config::get_playList_path() { return this->playListPath; }

bool server_is_running() {
  // Execute ps command and read its output
  FILE* fp = popen("/bin/ps -e -o comm=", "r");
  if (fp == nullptr) {
    log("Failed to execute ps command", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }

  // Read the output of the command
  char buffer[512];
  while (std::fgets(buffer, sizeof(buffer), fp) != nullptr) {
    // Remove the trailing newline character
    buffer[std::strcspn(buffer, "\n")] = '\0';

    if (std::strcmp(buffer, "usic server") == 0) {
      pclose(fp);
      return true;  // server is running
    }
  }

  pclose(fp);
  return false;  // server is not running
}

void setup_runtime(NamedPipe* pipeToServer, NamedPipe* pipeToClient) {
  if (!std::getenv("HOME")) {
    fmt::print(stderr, "Failed to get environment variable: HOME\n");
    std::exit(FATAL_ERROR);
  }
  // create runtime path
  if (!std::filesystem::exists(RUNTIME_PATH)) {
    std::filesystem::create_directory(RUNTIME_PATH);
  }

  // remove log files for initialization
  if (std::filesystem::exists(ERROR_LOG_FILE)) {
    std::filesystem::remove(ERROR_LOG_FILE);
  }
  if (std::filesystem::exists(INFO_LOG_FILE)) {
    std::filesystem::remove(INFO_LOG_FILE);
  }

  // setup named pipes
  if (pipeToServer == nullptr || pipeToClient == nullptr) {
    log("Failed to setup named pipes, pipeToServer or pipeToClient is nullptr",
        LogType::ERROR);
    std::exit(FATAL_ERROR);
  }

  if (pipeToServer->setup() == FATAL_ERROR) {
    log("Failed to setup pipeToServer", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
  if (pipeToClient->setup() == FATAL_ERROR) {
    log("Failed to setup pipeToClient", LogType::ERROR);
    std::exit(FATAL_ERROR);
  }
}

void log(std::string_view message, LogType type) {
  switch (type) {
    case LogType::ERROR:
      std::future<void>{} =
          std::async(std::launch::async, utils::log, message, ERROR_LOG_FILE);
      break;
    case LogType::INFO:
      std::future<void>{} =
          std::async(std::launch::async, utils::log, message, INFO_LOG_FILE);
      break;
  }
}