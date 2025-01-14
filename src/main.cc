#include "core.h"
// #include "log.h"
#include <iostream>

int main()
{
    CoreComponents& core = CoreComponents::getInstance();
    core.play("./music/李宗盛 - 山丘.wav");
    while (true) {
        std::cout << "Press n to play next song\n";
        std::cout << "Press q to quit\n";
        std::cout << "Press p to pause or resume\n";
        std::string input;
        std::getline(std::cin, input);
        if (input == "q") {
            break;
        }
        if (input == "n") {
            core.moveCursorToEnd();
        }
        else if (input == "p") {
            core.pauseOrResume();
        }
    }
    return 0;
}
