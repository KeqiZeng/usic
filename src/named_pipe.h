#pragma once

#include <string>
#include <sys/fcntl.h>

struct SpecialMessages
{
    static constexpr std::string_view OVER       = "OVER";
    static constexpr std::string_view GOT        = "GOT";
    static constexpr std::string_view NO_MESSAGE = "NO_MESSAGE";
};

enum class OpenMode
{
    RD_ONLY_BLOCK = O_RDONLY,
    WR_ONLY_BLOCK = O_WRONLY,
};

class NamedPipe
{
  public:
    NamedPipe(std::string_view pipe_path);
    ~NamedPipe();
    NamedPipe(const NamedPipe&)            = delete;
    NamedPipe(NamedPipe&&)                 = delete;
    NamedPipe& operator=(const NamedPipe&) = delete;
    NamedPipe& operator=(NamedPipe&&)      = delete;

    void setup();
    void openPipe(OpenMode open_mode);
    void closePipe();
    void deletePipe();
    int getFd();
    void writeIn(std::string_view msg);
    std::string readOut();

  private:
    static constexpr size_t MAX_MSG_SIZE = 1024;
    const std::string PIPE_PATH_;
    OpenMode mode_;
    int fd_ = -1;
};
