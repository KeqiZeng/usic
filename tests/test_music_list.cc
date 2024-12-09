#include "music_list.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("The methods of MusicList can work properly")
{
    SECTION("MusicList can be initialized from a file")
    {
        auto music_list = std::make_unique<MusicList>();
        CHECK(music_list->isEmpty());

        music_list->load("./tests/playLists/music_list.txt");

        CHECK(music_list->getCount() == 12);
        CHECK(
            music_list->getList() == std::vector<std::string>(
                                         {"Beyond-情人.flac",
                                          "李宗盛-山丘.wav",
                                          "赵雷-南方姑娘 (Live).mp3",
                                          "Beyond-灰色轨迹.flac",
                                          "Beyond-海阔天空.wav",
                                          "Anne-Sophie Versnaeyen,Gabriel "
                                          "Saban,Philippe Briand-Shining Horizon.flac",
                                          "Beyond-谁伴我闯荡 91.flac",
                                          "Beyond-灰色轨迹 91.flac",
                                          "Beyond-冷雨夜 91.flac",
                                          "Anne-Sophie Versnaeyen,Gabriel "
                                          "Saban-Facing the Past.flac",
                                          "赵雷-我记得.wav",
                                          "Beyond-海阔天空 05.mp3"}
                                     )
        );
    }

    SECTION("Check insertAfterTail, forward, backward, moveTo and updateCurrent can work properly")
    {
        auto music_list = std::make_unique<MusicList>();
        music_list->insertAfterTail("A");
        music_list->insertAfterTail("B");
        music_list->insertAfterTail("C");
        music_list->insertAfterTail("D");
        music_list->insertAfterTail("E");

        CHECK(music_list->getCount() == 5);

        CHECK(music_list->getMusic().value() == "A");
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "B");
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "C");
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "D");
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "E");
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "A");

        music_list->backward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "E");
        music_list->backward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "D");
        music_list->backward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "C");
        music_list->backward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "B");
        music_list->backward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "A");

        music_list->moveTo("C");
        CHECK(music_list->getMusic().value() == "C");
        music_list->moveTo("A");
        CHECK(music_list->getMusic().value() == "A");
        music_list->moveTo("E");
        CHECK(music_list->getMusic().value() == "E");
        music_list->moveTo("B");
        CHECK(music_list->getMusic().value() == "B");
        music_list->moveTo("D");
        CHECK(music_list->getMusic().value() == "D");
    }

    SECTION("Check insertAfterCurrent and clear can work properly")
    {
        auto music_list = std::make_unique<MusicList>();
        music_list->insertAfterTail("A");
        music_list->insertAfterTail("B");
        music_list->insertAfterTail("C");
        music_list->insertAfterTail("D");
        music_list->insertAfterTail("E");

        music_list->insertAfterCurrent("F");
        CHECK(music_list->getCount() == 6);
        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "F");

        music_list->forward();
        music_list->updateCurrent();
        CHECK(music_list->getMusic().value() == "B");

        music_list->insertAfterCurrent("G");
        music_list->insertAfterCurrent("H");
        music_list->insertAfterCurrent("I");
        music_list->insertAfterCurrent("J");

        CHECK(music_list->getCount() == 10);
        CHECK(music_list->getList() == std::vector<std::string>({"A", "F", "B", "J", "I", "H", "G", "C", "D", "E"}));

        music_list->moveTo("I");
        music_list->updateCurrent();
        music_list->insertAfterCurrent("K");
        music_list->insertAfterCurrent("L");
        music_list->insertAfterCurrent("M");
        music_list->insertAfterCurrent("N");
        music_list->insertAfterCurrent("O");

        CHECK(music_list->getCount() == 15);
        CHECK(
            music_list->getList() ==
            std::vector<std::string>({"A", "F", "B", "J", "I", "O", "N", "M", "L", "K", "H", "G", "C", "D", "E"})
        );

        music_list->clear();
        CHECK(music_list->isEmpty());
    }
}
