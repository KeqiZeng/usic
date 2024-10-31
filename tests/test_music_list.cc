#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "music_list.h"
#include "runtime.h"

TEST_CASE("The methods of MusicList can work properly") {
  SECTION("MusicList can be initialized from a file") {
    std::unique_ptr<Config> config =
        std::make_unique<Config>(false, false, "./tests", "playLists/");
    std::unique_ptr<MusicList> musicList(new MusicList(
        config.get(),
        fmt::format("{}{}", config->get_playList_path(), "music_list.txt")));

    CHECK(musicList->get_count() == 12);
    CHECK(
        musicList->get_list() ==
        std::vector<std::string>(
            {"./tests/Beyond-情人.flac", "./tests/李宗盛-山丘.wav",
             "./tests/赵雷-南方姑娘 (Live).mp3", "./tests/Beyond-灰色轨迹.flac",
             "./tests/Beyond-海阔天空.wav",
             "./tests/Anne-Sophie Versnaeyen,Gabriel "
             "Saban,Philippe Briand-Shining Horizon.flac",
             "./tests/Beyond-谁伴我闯荡 91.flac",
             "./tests/Beyond-灰色轨迹 91.flac", "./tests/Beyond-冷雨夜 91.flac",
             "./tests/Anne-Sophie Versnaeyen,Gabriel "
             "Saban-Facing the Past.flac",
             "./tests/赵雷-我记得.wav", "./tests/Beyond-海阔天空 05.mp3"}));
  }

  SECTION(
      "Check random_out can work properly when the count is less than 100") {
    std::unique_ptr<MusicList> musicList(new MusicList());
    musicList->tail_in("A");
    auto music_1 = musicList->random_out();
    CHECK(music_1->get_music() == "A");
    CHECK(musicList->get_count() == 0);

    musicList->tail_in("W");
    musicList->tail_in("X");
    musicList->tail_in("Y");
    musicList->tail_in("Z");
    auto originList = musicList->get_list();
    std::sort(originList.begin(), originList.end());
    auto rOut = musicList->random_out()->get_music();
    auto restList = musicList->get_list();
    restList.push_back(rOut);
    std::sort(restList.begin(), restList.end());
    CHECK(restList == originList);
    CHECK(musicList->get_count() == 3);
  }

  SECTION(
      "Check random_out can work properly when the count is larger than 100") {
    std::unique_ptr<MusicList> musicList(new MusicList());
    for (int i = 0; i < 240; i++) {
      musicList->head_in(std::to_string(i));
    }
    auto originList = musicList->get_list();
    std::sort(originList.begin(), originList.end());
    int originCount = musicList->get_count();

    auto rOut = musicList->random_out()->get_music();
    auto restList = musicList->get_list();
    restList.push_back(rOut);
    std::sort(restList.begin(), restList.end());
    int restCount = musicList->get_count();
    restCount += 1;
    CHECK(restList == originList);
    CHECK(originCount == restCount);
  }

  SECTION("Operations on musicList can work well") {
    std::unique_ptr<MusicList> musicList(new MusicList());
    CHECK(musicList->is_empty());
    musicList->tail_in("A");
    CHECK(musicList->head_out()->get_music() == "A");
    musicList->head_in("B");
    CHECK(musicList->tail_out()->get_music() == "B");

    musicList->tail_in("A");
    musicList->tail_in("B");
    CHECK(musicList->get_count() == 2);
    CHECK(musicList->get_list() == std::vector<std::string>({"A", "B"}));

    CHECK(musicList->head_out()->get_music() == "A");
    CHECK(musicList->get_count() == 1);

    musicList->head_in("C");
    musicList->head_in("D");
    CHECK(musicList->get_count() == 3);
    CHECK(musicList->get_list() == std::vector<std::string>({"D", "C", "B"}));

    CHECK(musicList->tail_out()->get_music() == "B");
    CHECK(musicList->get_count() == 2);
    CHECK(musicList->get_list() == std::vector<std::string>({"D", "C"}));

    CHECK(musicList->contain("C"));
    musicList->remove("C");
    CHECK(!musicList->contain("C"));
    CHECK(musicList->get_count() == 1);

    musicList->clear();
    CHECK(musicList->is_empty());

    auto music_null = musicList->head_out();
    CHECK(music_null == nullptr);
    music_null = musicList->tail_out();
    CHECK(music_null == nullptr);
    music_null = musicList->random_out();
    CHECK(music_null == nullptr);
  }
}