#include "music_list.h"
#include "fmt/core.h"
#include "runtime.h"

#include <cmath>
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
    return this->music_;
}

// three situations:
// 1. musicList->head_ == musicList->tail_ == NULL; no musicNode in List
// 2. musicList->head_ == musicList->tail_ != NULL; only one musicNode in List
// 3. musicList->head_ != musicList->tail_, head_ != NULL && tail_ != NULL,
// head_->prev == NULL, tail_->next == NULL
MusicList::MusicList(Config* config, std::string_view list_file_path)
{
    std::filesystem::path list_file{list_file_path};
    std::ifstream file(list_file);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = fmt::format("{}{}", config->getUsicLibrary(), line);
            this->tailIn(line);
        }
        file.close();
    }
    else {
        log(fmt::format("Failed to open file: {}", list_file_path), LogType::ERROR, __func__);
    }
}
MusicList::~MusicList()
{
    auto current = this->head_;
    while (current) {
        auto temp = current;
        current   = current->next_;
        temp->prev_.reset();
        temp->next_.reset();
        temp.reset();
    }
}

void MusicList::load(std::string_view list_path, Config* config)
{
    this->clear();
    std::filesystem::path list{list_path};
    std::ifstream file(list);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = fmt::format("{}{}", config->getUsicLibrary(), line);
            this->tailIn(line);
        }
        file.close();
    }
    else {
        log(fmt::format("Failed to open file: {}", list_path), LogType::ERROR, __func__);
    }
}

[[nodiscard]] bool MusicList::isEmpty() const
{
    return this->count_ == 0;
}
[[nodiscard]] int MusicList::getCount() const
{
    return this->count_;
}
[[nodiscard]] std::vector<std::string> MusicList::getList()
{
    std::vector<std::string> list;
    MusicNode* current = this->head_.get();
    while (current != nullptr) {
        list.push_back(current->music_);
        current = current->next_.get();
    }
    return list;
}

void MusicList::tailIn(std::string_view music)
{
    auto new_node = std::make_shared<MusicNode>(music);
    if (!this->head_) {
        this->head_ = std::move(new_node);
        this->tail_ = this->head_;
    }
    else {
        new_node->prev_    = this->tail_;
        this->tail_->next_ = std::move(new_node);
        this->tail_        = this->tail_->next_;
    }
    this->count_ += 1;
}

std::shared_ptr<MusicNode> MusicList::headOut()
{
    if (!this->head_) { return nullptr; }
    auto result = this->head_;
    this->head_ = this->head_->next_;
    if (!this->head_) { this->tail_ = nullptr; }
    else {
        result->next_      = nullptr;
        this->head_->prev_ = nullptr;
    }
    this->count_ -= 1;
    return result;
}

std::shared_ptr<MusicNode> MusicList::tailOut()
{
    if (!this->head_) { return nullptr; }
    auto result = this->tail_;
    this->tail_ = this->tail_->prev_;
    if (!this->tail_) { this->head_ = nullptr; }
    else {
        result->prev_      = nullptr;
        this->tail_->next_ = nullptr;
    }
    this->count_ -= 1;
    return result;
}

void MusicList::headIn(std::string_view music)
{
    auto new_node = std::make_shared<MusicNode>(music);
    if (!this->head_) {
        this->head_ = std::move(new_node);
        this->tail_ = head_;
    }
    else {
        new_node->next_    = this->head_;
        this->head_->prev_ = new_node;
        this->head_        = std::move(new_node);
    }
    this->count_ += 1;
}

std::shared_ptr<MusicNode> MusicList::randomOut()
{
    if (this->count_ == 0) { return nullptr; }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shared_ptr<MusicNode> result(nullptr);
    if (this->count_ < 100) {
        std::uniform_int_distribution<int> dist(0, this->count_ - 1);
        result = this->head_;
        for (int i = 0; i < dist(gen); i++) {
            result = result->next_;
        }
    }
    else {
        std::uniform_int_distribution<int> dist(1, 100);
        int index = dist(gen);
        if (index < 50) {
            result = this->head_;
            for (int i = 0; i < index + 10; i++) {
                result = result->next_;
            }
        }
        else {
            result = this->tail_;
            for (int i = 0; i < index + 10; i++) {
                result = result->prev_;
            }
        }
    }

    if (result == this->head_) {
        this->head_ = this->head_->next_;
        if (this->head_) { this->head_->prev_ = nullptr; }
        result->next_ = nullptr;
    }
    else if (result == this->tail_) {
        this->tail_ = this->tail_->prev_;
        if (this->tail_) { this->tail_->next_ = nullptr; }
        result->prev_ = nullptr;
    }
    else {
        result->prev_->next_ = result->next_;
        result->next_->prev_ = result->prev_;
        result->prev_        = nullptr;
        result->next_        = nullptr;
    }
    this->count_ -= 1;
    return result;
}

bool MusicList::contain(std::string_view music)
{
    auto current = this->head_;
    while (current) {
        if (current->getMusic() == music) { return true; }
        current = current->next_;
    }
    return false;
}

bool MusicList::remove(std::string_view music)
{
    auto current = this->head_;
    while (current) {
        if (current->getMusic() == music) {
            if (current == this->head_) {
                this->head_ = this->head_->next_;
                if (this->head_) { this->head_->prev_ = nullptr; }
                current->next_ = nullptr;
            }
            else if (current == this->tail_) {
                this->tail_ = this->tail_->prev_;
                if (this->tail_) { this->tail_->next_ = nullptr; }
                current->prev_ = nullptr;
            }
            else {
                current->prev_->next_ = current->next_;
                current->next_->prev_ = current->prev_;
                current->prev_        = nullptr;
                current->next_        = nullptr;
            }
            current.reset();
            this->count_ -= 1;
            return true;
        }
        current = current->next_;
    }
    return false;
}

void MusicList::clear()
{
    auto current = this->head_;
    while (current) {
        auto temp = current;
        current   = current->next_;
        temp->prev_.reset();
        temp->next_.reset();
        temp.reset();
    }
    this->head_  = nullptr;
    this->tail_  = nullptr;
    this->count_ = 0;
}