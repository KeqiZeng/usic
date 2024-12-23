#include "music_list.h"
#include "log.h"

#include <format>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

MusicNode::MusicNode(std::string_view music)
    : music_(std::move(music))
{
}

MusicList::~MusicList()
{
    this->clear();
}

void MusicList::load(std::string_view list_file_path)
{
    this->clear();
    std::filesystem::path list_file{list_file_path};
    std::ifstream file(list_file);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            this->insertAfterTail(line);
        }
        file.close();
    }
    else {
        LOG(std::format("Failed to open file: {}", list_file_path), LogType::ERROR);
    }
    current_      = head_;
    next_to_play_ = current_;
}

[[nodiscard]] bool MusicList::isEmpty() const
{
    return count_ == 0;
}
[[nodiscard]] int MusicList::getCount() const
{
    return count_;
}
[[nodiscard]] std::vector<std::string> MusicList::getList()
{
    std::vector<std::string> list;
    MusicNode* current = head_.get();
    do {
        list.push_back(current->music_);
        current = current->next_.get();
    } while (current != head_.get());

    return list;
}

void MusicList::insertAfterTail(std::string_view music)
{
    auto new_node = std::make_shared<MusicNode>(music);
    if (!head_) {
        head_ = new_node;
        tail_ = head_;

        head_->prev_ = tail_;
        head_->next_ = tail_;

        tail_->prev_ = head_;
        tail_->next_ = head_;

        current_      = head_;
        next_to_play_ = current_;
    }
    else {
        new_node->prev_ = tail_;
        tail_->next_    = new_node;
        tail_           = tail_->next_;

        tail_->next_ = head_;
        head_->prev_ = tail_;
    }
    count_ += 1;
}

void MusicList::insertAfterCurrent(std::string_view music)
{
    if (!current_) {
        LOG("current is null", LogType::ERROR);
        return;
    }
    auto new_node          = std::make_shared<MusicNode>(music);
    new_node->prev_        = current_;
    new_node->next_        = current_->next_;
    current_->next_->prev_ = new_node;
    current_->next_        = new_node;
    count_ += 1;
}

void MusicList::forward()
{
    if (!current_) {
        LOG("current is null", LogType::ERROR);
        return;
    }
    next_to_play_ = current_->next_;
}

void MusicList::backward()
{
    if (!current_) {
        LOG("current is null", LogType::ERROR);
        return;
    }
    next_to_play_ = current_->prev_;
}

void MusicList::shuffle()
{
    if (!current_) {
        LOG("current is null", LogType::ERROR);
        return;
    }

    // generate a random number between 1 and 100
    // if the number is in the first half, move forward
    // else move backward
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    int step = dist(gen);

    next_to_play_ = current_;
    if (step <= 50) {
        for (int i = 0; i < step; ++i) {
            next_to_play_ = next_to_play_->next_;
        }
    }
    else {
        for (int i = 0; i < step - 50; ++i) {
            next_to_play_ = next_to_play_->prev_;
        }
    }
}

bool MusicList::moveTo(std::string_view music)
{
    auto current = current_;
    do {
        if (current->music_ == music) {
            next_to_play_ = current;
            return true;
        }
        current = current->next_;
    } while (current != current_);
    return false;
}

void MusicList::updateCurrent()
{
    current_ = next_to_play_;
}

void MusicList::single()
{
    if (!current_) {
        LOG("current is null", LogType::ERROR);
        return;
    }
    next_to_play_ = current_;
}

const std::optional<std::string> MusicList::getMusic() const
{
    if (!next_to_play_) {
        LOG("next_to_play is null", LogType::ERROR);
        return std::nullopt;
    }
    return next_to_play_->music_;
}

void MusicList::clear()
{
    if (!head_) {
        return;
    }

    auto current = head_;
    do {
        auto temp   = current;
        current     = current->next_;
        temp->prev_ = nullptr;
        temp->next_ = nullptr;
        temp.reset();
    } while (current != head_);
    head_  = nullptr;
    tail_  = nullptr;
    count_ = 0;
}