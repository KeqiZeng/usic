#include <cmath>
#include <cstring>
#include <exception>
#include <sys/_types/_pid_t.h>
#include <unistd.h>

#include "client.h"
#include "fmt/core.h"
#include "runtime.h"
#include "server.h"

/* marcos */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_COREAUDIO
#define MA_ENABLE_ALSA
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

// miniaudio should be included after MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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
            fmt::print("Server is already running\n");
            return 0;
        }

        pid_t pid = fork();
        if (pid < 0) {
            // Forking failed
            log("forking child process failed", LogType::ERROR, __func__);
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
    if (argc > 1) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            fmt::print("{}\n", VERSION);
            return 0;
        }

        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            fmt::print("{}", DOC);
            return 0;
        }

        if (!has_server) {
            fmt::print("Server is not running\n");
            return 1;
        }
        try {
            client(argc, argv);
        }
        catch (std::exception& e) {
            return FATAL_ERROR;
        }
        return 0;
    }
}