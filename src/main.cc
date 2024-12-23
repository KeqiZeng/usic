// Macros for miniaudio
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION

#include "core.h"
// #include "log.h"
#include <iostream>

int main()
{
    CoreComponents& core = CoreComponents::get();
    core.play("./music/王力宏 - 需要人陪.flac");
    while (true) {
        std::cout << "Press n to play next song\n";
        std::cout << "Press q to quit\n";
        std::cout << "Press p to pause or resume\n";
        std::string input;
        std::getline(std::cin, input);
        if (input == "q") {
            break;
        }
        else if (input == "n") {
            core.play("./music/李宗盛 - 山丘.wav");
        }
        else if (input == "p") {
            core.pauseOrResume();
        }
    }
    return 0;
}
