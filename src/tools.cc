#include "tools.h"

#include "fmt/core.h"
#include "runtime.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace tools
{

static bool createPlayList(std::string_view default_list, const std::filesystem::path& play_list_path, Config* config)
{
    const std::string COMMAND = fmt::format("fzf --multi < {} > {}", default_list, play_list_path.string());
    int ret_code              = std::system(COMMAND.c_str());
    if (ret_code != 0) {
        fmt::print(stderr, "command failed with exit code {}\n", ret_code);
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
        fmt::print(stderr, "command failed with exit code: {}\n", ret_code);
        return;
    }

    std::ifstream infile(BLANK_FILE);
    if (!infile.is_open()) {
        fmt::print(stderr, "failed to open temporary file: {}\n", BLANK_FILE);
        return;
    }

    std::string line;
    std::ofstream outfile(play_list_path, std::ios::app);
    if (!outfile.is_open()) {
        fmt::print(stderr, "failed to open playlist file: {}\n", play_list_path.string());
        return;
    }
    while (std::getline(infile, line)) {
        // check if the line already exists in the file
        if (std::filesystem::exists(play_list_path)) {
            if (!utils::isLineInFile(line, play_list_path.string())) { outfile << line << '\n'; }
        }
    }
    outfile.close();
    infile.close();
    utils::deleteTmpFiles();
}

} // namespace tools