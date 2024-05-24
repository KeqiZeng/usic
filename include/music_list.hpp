#pragma once

#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "config.hpp"
#include "constants.hpp"
#include "fmt/core.h"
#include "runtime.hpp"

class MusicNode {
  friend class MusicList;

 private:
  const std::string music;
  std::shared_ptr<MusicNode> prev{nullptr};
  std::shared_ptr<MusicNode> next{nullptr};

 public:
  MusicNode() = default;
  MusicNode(std::string _music) : music(std::move(_music)){};
  ~MusicNode() = default;

  auto get_music() -> std::string { return this->music; }
};

// three situations:
// 1. musicList->head == musicList->tail == NULL; no musicNode in List
// 2. musicList->head == musicList->tail != NULL; only one musicNode in List
// 3. musicList->head != musicList->tail, head != NULL && tail != NULL,
// head->prev == NULL, tail->next == NULL
class MusicList {
 private:
  std::shared_ptr<MusicNode> head{nullptr};
  std::shared_ptr<MusicNode> tail{nullptr};
  int count{0};

 public:
  MusicList() = default;
  MusicList(Config* config, const std::string& listFile) {
    std::ifstream file(listFile.c_str());
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        line = fmt::format("{}{}", config->get_usic_library(), line);
        this->tail_in(line);
      }
      file.close();
    } else {
      log(fmt::format("Failed to open file: {}", listFile), LogType::ERROR);
    }
  }
  ~MusicList() {
    auto current = this->head;
    while (current) {
      auto temp = current;
      current = current->next;
      temp->prev.reset();
      temp->next.reset();
      temp.reset();
    }
  }

  auto load(const std::string& listPath, Config* config,
            bool reload = true) -> void {
    if (reload) {
      this->clear();
    }
    std::ifstream file(listPath.c_str());
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        line = fmt::format("{}{}", config->get_usic_library(), line);
        this->tail_in(line);
      }
      file.close();
    } else {
      log(fmt::format("Failed to open file: {}", listPath), LogType::ERROR);
    }
  }

  [[nodiscard]] auto is_empty() const -> bool { return this->count == 0; }
  [[nodiscard]] auto get_count() const -> int { return this->count; }
  [[nodiscard]] auto get_list() -> std::vector<std::string> {
    std::vector<std::string> list;
    MusicNode* current = this->head.get();
    while (current != nullptr) {
      list.push_back(current->music);
      current = current->next.get();
    }
    return list;
  }

  // auto queue(const std::string& music) -> void {
  auto tail_in(const std::string& music) -> void {
    auto newNode = std::make_shared<MusicNode>(music);
    if (!this->head) {
      this->head = std::move(newNode);
      this->tail = this->head;
    } else {
      newNode->prev = this->tail;
      this->tail->next = std::move(newNode);
      this->tail = this->tail->next;
    }
    this->count += 1;
  }

  auto head_out() -> std::shared_ptr<MusicNode> {
    if (!this->head) {
      return nullptr;
    }
    auto result = this->head;
    this->head = this->head->next;
    if (!this->head) {
      this->tail = nullptr;
    } else {
      result->next = nullptr;
      this->head->prev = nullptr;
    }
    this->count -= 1;
    return result;
  }

  auto tail_out() -> std::shared_ptr<MusicNode> {
    if (!this->head) {
      return nullptr;
    }
    auto result = this->tail;
    this->tail = this->tail->prev;
    if (!this->tail) {
      this->head = nullptr;
    } else {
      result->prev = nullptr;
      this->tail->next = nullptr;
    }
    this->count -= 1;
    return result;
  }

  auto head_in(const std::string& music) -> void {
    auto newNode = std::make_shared<MusicNode>(music);
    if (!this->head) {
      this->head = std::move(newNode);
      this->tail = head;
    } else {
      newNode->next = this->head;
      this->head->prev = newNode;
      this->head = std::move(newNode);
    }
    this->count += 1;
  }

  auto random_out() -> std::shared_ptr<MusicNode> {
    if (this->count == 0) {
      return nullptr;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shared_ptr<MusicNode> result(nullptr);
    if (this->count < 100) {
      std::uniform_int_distribution<int> dist(0, this->count - 1);
      result = this->head;
      for (int i = 0; i < dist(gen); i++) {
        result = result->next;
      }
    } else {
      std::uniform_int_distribution<int> dist(1, 100);
      int index = dist(gen);
      if (index < 50) {
        result = this->head;
        for (int i = 0; i < index + 10; i++) {
          result = result->next;
        }
      } else {
        result = this->tail;
        for (int i = 0; i < index + 10; i++) {
          result = result->prev;
        }
      }
    }

    if (result == this->head) {
      this->head = this->head->next;
      if (this->head) {
        this->head->prev = nullptr;
      }
      result->next = nullptr;
    } else if (result == this->tail) {
      this->tail = this->tail->prev;
      if (this->tail) {
        this->tail->next = nullptr;
      }
      result->prev = nullptr;
    } else {
      result->prev->next = result->next;
      result->next->prev = result->prev;
      result->prev = nullptr;
      result->next = nullptr;
    }
    this->count -= 1;
    return result;
  }

  auto contain(const std::string& music) -> bool {
    auto current = this->head;
    while (current) {
      if (current->get_music() == music) {
        return true;
      }
      current = current->next;
    }
    return false;
  }

  auto remove(const std::string& music) -> bool {
    auto current = this->head;
    while (current) {
      if (current->get_music() == music) {
        if (current == this->head) {
          this->head = this->head->next;
          if (this->head) {
            this->head->prev = nullptr;
          }
          current->next = nullptr;
        } else if (current == this->tail) {
          this->tail = this->tail->prev;
          if (this->tail) {
            this->tail->next = nullptr;
          }
          current->prev = nullptr;
        } else {
          current->prev->next = current->next;
          current->next->prev = current->prev;
          current->prev = nullptr;
          current->next = nullptr;
        }
        current.reset();
        this->count -= 1;
        return true;
      }
      current = current->next;
    }
    return false;
  }

  auto clear() -> void {
    auto current = this->head;
    while (current) {
      auto temp = current;
      current = current->next;
      temp->prev.reset();
      temp->next.reset();
      temp.reset();
    }
    this->head = nullptr;
    this->tail = nullptr;
    this->count = 0;
  }
};
