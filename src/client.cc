#include "client.h"
#include "named_pipe.h"
#include "runtime_path.h"
#include "utils.h"
#include <iostream>
#include <string>

Client::Client()
    : pipe_to_client_(NamedPipe(RuntimePath::SERVER_TO_CLIENT_PIPE))
    , pipe_from_client_(NamedPipe(RuntimePath::CLIENT_TO_SERVER_PIPE))
{
}

void Client::run(int argc, char** argv)
{
    pipe_from_client_.openPipe(OpenMode::WR_ONLY_BLOCK);
    pipe_to_client_.openPipe(OpenMode::RD_ONLY_BLOCK);

    // send command to server
    const std::string CMD = Utils::concatenateArgs(argc, argv);
    pipe_from_client_.writeIn(CMD);

    // receive responses from server
    std::string msg;
    while (true) {
        msg = pipe_to_client_.readOut();
        if (msg != "") {
            if (msg == SpecialMessages::OVER) {
                break;
            }
            if (msg != SpecialMessages::NO_MESSAGE) {
                std::cout << msg << "\n";
            }
            pipe_from_client_.writeIn(SpecialMessages::GOT);
        }
    }

    pipe_from_client_.closePipe();
    pipe_to_client_.closePipe();
}

Client& Client::getInstance() noexcept
{
    static Client instance;
    return instance;
}
