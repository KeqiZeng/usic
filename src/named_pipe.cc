#include "named_pipe.h"
#include "log.h"
#include <format>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>

NamedPipe::NamedPipe(std::string_view pipe_path)
    : PIPE_PATH_(pipe_path)
{
}
NamedPipe::~NamedPipe()
{
    this->closePipe();
}

void NamedPipe::setup()
{
    if (access(PIPE_PATH_.data(), F_OK) != -1) {
        // if named pipe exists, delete it for initialization
        if (unlink(PIPE_PATH_.data()) == -1) {
            LOG(std::format("failed to delete named pipe {}", PIPE_PATH_), LogType::ERROR);
            throw std::runtime_error("failed to delete named pipe");
        }
    }

    if (mkfifo(PIPE_PATH_.data(), 0666)) {
        LOG(std::format("failed to create named pipe {}", PIPE_PATH_), LogType::ERROR);
        throw std::runtime_error("failed to create named pipe");
    }
    LOG(std::format("created named pipe {}", PIPE_PATH_), LogType::INFO);
}

void NamedPipe::openPipe(OpenMode open_mode)
{
    fd_ = open(PIPE_PATH_.data(), static_cast<int>(open_mode));
    if (fd_ == -1) {
        LOG(std::format("failed to open named pipe {}", PIPE_PATH_), LogType::ERROR);
        throw std::runtime_error("failed to open named pipe");
    }
    mode_ = open_mode;
}

void NamedPipe::closePipe()
{
    if (fd_ != -1)
        close(fd_);
}

void NamedPipe::deletePipe()
{
    unlink(PIPE_PATH_.data());
    LOG(std::format("deleted named pipe {}", PIPE_PATH_), LogType::INFO);
}

int NamedPipe::getFd()
{
    return fd_;
}

void NamedPipe::writeIn(std::string_view msg)
{
    if (mode_ != OpenMode::WR_ONLY_BLOCK) {
        LOG(std::format("named pipe {} is not writable", PIPE_PATH_), LogType::ERROR);
        return;
    }

    ssize_t bytes_written = write(fd_, msg.data(), msg.size());
    if (bytes_written == -1) {
        LOG(std::format("failed to write to named pipe {}", PIPE_PATH_), LogType::ERROR);
    }
}

std::string NamedPipe::readOut()
{
    if (mode_ != OpenMode::RD_ONLY_BLOCK) {
        LOG(std::format("named pipe {} is not readable", PIPE_PATH_), LogType::ERROR);
        return "";
    }
    std::string msg;
    msg.resize(MAX_MSG_SIZE);
    ssize_t bytes_read = read(fd_, msg.data(), MAX_MSG_SIZE);
    if (bytes_read == -1) {
        LOG(std::format("failed to read from named pipe {}", PIPE_PATH_), LogType::ERROR);
        return "";
    }
    msg.resize(bytes_read);

    return msg;
}
