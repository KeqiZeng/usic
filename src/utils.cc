#include "utils.h"
#include "config.h"
#include "log.h"
#include "miniaudio.h"
#include <_stdio.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

constexpr unsigned int SECONDS_PER_MINUTE = 60;

namespace Utils
{

void createFile(std::string_view filename)
{
    // Create parent directory if it doesn't exist
    std::filesystem::path file_path(filename);
    if (!std::filesystem::exists(file_path.parent_path())) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    // Create file and truncate if it already exists
    std::ofstream out_file(filename.data(), std::ios::out | std::ios::trunc);
    if (!out_file) {
        LOG(std::format("failed to create file: {}", filename), LogType::ERROR);
        throw std::ios_base::failure(std::format("failed to create file: {}", filename));
    }

    out_file.close();
    if (out_file.fail()) {
        LOG(std::format("failed to close file: {}", filename), LogType::ERROR);
        throw std::ios_base::failure(std::format("failed to close file: {}", filename));
    }
}

static const std::string generateRandomString()
{
    static const std::string CHARS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, CHARS.size() - 1);

    constexpr size_t LENGTH = 10;
    std::string result;
    result.reserve(LENGTH);
    for (size_t i = 0; i < LENGTH; ++i) {
        result += CHARS[dis(gen)];
    }
    return result;
}

const std::string createTmpBlankFile()
{
    std::string random_name = "usic_" + generateRandomString() + ".txt";
    std::string temp_file   = std::filesystem::temp_directory_path() / random_name;
    createFile(temp_file);
    return temp_file;
}

const std::string createTmpDefaultList()
{
    std::string random_name = "usic_" + generateRandomString() + ".txt";
    std::string temp_file   = std::filesystem::temp_directory_path() / random_name;

    std::ofstream outfile(temp_file, std::ios::out | std::ios::trunc);
    if (!outfile.is_open()) {
        LOG(std::format("Failed to open temporary file: {}", temp_file), LogType::ERROR);
        throw std::runtime_error(std::format("Failed to create temporary file: {}", temp_file));
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(Config::USIC_LIBRARY)) {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        std::ranges::transform(ext, ext.begin(), ::tolower);

        if (ext == ".mp3" || ext == ".flac" || ext == ".wav") {
            outfile << entry.path().filename().string() << '\n';
            if (outfile.fail()) {
                LOG(std::format("Failed to write to temporary file: {}", temp_file), LogType::ERROR);
                throw std::runtime_error(std::format("Failed to write to temporary file: {}", temp_file));
            }
        }
    }

    outfile.close();
    if (outfile.fail()) {
        LOG(std::format("Failed to close temporary file: {}", temp_file), LogType::ERROR);
        throw std::runtime_error(std::format("Failed to close temporary file: {}", temp_file));
    }

    return temp_file;
}

void removeTmpFiles()
{
    const auto TMP_DIR = std::filesystem::temp_directory_path();
    for (const auto& entry : std::filesystem::directory_iterator(TMP_DIR)) {
        if (!entry.is_regular_file())
            continue;

        const std::string FILENAME = entry.path().filename().string();
        if (FILENAME.starts_with("usic_") && FILENAME.ends_with(".txt")) {
            std::error_code ec;
            std::filesystem::remove(entry.path(), ec);
            if (ec) {
                LOG(std::format("Failed to delete temporary file {}: {}", entry.path().string(), ec.message()),
                    LogType::ERROR);
            }
        }
    }
}

bool isLineInFile(std::string_view line, std::string_view file_name)
{
    std::ifstream file(file_name.data());
    if (file.is_open()) {
        std::string buffer;
        while (std::getline(file, buffer)) {
            if (buffer == line) {
                file.close();
                return true;
            }
        }
        file.close();
    }
    return false;
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

std::optional<unsigned int> timeStrToSec(std::string_view time_str)
{
    int min = 0;
    int sec = 0;

    std::istringstream iss(time_str.data());
    char colon{};
    if (!(iss >> min >> colon >> sec) || colon != ':') {
        return std::nullopt; // failed conversion
    }

    // minutes should not be less than 0
    if (min < 0) {
        return std::nullopt;
    }

    // seconds should be 0-60
    if (sec < 0 || sec > SECONDS_PER_MINUTE) {
        return std::nullopt;
    }

    return static_cast<unsigned int>(min * SECONDS_PER_MINUTE + sec);
}

std::optional<std::string> secToTimeStr(unsigned int seconds)
{
    unsigned int min = seconds / SECONDS_PER_MINUTE;
    unsigned int sec = seconds % SECONDS_PER_MINUTE;

    return std::format("{:d}:{:02d}", min, sec);
}

bool commandEq(const std::string& command, const std::string_view& target)
{
    return (
        command == target ||
        std::ranges::find(Config::COMMANDS.at(target), command) != Config::COMMANDS.at(target).end()
    );
}

const std::string concatenateArgs(int argc, char* argv[])
{
    std::string result;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // check if the argument contains spaces
        if (arg.find(' ') != std::string::npos) {
            result += "\"" + arg + "\"";
        }
        else {
            result += arg;
        }

        if (i < argc - 1) { // add separator if not the last argument
            result += " ";
        }
    }

    return result;
}

ma_result reinitDecoder(std::string_view filename, ma_decoder_config* config, ma_decoder* decoder)
{
    ma_result result = ma_decoder_uninit(decoder);
    if (result != MA_SUCCESS) {
        LOG("failed to uninitialize decoder", LogType::ERROR, result);
        return result;
    }

    result = ma_decoder_init_file(filename.data(), config, decoder);
    if (result != MA_SUCCESS) {
        LOG(std::format("failed to initialize decoder for file {}", filename), LogType::ERROR, result);
        return result;
    }
    return result;
}

ma_result seekToStart(ma_decoder* decoder) noexcept
{
    return ma_decoder_seek_to_pcm_frame(decoder, 0);
}

ma_result seekToEnd(ma_decoder* decoder)
{
    ma_uint64 length = 0;
    ma_result result = ma_decoder_get_length_in_pcm_frames(decoder, &length);
    if (result != MA_SUCCESS) {
        LOG("failed to get length in PCM frames", LogType::ERROR, result);
        return result;
    }
    return ma_decoder_seek_to_pcm_frame(decoder, length);
}

std::string getWAVFileName(std::string_view filename)
{
    // create a file in temp directory with same name but .wav extension
    std::filesystem::path file_path(filename);
    std::string base_name          = file_path.stem().string();
    std::string temp_dir           = std::filesystem::temp_directory_path().string();
    std::filesystem::path wav_path = std::filesystem::path(temp_dir) / "usic";

    // create the usic directory in temp if it doesn't exist
    if (!std::filesystem::exists(wav_path)) {
        std::filesystem::create_directories(wav_path);
    }

    return (wav_path / (base_name + ".wav")).string();
}

} // namespace Utils
