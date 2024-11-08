#pragma once
#include <string>
#include <vector>

#include "runtime.h"

class MusicNode
{
  public:
    MusicNode() = default;
    MusicNode(std::string_view music);
    // ~MusicNode() = default;

    [[nodiscard]] const std::string getMusic() const;

  private:
    const std::string music_;
    std::shared_ptr<MusicNode> prev_{nullptr};
    std::shared_ptr<MusicNode> next_{nullptr};

    friend class MusicList;
};

class MusicList
{
  public:
    MusicList() = default;
    MusicList(Config* config, std::string_view list_file);
    ~MusicList();

    void load(std::string_view list_path, Config* config, bool reload = true);
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] int getCount() const;
    [[nodiscard]] std::vector<std::string> getList();
    void tailIn(std::string_view music);
    std::shared_ptr<MusicNode> headOut();
    std::shared_ptr<MusicNode> tailOut();
    void headIn(std::string_view music);
    std::shared_ptr<MusicNode> randomOut();
    bool contain(std::string_view music);
    bool remove(std::string_view music);
    void clear();

  private:
    std::shared_ptr<MusicNode> head_{nullptr};
    std::shared_ptr<MusicNode> tail_{nullptr};
    int count_{0};
};