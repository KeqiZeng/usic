#pragma once
#include <memory>
#include <string>
#include <vector>

class MusicNode
{
  public:
    MusicNode() = default;
    MusicNode(std::string_view music);

  private:
    const std::string music_;
    std::shared_ptr<MusicNode> prev_{nullptr};
    std::shared_ptr<MusicNode> next_{nullptr};

    friend class MusicList;
};

class MusicList
{
  public:
    ~MusicList();

    void load(std::string_view list_path);
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] int getCount() const;
    [[nodiscard]] std::vector<std::string> getList();
    void insertAfterTail(std::string_view music);
    void insertAfterCurrent(std::string_view music);
    void forward();
    void backward();
    void shuffle();
    bool moveTo(std::string_view music);
    void updateCurrent();
    void single();
    [[nodiscard]] const std::optional<std::string> getMusic() const;
    void clear();

  private:
    std::shared_ptr<MusicNode> head_{nullptr};
    std::shared_ptr<MusicNode> tail_{nullptr};
    std::shared_ptr<MusicNode> current_{nullptr};
    std::shared_ptr<MusicNode> next_to_play_{nullptr};
    int count_{0};
};