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
    if (access(this->PIPE_PATH_.data(), F_OK) != -1) {
        // if named pipe exists, delete it for initialization
        if (unlink(this->PIPE_PATH_.data()) == -1) {
            log(fmt::format("failed to delete named pipe {}", this->PIPE_PATH_), LogType::ERROR, __func__);
            // return FATAL_ERROR;
            throw std::runtime_error("failed to delete named pipe");
        }
    }

    if (mkfifo(this->PIPE_PATH_.data(), 0666)) {
        log(fmt::format("failed to create named pipe {}", this->PIPE_PATH_), LogType::ERROR, __func__);
        throw std::runtime_error("failed to create named pipe");
    }
    log(fmt::format("created named pipe {}", this->PIPE_PATH_), LogType::INFO, __func__);
    // return 0;
}

void NamedPipe::openPipe(OpenMode open_mode)
{
    this->fd_ = open(this->PIPE_PATH_.data(), static_cast<int>(open_mode));
    if (this->fd_ == -1) {
        log(fmt::format("failed to open named pipe {}", this->PIPE_PATH_), LogType::ERROR, __func__);
        throw std::runtime_error("failed to open named pipe");
    }
    this->mode_ = open_mode;
}

void NamedPipe::closePipe()
{
    if (this->fd_ != -1) close(this->fd_);
}

void NamedPipe::deletePipe()
{
    unlink(this->PIPE_PATH_.data());
    log(fmt::format("deleted named pipe {}", this->PIPE_PATH_), LogType::INFO, __func__);
}

int NamedPipe::getFd()
{
    return this->fd_;
}

void NamedPipe::writeIn(std::string_view msg)
{
    if (this->mode_ != OpenMode::WR_ONLY_BLOCK) {
        log(fmt::format("named pipe {} is not writable", this->PIPE_PATH_), LogType::ERROR, __func__);
        return;
    }

    ssize_t bytes_written = write(this->fd_, msg.data(), msg.size());
    if (bytes_written == -1) {
        log(fmt::format("failed to write to named pipe {}", this->PIPE_PATH_), LogType::ERROR, __func__);
    }
}

std::string NamedPipe::readOut()
{
    if (this->mode_ != OpenMode::RD_ONLY_BLOCK) {
        log(fmt::format("named pipe {} is not readable", this->PIPE_PATH_), LogType::ERROR, __func__);
        return "";
    }
    std::string msg;
    msg.resize(1024);
    ssize_t bytes_read = read(this->fd_, msg.data(), 1024);
    if (bytes_read == -1) {
        log(fmt::format("failed to read from named pipe {}", this->PIPE_PATH_), LogType::ERROR, __func__);
    }
    else {
        msg.resize(bytes_read);
    }
    return msg;
}

Config::Config(bool repetitive, bool random, std::string_view usic_library, std::string_view play_list_path)
    : repetitive_(repetitive)
    , random_(random)
{
    if (usic_library.empty()) {
        log("usicLibrary can not be empty", LogType::ERROR, __func__);
        throw std::runtime_error("usicLibrary can not be empty");
    }
    if (usic_library.back() != '/') { this->usic_library_ = fmt::format("{}{}", usic_library, '/'); }
    else {
        this->usic_library_ = usic_library;
    }

    if (play_list_path.empty()) {
        log("playListPath is empty, use USIC_LIBARY as default", LogType::INFO, __func__);
        this->play_list_path_ = this->usic_library_;
    }
    if (play_list_path.back() != '/') {
        this->play_list_path_ = fmt::format("{}{}{}", this->usic_library_, play_list_path, '/');
    }
    else {
        this->play_list_path_ = fmt::format("{}{}", this->usic_library_, play_list_path);
    }
}

bool Config::isRepetitive()
{
    return this->repetitive_;
}
bool Config::isRandom()
{
    return this->random_;
}
bool* Config::getRandomPtr()
{
    return &this->random_;
}
bool* Config::getRepetitivePtr()
{
    return &this->repetitive_;
}
void Config::toggleRandom()
{
    this->random_ = !this->random_;
}
void Config::toggleRepetitive()
{
    this->repetitive_ = !this->repetitive_;
}
std::string Config::getUsicLibrary()
{
    return this->usic_library_;
}
std::string Config::getPlayListPath()
{
    return this->play_list_path_;
}

bool isServerRunning()
{
    // Execute ps command and read its output
    FILE* fp = popen("/bin/ps -e -o comm=", "r");
    if (fp == nullptr) {
        log("failed to check if server is running", LogType::ERROR, __func__);
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
    if (!std::filesystem::exists(RUNTIME_PATH)) { std::filesystem::create_directory(RUNTIME_PATH); }

    // remove log files for initialization
    if (std::filesystem::exists(ERROR_LOG_FILE)) { std::filesystem::remove(ERROR_LOG_FILE); }
    if (std::filesystem::exists(INFO_LOG_FILE)) { std::filesystem::remove(INFO_LOG_FILE); }

    // setup named pipes
    if (pipe_to_server == nullptr || pipe_to_client == nullptr) {
        log("failed to setup named pipes, pipe_to_server or pipe_to_client is nullptr", LogType::ERROR, __func__);
        throw std::runtime_error("failed to setup named pipes, pipe_to_server or pipe_to_client is nullptr");
    }

    try {
        pipe_to_server->setup();
    }
    catch (std::exception& e) {
        log(fmt::format("failed to setup pipeToServer: {}", e.what()), LogType::ERROR, __func__);
        throw std::runtime_error(fmt::format("failed to setup pipeToServer: {}", e.what()));
    }

    try {
        pipe_to_client->setup();
    }
    catch (std::exception& e) {
        log(fmt::format("failed to setup pipe_to_client: {}", e.what()), LogType::ERROR, __func__);
        throw std::runtime_error(fmt::format("failed to setup pipe_to_client: {}", e.what()));
    }
}

void log(std::string_view message, LogType type, std::string_view func_name)
{
    switch (type) {
    case LogType::ERROR:
        std::future<void>{} = std::async(std::launch::async, utils::logMsg, message, ERROR_LOG_FILE, func_name);
        break;
    case LogType::INFO:
        std::future<void>{} = std::async(std::launch::async, utils::logMsg, message, INFO_LOG_FILE, func_name);
        break;
    default:
        break;
    }
}