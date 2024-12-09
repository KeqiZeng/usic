#include "runtime.h"

#include <_stdio.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <future>
#include <string_view>
#include <sys/_types/_ssize_t.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>

#include "fmt/core.h"
#include "utils.h"

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
            LOG(fmt::format("failed to delete named pipe {}", PIPE_PATH_), LogType::ERROR);
            throw std::runtime_error("failed to delete named pipe");
        }
    }

    if (mkfifo(PIPE_PATH_.data(), 0666)) {
        LOG(fmt::format("failed to create named pipe {}", PIPE_PATH_), LogType::ERROR);
        throw std::runtime_error("failed to create named pipe");
    }
    LOG(fmt::format("created named pipe {}", PIPE_PATH_), LogType::INFO);
}

void NamedPipe::openPipe(OpenMode open_mode)
{
    fd_ = open(PIPE_PATH_.data(), static_cast<int>(open_mode));
    if (fd_ == -1) {
        LOG(fmt::format("failed to open named pipe {}", PIPE_PATH_), LogType::ERROR);
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
    LOG(fmt::format("deleted named pipe {}", PIPE_PATH_), LogType::INFO);
}

int NamedPipe::getFd()
{
    return fd_;
}

void NamedPipe::writeIn(std::string_view msg)
{
    if (mode_ != OpenMode::WR_ONLY_BLOCK) {
        LOG(fmt::format("named pipe {} is not writable", PIPE_PATH_), LogType::ERROR);
        return;
    }

    ssize_t bytes_written = write(fd_, msg.data(), msg.size());
    if (bytes_written == -1) {
        LOG(fmt::format("failed to write to named pipe {}", PIPE_PATH_), LogType::ERROR);
    }
}

std::string NamedPipe::readOut()
{
    if (mode_ != OpenMode::RD_ONLY_BLOCK) {
        LOG(fmt::format("named pipe {} is not readable", PIPE_PATH_), LogType::ERROR);
        return "";
    }
    std::string msg;
    msg.resize(1024);
    ssize_t bytes_read = read(fd_, msg.data(), 1024);
    if (bytes_read == -1) {
        LOG(fmt::format("failed to read from named pipe {}", PIPE_PATH_), LogType::ERROR);
        return "";
    }
    else {
        msg.resize(bytes_read);
    }
    return msg;
}

// Config::Config(bool random, bool single_loop, std::string_view usic_library, std::string_view play_list_path)
//     : random_(random)
//     , single_loop_(single_loop)
Config::Config(std::string_view usic_library, std::string_view play_list_path, PlayMode mode)
    : mode_{mode}
{
    if (usic_library.empty()) {
        LOG("usicLibrary can not be empty", LogType::ERROR);
        throw std::runtime_error("usicLibrary can not be empty");
    }
    if (usic_library.back() != '/') {
        usic_library_ = fmt::format("{}{}", usic_library, '/');
    }
    else {
        usic_library_ = usic_library;
    }

    if (play_list_path.empty()) {
        LOG("playListPath is empty, use USIC_LIBARY as default", LogType::INFO);
        play_list_path_ = usic_library_;
    }
    if (play_list_path.back() != '/') {
        play_list_path_ = fmt::format("{}{}{}", usic_library_, play_list_path, '/');
    }
    else {
        play_list_path_ = fmt::format("{}{}", usic_library_, play_list_path);
    }
}

std::string Config::getUsicLibrary()
{
    return usic_library_;
}

std::string Config::getPlayListPath()
{
    return play_list_path_;
}

PlayMode Config::getPlayMode()
{
    return mode_;
}

void Config::setPlayMode(PlayMode mode)
{
    mode_ = mode;
}

bool isServerRunning()
{
    // Execute ps command and read its output
    FILE* fp = popen("/bin/ps -e -o comm=", "r");
    if (fp == nullptr) {
        LOG("failed to check if server is running", LogType::ERROR);
        throw std::runtime_error("failed to check if server is running");
    }

    // Read the output of the command
    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), fp) != nullptr) {
        // Remove the trailing newline character
        buffer[std::strcspn(buffer, "\n")] = '\0';

        if (std::strcmp(buffer, "usic server") == 0) {
            pclose(fp);
            return true; // server is running
        }
    }

    pclose(fp);
    return false; // server is not running
}

void setupRuntime(NamedPipe* pipe_to_server, NamedPipe* pipe_to_client)
{
    if (!std::getenv("HOME")) {
        fmt::print(stderr, "failed to get environment variable: HOME\n");
        throw std::runtime_error("failed to get environment variable: HOME");
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
    if (pipe_to_server == nullptr || pipe_to_client == nullptr) {
        LOG("failed to setup named pipes, pipe_to_server or pipe_to_client is nullptr", LogType::ERROR);
        throw std::runtime_error("failed to setup named pipes, pipe_to_server or pipe_to_client is nullptr");
    }

    try {
        pipe_to_server->setup();
    }
    catch (std::exception& e) {
        LOG(fmt::format("failed to setup pipeToServer: {}", e.what()), LogType::ERROR);
        throw std::runtime_error(fmt::format("failed to setup pipeToServer: {}", e.what()));
    }

    try {
        pipe_to_client->setup();
    }
    catch (std::exception& e) {
        LOG(fmt::format("failed to setup pipe_to_client: {}", e.what()), LogType::ERROR);
        throw std::runtime_error(fmt::format("failed to setup pipe_to_client: {}", e.what()));
    }
}

void log(std::string_view message, LogType type, std::string_view file, int line, std::string_view func)
{
    switch (type) {
    case LogType::ERROR:
        std::future<void>{} = std::async(std::launch::async, utils::logMsg, message, ERROR_LOG_FILE, file, line, func);
        break;
    case LogType::INFO:
        std::future<void>{} = std::async(std::launch::async, utils::logMsg, message, INFO_LOG_FILE, file, line, func);
        break;
    default:
        break;
    }
}