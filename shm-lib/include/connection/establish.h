#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifndef TEST_SINGLE_MACHINE
#define NUM_CLIENTS 2
#else
#define NUM_CLIENTS 1
#endif

class FollowerManager {
public:
  explicit FollowerManager(std::vector<std::string> machines);
  ~FollowerManager();
  FollowerManager(FollowerManager &&) noexcept;
  FollowerManager &operator=(FollowerManager &&) noexcept;
  FollowerManager(const FollowerManager &) = delete;
  FollowerManager &operator=(const FollowerManager &) = delete;

  void start_followers(std::function<std::string(uint32_t)> build_command);
  void stop_followers();

  static std::vector<std::string>
  split_machines(const std::string &machines_str) {
    std::string machines_str_copy = machines_str;
    std::vector<std::string> machines;
    size_t pos = 0;
    while ((pos = machines_str_copy.find(',')) != std::string::npos) {
      machines.push_back(machines_str_copy.substr(0, pos));
      machines_str_copy.erase(0, pos + 1);
    }
    if (!machines_str_copy.empty()) {
      machines.push_back(machines_str_copy);
    }
    return machines;
  }

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
