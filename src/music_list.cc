#include "music_list.h"
#include "fmt/core.h"
#include "runtime.h"

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

[[nodiscard]] const std::string MusicNode::getMusic() const
{
    return music_;
}

// three situations:
// 1. musicList->head_ == musicList->tail_ == NULL; no musicNode in List
// 2. musicList->head_ == musicList->tail_ != NULL; only one musicNode in List
// 3. musicList->head_ != musicList->tail_, head_ != NULL && tail_ != NULL,
// head_->prev == NULL, tail_->next == NULL
MusicList::MusicList(std::string_view list_file_path)
{
    std::filesystem::path list_file{list_file_path};
    std::ifstream file(list_file);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            this->tailIn(line);
        }
        file.close();
    }
    else {
        LOG(fmt::format("Failed to open file: {}", list_file_path), LogType::ERROR);
    }
}
MusicList::~MusicList()
{
    auto current = head_;
    while (current) {
        auto temp = current;
        current   = current->next_;
        temp->prev_.reset();
        temp->next_.reset();
        temp.reset();
    }
}

void MusicList::load(std::string_view list_path)
{
    this->clear();
    std::filesystem::path list{list_path};
    std::ifstream file(list);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            this->tailIn(line);
        }
        file.close();
    }
    else {
        LOG(fmt::format("Failed to open file: {}", list_path), LogType::ERROR);
    }
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
    while (current != nullptr) {
        list.push_back(current->music_);
        current = current->next_.get();
    }
    return list;
}

void MusicList::tailIn(std::string_view music)
{
    auto new_node = std::make_shared<MusicNode>(music);
    if (!head_) {
        head_ = std::move(new_node);
        tail_ = head_;
    }
    else {
        new_node->prev_ = tail_;
        tail_->next_    = std::move(new_node);
        tail_           = tail_->next_;
    }
    count_ += 1;
}

std::shared_ptr<MusicNode> MusicList::headOut()
{
    if (!head_) {
        return nullptr;
    }
    auto result = head_;
    head_       = head_->next_;
    if (!head_) {
        tail_ = nullptr;
    }
    else {
        result->next_ = nullptr;
        head_->prev_  = nullptr;
    }
    count_ -= 1;
    return result;
}

std::shared_ptr<MusicNode> MusicList::tailOut()
{
    if (!head_) {
        return nullptr;
    }
    auto result = tail_;
    tail_       = tail_->prev_;
    if (!tail_) {
        head_ = nullptr;
    }
    else {
        result->prev_ = nullptr;
        tail_->next_  = nullptr;
    }
    count_ -= 1;
    return result;
}

void MusicList::headIn(std::string_view music)
{
    auto new_node = std::make_shared<MusicNode>(music);
    if (!head_) {
        head_ = std::move(new_node);
        tail_ = head_;
    }
    else {
        new_node->next_ = head_;
        head_->prev_    = new_node;
        head_           = std::move(new_node);
    }
    count_ += 1;
}

std::shared_ptr<MusicNode> MusicList::randomOut()
{
    if (count_ == 0) {
        return nullptr;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shared_ptr<MusicNode> result(nullptr);
    if (count_ < 100) {
        std::uniform_int_distribution<int> dist(0, count_ - 1);
        result = head_;
        for (int i = 0; i < dist(gen); i++) {
            result = result->next_;
        }
    }
    else {
        std::uniform_int_distribution<int> dist(1, 100);
        int index = dist(gen);
        if (index < 50) {
            result = head_;
            for (int i = 0; i < index + 10; i++) {
                result = result->next_;
            }
        }
        else {
            result = tail_;
            for (int i = 0; i < index + 10; i++) {
                result = result->prev_;
            }
        }
    }

    if (result == head_) {
        head_ = head_->next_;
        if (head_) {
            head_->prev_ = nullptr;
        }
        result->next_ = nullptr;
    }
    else if (result == tail_) {
        tail_ = tail_->prev_;
        if (tail_) {
            tail_->next_ = nullptr;
        }
        result->prev_ = nullptr;
    }
    else {
        result->prev_->next_ = result->next_;
        result->next_->prev_ = result->prev_;
        result->prev_        = nullptr;
        result->next_        = nullptr;
    }
    count_ -= 1;
    return result;
}

bool MusicList::contain(std::string_view music)
{
    auto current = head_;
    while (current) {
        if (current->getMusic() == music) {
            return true;
        }
        current = current->next_;
    }
    return false;
}

bool MusicList::remove(std::string_view music)
{
    auto current = head_;
    while (current) {
        if (current->getMusic() == music) {
            if (current == head_) {
                head_ = head_->next_;
                if (head_) {
                    head_->prev_ = nullptr;
                }
                current->next_ = nullptr;
            }
            else if (current == tail_) {
                tail_ = tail_->prev_;
                if (tail_) {
                    tail_->next_ = nullptr;
                }
                current->prev_ = nullptr;
            }
            else {
                current->prev_->next_ = current->next_;
                current->next_->prev_ = current->prev_;
                current->prev_        = nullptr;
                current->next_        = nullptr;
            }
            current.reset();
            count_ -= 1;
            return true;
        }
        current = current->next_;
    }
    return false;
}

void MusicList::clear()
{
    auto current = head_;
    while (current) {
        auto temp = current;
        current   = current->next_;
        temp->prev_.reset();
        temp->next_.reset();
        temp.reset();
    }
    head_  = nullptr;
    tail_  = nullptr;
    count_ = 0;
}