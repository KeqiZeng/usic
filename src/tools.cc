#include "tools.h"

#include "app.h"
#include "config.h"
#include "fmt/core.h"
#include "runtime.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

bool flagOrTool(int argc, char* argv[])
{
    if (argc > 1) {
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }

        if (utils::commandEq(args[0], PRINT_VERSION)) {
            fmt::print("{}\n", VERSION);
        }
        else if (utils::commandEq(args[0], PRINT_HELP)) {
            fmt::print("{}\n", DOC);
        }
        else if (utils::commandEq(args[0], ADD_MUSIC_TO_LIST)) {
            if (args.size() < 2) {
                fmt::print(stderr, "not enough arguments\n");
            }
            auto config = std::make_unique<Config>(
                USIC_LIBRARY,
                PLAY_LISTS_PATH,
                DEFAULT_PLAY_MODE
            ); // throw fatal error
            tools::addMusicToList(args[1], config.get());
        }
        else if (utils::commandEq(args[0], FUZZY_PLAY)) {
            auto config = std::make_unique<Config>(
                USIC_LIBRARY,
                PLAY_LISTS_PATH,
                DEFAULT_PLAY_MODE
            ); // throw fatal error
            tools::fuzzyPlay(argv[0], config.get());
        }
        else if (utils::commandEq(args[0], FUZZY_PLAY_LATER)) {
            auto config = std::make_unique<Config>(
                USIC_LIBRARY,
                PLAY_LISTS_PATH,
                DEFAULT_PLAY_MODE
            ); // throw fatal error
            tools::fuzzyPlayLater(argv[0], config.get());
        }
        else {
            return false;
        }
    }
    return true;
}

namespace tools
{

static bool isFZFavailable()
{
    int ret_code = std::system("fzf --version > /dev/null 2>&1");
    if (ret_code != 0) {
        fmt::print(stderr, "fzf is not installed\n");
        return false;
    }
    return true;
}

static bool startServer(std::string_view usic_command)
{
    const std::string COMMAND = fmt::format("{} > /dev/null 2>&1", usic_command);
    int ret_code              = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "failed to start usic server\n");
        return false;
    }
    return true;
}

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

static std::optional<std::string> getMusicNameFZF(Config* config)
{
    std::string music_list_name{};
    if (DEFAULT_PLAY_LIST.empty()) {
        const std::string DEFAULT_LIST = utils::createTmpDefaultList();
        music_list_name                = DEFAULT_LIST;
    }
    else {
        music_list_name = fmt::format("{}{}", config->getPlayListPath(), DEFAULT_PLAY_LIST);
    }

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

void addMusicToList(std::string_view list_name, Config* config)
{
    if (!isFZFavailable()) {
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

    int ret_code = std::system(COMMAND.c_str());
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

void fuzzyPlay(std::string_view usic_command, Config* config)
{
    if (!isFZFavailable()) {
        return;
    }

    if (!startServer(usic_command)) {
        return;
    }

    std::optional<std::string> music_name = std::nullopt;
    music_name                            = getMusicNameFZF(config);

    if (!music_name.has_value()) {
        fmt::print(stderr, "failed to get music name\n");
        return;
    }
    const std::string COMMAND = fmt::format("{} {} {}", usic_command, PLAY, music_name.value());

    int ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code {}\n", COMMAND, ret_code);
    }
}

void fuzzyPlayLater(std::string_view usic_command, Config* config)
{
    if (!isFZFavailable()) {
        return;
    }

    if (!startServer(usic_command)) {
        return;
    }

    std::optional<std::string> music_name = std::nullopt;
    music_name                            = getMusicNameFZF(config);

    if (!music_name.has_value()) {
        fmt::print(stderr, "failed to get music name\n");
        return;
    }
    const std::string COMMAND = fmt::format("{} {} {}", usic_command, PLAY_LATER, music_name.value());

    int ret_code = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "{}: command failed with exit code {}\n", COMMAND, ret_code);
    }
}

} // namespace tools