#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "fmt/core.h"

const int FATAL_ERROR = -1;

const std::string RUNTIME_PATH   = fmt::format("{}/{}", std::getenv("HOME"), ".local/state/usic");
const std::string ERROR_LOG_FILE = fmt::format("{}/{}", RUNTIME_PATH, "error.log");
const std::string INFO_LOG_FILE  = fmt::format("{}/{}", RUNTIME_PATH, "info.log");
const std::string PIPE_TO_SERVER = fmt::format("{}/{}", RUNTIME_PATH, "pipe_to_server");
const std::string PIPE_TO_CLIENT = fmt::format("{}/{}", RUNTIME_PATH, "pipe_to_client");

enum class LogType
{
    ERROR,
    INFO,
};

enum class OpenMode
{
    RD_ONLY_BLOCK = O_RDONLY,
    WR_ONLY_BLOCK = O_WRONLY,
};

enum class PlayMode
{
    NORMAL,
    SHUFFLE,
    SINGLE,
};

// for named pipes communication
const std::string OVER       = "OVER";
const std::string GOT        = "GOT";
const std::string NO_MESSAGE = "NO_MESSAGE";

class NamedPipe
{
  public:
    NamedPipe(std::string_view pipe_path);
    ~NamedPipe();

    void setup();
    void openPipe(OpenMode open_mode);
    void closePipe();
    void deletePipe();
    int getFd();
    void writeIn(std::string_view msg);
    std::string readOut();

  private:
    const std::string PIPE_PATH_;
    OpenMode mode_;
    int fd_ = -1;
};

class Config
{
  public:
    // Config(bool random, bool single_loop, std::string_view usic_library, std::string_view play_list_path);
    Config(std::string_view usic_library, std::string_view play_list_path, PlayMode mode);
    std::string getUsicLibrary();
    std::string getPlayListPath();
    PlayMode getPlayMode();
    void setPlayMode(PlayMode mode);

  private:
    std::string usic_library_;
    std::string play_list_path_;
    PlayMode mode_;
};

bool isServerRunning();
void setupRuntime(NamedPipe* pipe_to_server, NamedPipe* pipe_to_client);

#define LOG(message, type) log(message, type, std::string_view{__FILE__}, __LINE__, std::string_view{__func__})
void log(std::string_view message, LogType type, std::string_view file, int line, std::string_view func);