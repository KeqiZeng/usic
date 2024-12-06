#include "tools.h"

#include "config.h"
#include "fmt/core.h"
#include "runtime.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace tools
{

static bool createPlayList(std::string_view default_list, const std::filesystem::path& play_list_path, Config* config)
{
    const std::string COMMAND = fmt::format("fzf --multi < {} > {}", default_list, play_list_path.string());
    int ret_code              = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code {}\n", COMMAND, ret_code);
        return false;
    }
    return true;
}

void addMusicToList(std::string_view list_name, Config* config)
{
    int ret_code = std::system("fzf --version > /dev/null 2>&1");
    if (ret_code != 0) {
        fmt::print(stderr, "fzf is not installed\n");
        return;
    }

    const std::string DEFAULT_LIST       = utils::createTmpDefaultList();
    std::filesystem::path play_list_path = std::filesystem::path(config->getPlayListPath()) / list_name;

    if (!std::filesystem::exists(play_list_path)) {
        if (!createPlayList(DEFAULT_LIST, play_list_path, config)) {
            fmt::print(stderr, "failed to create playlist file: {}\n", list_name);
        }
        return;
    }

    const std::string BLANK_FILE = utils::createTmpBlankFile();
    const std::string COMMAND    = fmt::format("fzf --multi < {} > {}", DEFAULT_LIST, BLANK_FILE);

    ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code: {}\n", COMMAND, ret_code);
        return;
    }

    std::ifstream infile(BLANK_FILE);
    if (!infile.is_open()) {
        fmt::print(stderr, "failed to open temporary file: {}\n", BLANK_FILE);
        return;
    }

    std::string music;
    std::ofstream outfile(play_list_path, std::ios::app);
    if (!outfile.is_open()) {
        fmt::print(stderr, "failed to open playlist file: {}\n", play_list_path.string());
        return;
    }
    while (std::getline(infile, music)) {
        // only add music that is not in playlist
        if (std::filesystem::exists(play_list_path)) {
            if (!utils::isLineInFile(music, play_list_path.string())) {
                outfile << music << '\n';
            }
        }
    }
    outfile.close();
    infile.close();
    utils::removeTmpFiles();
}

static std::optional<std::string> getMusicNameFZF(std::string_view music_list_name)
{
    const std::string BLANK_FILE = utils::createTmpBlankFile();
    const std::string COMMAND    = fmt::format("fzf < {} > {}", music_list_name, BLANK_FILE);
    int ret_code                 = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code {}\n", COMMAND, ret_code);
        return std::nullopt;
    }
    std::ifstream infile(BLANK_FILE);
    if (!infile.is_open()) {
        fmt::print(stderr, "failed to open temporary file: {}\n", BLANK_FILE);
        return std::nullopt;
    }
    std::string music;
    std::getline(infile, music);
    infile.close();

    if (music.empty()) {
        return std::nullopt;
    }
    return fmt::format("\"{}\"", music);
}

void fuzzyPlay(std::string_view usic_command, Config* config)
{
    int ret_code = std::system("fzf --version > /dev/null 2>&1");
    if (ret_code != 0) {
        fmt::print(stderr, "fzf is not installed\n");
        return;
    }

    ret_code = std::system(fmt::format("{} > /dev/null 2>&1", usic_command).c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "failed to start usic server\n", ret_code);
        return;
    }

    std::optional<std::string> music_name = std::nullopt;
    if (DEFAULT_PLAY_LIST.empty()) {
        const std::string DEFAULT_LIST = utils::createTmpDefaultList();
        music_name                     = getMusicNameFZF(DEFAULT_LIST);
    }
    else {
        music_name = getMusicNameFZF(fmt::format("{}{}", config->getPlayListPath(), DEFAULT_PLAY_LIST));
    }

    if (!music_name.has_value()) {
        fmt::print(stderr, "failed to get music name\n");
        return;
    }
    const std::string COMMAND = fmt::format("{} {} {}", usic_command, PLAY, music_name.value());

    ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code {}\n", COMMAND, ret_code);
    }
}

} // namespace tools