#pragma once
#include <fstream>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "runtime.h"

class MusicNode {
  friend class MusicList;

 private:
  const std::string music;
  std::shared_ptr<MusicNode> prev{nullptr};
  std::shared_ptr<MusicNode> next{nullptr};

 public:
  MusicNode() = default;
  MusicNode(std::string _music);
  // ~MusicNode() = default;

  [[nodiscard]] const std::string get_music() const;
};

class MusicList {
 private:
  std::shared_ptr<MusicNode> head{nullptr};
  std::shared_ptr<MusicNode> tail{nullptr};
  int count{0};

 public:
  MusicList() = default;
  MusicList(Config* config, const std::string& listFile);
  ~MusicList();

  void load(const std::string& listPath, Config* config, bool reload = true);
  [[nodiscard]] bool is_empty() const;
  [[nodiscard]] int get_count() const;
  [[nodiscard]] std::vector<std::string> get_list();
  void tail_in(const std::string& music);
  std::shared_ptr<MusicNode> head_out();
  std::shared_ptr<MusicNode> tail_out();
  void head_in(const std::string& music);
  std::shared_ptr<MusicNode> random_out();
  bool contain(const std::string& music);
  bool remove(const std::string& music);
  void clear();
};