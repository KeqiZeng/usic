#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"

TEST_CASE("The methods of MusicList can work properly")
{
    SECTION("MusicList can be initialized from a file")
    {
        std::unique_ptr<Config> config = std::make_unique<Config>(false, false, "./tests", "playLists/");
        std::unique_ptr<MusicList> musicList(
            new MusicList(config.get(), fmt::format("{}{}", config->getPlayListPath(), "music_list.txt"))
        );

        CHECK(musicList->getCount() == 12);
        CHECK(
            musicList->getList() == std::vector<std::string>(
                                        {"./tests/Beyond-情人.flac",
                                         "./tests/李宗盛-山丘.wav",
                                         "./tests/赵雷-南方姑娘 (Live).mp3",
                                         "./tests/Beyond-灰色轨迹.flac",
                                         "./tests/Beyond-海阔天空.wav",
                                         "./tests/Anne-Sophie Versnaeyen,Gabriel "
                                         "Saban,Philippe Briand-Shining Horizon.flac",
                                         "./tests/Beyond-谁伴我闯荡 91.flac",
                                         "./tests/Beyond-灰色轨迹 91.flac",
                                         "./tests/Beyond-冷雨夜 91.flac",
                                         "./tests/Anne-Sophie Versnaeyen,Gabriel "
                                         "Saban-Facing the Past.flac",
                                         "./tests/赵雷-我记得.wav",
                                         "./tests/Beyond-海阔天空 05.mp3"}
                                    )
        );
    }

    SECTION("Check randomOut can work properly when the count_ is less than 100")
    {
        std::unique_ptr<MusicList> musicList(new MusicList());
        musicList->tailIn("A");
        auto music_1 = musicList->randomOut();
        CHECK(music_1->getMusic() == "A");
        CHECK(musicList->getCount() == 0);

        musicList->tailIn("W");
        musicList->tailIn("X");
        musicList->tailIn("Y");
        musicList->tailIn("Z");
        auto originList = musicList->getList();
        std::sort(originList.begin(), originList.end());
        auto rOut     = musicList->randomOut()->getMusic();
        auto restList = musicList->getList();
        restList.push_back(rOut);
        std::sort(restList.begin(), restList.end());
        CHECK(restList == originList);
        CHECK(musicList->getCount() == 3);
    }

    SECTION("Check randomOut can work properly when the count_ is larger than 100")
    {
        std::unique_ptr<MusicList> musicList(new MusicList());
        for (int i = 0; i < 240; i++) {
            musicList->headIn(std::to_string(i));
        }
        auto originList = musicList->getList();
        std::sort(originList.begin(), originList.end());
        int originCount = musicList->getCount();

        auto rOut     = musicList->randomOut()->getMusic();
        auto restList = musicList->getList();
        restList.push_back(rOut);
        std::sort(restList.begin(), restList.end());
        int restCount = musicList->getCount();
        restCount += 1;
        CHECK(restList == originList);
        CHECK(originCount == restCount);
    }

    SECTION("Operations on musicList can work well")
    {
        std::unique_ptr<MusicList> musicList(new MusicList());
        CHECK(musicList->isEmpty());
        musicList->tailIn("A");
        CHECK(musicList->headOut()->getMusic() == "A");
        musicList->headIn("B");
        CHECK(musicList->tailOut()->getMusic() == "B");

        musicList->tailIn("A");
        musicList->tailIn("B");
        CHECK(musicList->getCount() == 2);
        CHECK(musicList->getList() == std::vector<std::string>({"A", "B"}));

        CHECK(musicList->headOut()->getMusic() == "A");
        CHECK(musicList->getCount() == 1);

        musicList->headIn("C");
        musicList->headIn("D");
        CHECK(musicList->getCount() == 3);
        CHECK(musicList->getList() == std::vector<std::string>({"D", "C", "B"}));

        CHECK(musicList->tailOut()->getMusic() == "B");
        CHECK(musicList->getCount() == 2);
        CHECK(musicList->getList() == std::vector<std::string>({"D", "C"}));

        CHECK(musicList->contain("C"));
        musicList->remove("C");
        CHECK(!musicList->contain("C"));
        CHECK(musicList->getCount() == 1);

        musicList->clear();
        CHECK(musicList->isEmpty());

        auto music_null = musicList->headOut();
        CHECK(music_null == nullptr);
        music_null = musicList->tailOut();
        CHECK(music_null == nullptr);
        music_null = musicList->randomOut();
        CHECK(music_null == nullptr);
    }
}