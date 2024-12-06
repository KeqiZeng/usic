#include "client.h"
#include "fmt/core.h"
#include "runtime.h"

#include <memory>
#include <string>

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

void client(int argc, char* argv[])
{
    auto pipe_to_server = std::make_unique<NamedPipe>(PIPE_TO_SERVER);
    auto pipe_to_client = std::make_unique<NamedPipe>(PIPE_TO_CLIENT);

    pipe_to_server->openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client->openPipe(OpenMode::RD_ONLY_BLOCK);

    // send command to server
    const std::string CMD = concatenateArgs(argc, argv);
    pipe_to_server->writeIn(CMD);

    // receive response from server
    std::string msg;
    while (true) {
        msg = pipe_to_client->readOut();
        if (msg != "") {
            if (msg == OVER) {
                break;
            }
            if (msg != NO_MESSAGE) {
                fmt::print("{}\n", msg);
            }
            pipe_to_server->writeIn(GOT);
        }
    }
}