#include "tools.h"

#include "app.h"
#include "config.h"
#include "runtime_path.h"
#include "utils.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>

bool handleFlagOrTool(int argc, char* argv[])
{
    if (argc > 1) {
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            args.emplace_back(argv[i]);
        }

        if (Utils::commandEq(args[0], Config::PRINT_VERSION)) {
            std::cout << std::format("usic version: {}\n", VERSION);
        }
        else if (Utils::commandEq(args[0], Config::PRINT_HELP)) {
            std::cout << DOC << "\n";
        }
        else if (Utils::commandEq(args[0], Config::ADD_MUSIC_TO_LIST)) {
            if (args.size() < 2) {
                std::cerr << std::format("{}: arguments are not enough.\n", Config::ADD_MUSIC_TO_LIST);
                return false;
            }
            Tools::addMusicToList(args[1]);
        }
        else if (Utils::commandEq(args[0], Config::FUZZY_PLAY)) {
            Tools::fuzzyPlay(argv[0]);
        }
        else if (Utils::commandEq(args[0], Config::FUZZY_PLAY_LATER)) {
            Tools::fuzzyPlayLater(argv[0]);
        }
        else {
            return false;
        }
    }
    return true;
}

namespace Tools
{

static bool isFZFavailable()
{
    int ret_code = std::system("fzf --version > /dev/null 2>&1");
    if (ret_code != 0) {
        std::cerr << "fzf is not installed\n";
        return false;
    }
    return true;
}

static bool startServer(std::string_view usic_command)
{
    const std::string COMMAND = std::format("{} > /dev/null 2>&1", usic_command);
    int ret_code              = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << "failed to start usic server\n";
        return false;
    }
    return true;
}

static bool createPlayList(std::string_view default_list, const std::filesystem::path& play_list_path)
{
    const std::string COMMAND = std::format("fzf --multi < {} > {}", default_list, play_list_path.string());
    int ret_code              = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << std::format("{}: command failed with exit code: {}\n", COMMAND, ret_code);
        return false;
    }
    return true;
}

static std::optional<std::string> getMusicNameFZF()
{
    std::string music_list_name{};
    if (Config::DEFAULT_PLAYLIST.empty()) {
        const std::string DEFAULT_LIST = Utils::createTmpDefaultList();
        music_list_name                = DEFAULT_LIST;
    }
    else {
        music_list_name = std::format("{}{}", RuntimePath::PLAYLISTS_PATH, Config::DEFAULT_PLAYLIST);
    }

    const std::string BLANK_FILE = Utils::createTmpBlankFile();
    const std::string COMMAND    = std::format("fzf < {} > {}", music_list_name, BLANK_FILE);
    int ret_code                 = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << std::format("{}: command failed with exit code {}\n", COMMAND, ret_code);
        return std::nullopt;
    }
    std::ifstream infile(BLANK_FILE);
    if (!infile.is_open()) {
        std::cerr << std::format("failed to open temporary file: {}\n", BLANK_FILE);
        return std::nullopt;
    }
    std::string music;
    std::getline(infile, music);
    infile.close();

    if (music.empty()) {
        return std::nullopt;
    }
    return std::format("\"{}\"", music);
}

void addMusicToList(std::string_view list_name)
{
    if (!isFZFavailable()) {
        return;
    }

    const std::string DEFAULT_LIST = Utils::createTmpDefaultList();
    std::filesystem::path play_list_path =
        std::filesystem::path(Config::USIC_LIBRARY) / Config::PLAYLISTS_DIR / list_name;

    if (!std::filesystem::exists(play_list_path)) {
        if (!createPlayList(DEFAULT_LIST, play_list_path)) {
            std::cerr << std::format("failed to create playlist file: {}\n", list_name);
        }
        return;
    }

    const std::string BLANK_FILE = Utils::createTmpBlankFile();
    const std::string COMMAND    = std::format("fzf --multi < {} > {}", DEFAULT_LIST, BLANK_FILE);

    int ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << std::format("{}: command failed with exit code: {}\n", COMMAND, ret_code);
        return;
    }

    std::ifstream infile(BLANK_FILE);
    if (!infile.is_open()) {
        std::cerr << std::format("failed to open temporary file: {}\n", BLANK_FILE);
        return;
    }

    std::string music;
    std::ofstream outfile(play_list_path, std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << std::format("failed to open playlist file: {}\n", play_list_path.string());
        return;
    }
    while (std::getline(infile, music)) {
        // only add music that is not in playlist
        if (std::filesystem::exists(play_list_path)) {
            if (!Utils::isLineInFile(music, play_list_path.string())) {
                outfile << music << '\n';
            }
        }
    }
    outfile.close();
    infile.close();
    Utils::removeTmpFiles();
}

void fuzzyPlay(std::string_view usic_command)
{
    if (!isFZFavailable()) {
        return;
    }

    if (!startServer(usic_command)) {
        return;
    }

    std::optional<std::string> music_name = std::nullopt;
    music_name                            = getMusicNameFZF();

    if (!music_name.has_value()) {
        std::cerr << "failed to get music name\n";
        return;
    }
    const std::string COMMAND = std::format("{} {} {}", usic_command, Config::PLAY, music_name.value());

    int ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << std::format("{}: command failed with exit code {}\n", COMMAND, ret_code);
    }
}

void fuzzyPlayLater(std::string_view usic_command)
{
    if (!isFZFavailable()) {
        return;
    }

    if (!startServer(usic_command)) {
        return;
    }

    std::optional<std::string> music_name = std::nullopt;
    music_name                            = getMusicNameFZF();

    if (!music_name.has_value()) {
        std::cerr << "failed to get music name\n";
        return;
    }
    const std::string COMMAND = std::format("{} {} {}", usic_command, Config::PLAY_LATER, music_name.value());

    int ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        std::cerr << std::format("{}: command failed with exit code {}\n", COMMAND, ret_code);
    }
}

} // namespace Tools
