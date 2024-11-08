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

const std::string VERSION = "0.0.1";
const std::string DOC     = "USIC DOCUMENT:\n"
                            "Server:\n"
                            "  usic [FLAGS]\n"
                            "  usic                Run server\n"
                            "FLAGS:\n"
                            "  -h, --help          Print this document\n"
                            "  -v, --version       Print version\n"
                            "\n"
                            "Client:\n"
                            "  usic <COMMAND> [ARGUMENT]\n"
                            "COMMAND:\n"
                            "  load                  Load playlist                REQUIRED ARGUMENT: "
                            "The path of playlist file\n"
                            "  play                  Play music                   OPTIONAL ARGUMENT: "
                            "The path of music file; plays first in playlist if not provided\n"
                            "  play_later            Play given music next        REQUIRED ARGUMENT: "
                            "The path of music file\n"
                            "  play_next             Play next music in playlist  NO ARGUMENT\n"
                            "  play_prev             Play previous music          NO ARGUMENT\n"
                            "  pause                 Pause or resume music        NO ARGUMENT\n"
                            "  cursor_forward        Move cursor forward          NO ARGUMENT\n"
                            "  cursor_backward       Move cursor backward         NO ARGUMENT\n"
                            "  set_cursor            Set cursor to given time     REQUIRED ARGUMENT: "
                            "Time in MM:SS format\n"
                            "  get_current_progress  Get current progress         NO ARGUMENT\n"
                            "  volume_up             Increase volume              NO ARGUMENT\n"
                            "  volume_down           Decrease volume              NO ARGUMENT\n"
                            "  set_volume            Set volume                   REQUIRED ARGUMENT: "
                            "Volume between 0.0 and 1.0\n"
                            "  get_volume            Get current volume           NO ARGUMENT\n"
                            "  mute                  Mute or unmute volume        NO ARGUMENT\n"
                            "  set_random            Toggle random mode           NO ARGUMENT\n"
                            "  set_repetitive        Toggle repetitive mode       NO ARGUMENT\n"
                            "  get_list              Print playlist               NO ARGUMENT\n"
                            "  quit                  Quit server                  NO ARGUMENT\n";

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
    Config(bool repetitive, bool random, std::string_view usic_library, std::string_view play_list_path);
    bool isRepetitive();
    bool isRandom();
    bool* getRepetitivePtr();
    bool* getRandomPtr();
    void toggleRandom();
    void toggleRepetitive();
    std::string getUsicLibrary();
    std::string getPlayListPath();

  private:
    bool repetitive_;
    bool random_;
    std::string usic_library_;
    std::string play_list_path_;
};

bool isServerRunning();
void setupRuntime(NamedPipe* pipe_to_server, NamedPipe* pipe_to_client);
void log(std::string_view message, LogType type, std::string_view func_name);