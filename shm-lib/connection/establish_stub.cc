#include "connection/establish.h"

#include <iostream>
#include <utility>

struct FollowerManager::Impl {};

FollowerManager::FollowerManager(std::vector<std::string> machines)
    : impl_(std::make_unique<Impl>()) {
  (void)machines;
}

FollowerManager::~FollowerManager() = default;

FollowerManager::FollowerManager(FollowerManager &&) noexcept = default;

FollowerManager &FollowerManager::operator=(FollowerManager &&) noexcept = default;

void FollowerManager::start_followers(
    std::function<std::string(uint32_t)> build_command) {
  (void)build_command;
  std::cerr
      << "[FollowerManager] libssh headers/library not found at CMake time; "
         "SSH follower start is disabled. Install libssh-devel to enable.\n";
}

void FollowerManager::stop_followers() {}
