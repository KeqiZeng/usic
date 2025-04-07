#include "client.h"
#include "log.h"
#include "server.h"
#include "tools.h"
#include "utils.h"
#include <iostream>
#include <unistd.h>

const int FATAL_ERROR = 1;

int main(int argc, char** argv)
{
    bool has_server{false};
    try {
        has_server = Utils::isServerRunning();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return FATAL_ERROR;
    }

    if (argc == 1) {
        if (has_server) {
            std::cerr << "Server is already running\n";
            return 0;
        }

        pid_t pid = fork();
        if (pid < 0) {
            // Forking failed
            LOG("forking child process failed", LogType::ERROR);
            return FATAL_ERROR;
        }
        if (pid > 0) {
            // Parent process, exit
            return 0;
        }

        // change the name of server process
        strncpy(argv[0], "usic server", sizeof("usic server"));
        try {
            Server::getInstance().run();
        }
        catch (std::exception& e) {
            return FATAL_ERROR;
        }
        return 0;
    }

    try {
        if (handleFlagOrTool(argc, argv)) {
            return 0;
        }
    }
    catch (std::exception& e) {
        return FATAL_ERROR;
    }

    if (!has_server) {
        std::cerr << "Server is not running\n";
        return FATAL_ERROR;
    }

    try {
        Client::getInstance().run(argc, argv);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return FATAL_ERROR;
    }
    return 0;
}
