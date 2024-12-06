#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <sys/_types/_pid_t.h>
#include <unistd.h>

#include "app.h"
#include "client.h"
#include "config.h"
#include "fmt/core.h"
#include "runtime.h"
#include "server.h"
#include "tools.h"
#include "utils.h"

/* marcos */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_COREAUDIO
#define MA_ENABLE_ALSA
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

// miniaudio should be included after MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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
                REPETITIVE,
                RANDOM,
                USIC_LIBRARY,
                PLAY_LISTS_PATH
            ); // throw fatal error
            tools::addMusicToList(args[1], config.get());
        }
        else if (utils::commandEq(args[0], FUZZY_PLAY)) {
            auto config = std::make_unique<Config>(
                REPETITIVE,
                RANDOM,
                USIC_LIBRARY,
                PLAY_LISTS_PATH
            ); // throw fatal error
            tools::fuzzyPlay(argv[0], config.get());
        }
        else {
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    bool has_server{false};
    try {
        has_server = isServerRunning();
    }
    catch (std::exception& e) {
        fmt::print("{}\n", e.what());
    }

    if (argc == 1) {
        if (has_server) {
            fmt::print(stderr, "Server is already running\n");
            return 0;
        }

        pid_t pid = fork();
        if (pid < 0) {
            // Forking failed
            LOG("forking child process failed", LogType::ERROR);
            return 1;
        }
        if (pid > 0) {
            // Parent process, exit
            return 0;
        }

        // change the name of server process
        strncpy(argv[0], "usic server", sizeof("usic server"));
        try {
            server();
        }
        catch (std::exception& e) {
            return FATAL_ERROR;
        }
        return 0;
    }

    try {
        if (flagOrTool(argc, argv)) {
            return 0;
        }
    }
    catch (std::exception& e) {
        return FATAL_ERROR;
    }

    if (!has_server) {
        fmt::print(stderr, "Server is not running\n");
        return FATAL_ERROR;
    }
    try {
        client(argc, argv);
    }
    catch (std::exception& e) {
        return FATAL_ERROR;
    }
    return 0;
}