#pragma once
#include <memory>
#include <string>
#include <vector>

enum class PlayMode
{
    SEQUENCE,
    SHUFFLE,
    SINGLE,
};

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
    MusicList() = default;
    ~MusicList();
    MusicList(const MusicList&)            = delete;
    MusicList(MusicList&&)                 = delete;
    MusicList& operator=(const MusicList&) = delete;
    MusicList& operator=(MusicList&&)      = delete;

    void load(std::string_view list_path);
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] int getCount() const noexcept;
    [[nodiscard]] std::vector<std::string> getList();
    void insertAfterTail(std::string_view music);
    void insertAfterCurrent(std::string_view music);
    void forward();
    void backward();
    void shuffle();
    bool moveTo(std::string_view music) noexcept;
    void updateCurrent() noexcept;
    void single();
    [[nodiscard]] const std::optional<const std::string> getMusic() const;
    void clear() noexcept;

  private:
    std::shared_ptr<MusicNode> head_{nullptr};
    std::shared_ptr<MusicNode> tail_{nullptr};
    std::shared_ptr<MusicNode> current_{nullptr};
    std::shared_ptr<MusicNode> next_to_play_{nullptr};
    int count_{0};
};