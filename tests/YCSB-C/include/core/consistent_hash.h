#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>

#include "core/utils.h"

namespace ycsbc {
class HashRing {
 public:
  HashRing() {};
  HashRing(uint32_t server_nr)
      : server_nr_(server_nr), virtual_nodes_(server_nr) {
    for (uint32_t i = 0; i < server_nr_; i++) {
      AddNode(i);
    }
  }
  void AddNode(uint32_t node) {
    virtual_nodes_[node] = std::vector<uint64_t>();
    for (uint32_t i = 0; i < VIRT_uint32_tODE_MULTIPLIER; i++) {
      uint64_t hash_point = utils::Random64();
      ring_[hash_point] = node;
      virtual_nodes_[node].push_back(hash_point);
    }
  }
  void RemoveNode(uint32_t node) {
    for (auto n : virtual_nodes_[node]) {
      ring_.erase(n);
    }
    virtual_nodes_.erase(node);
  }
  uint32_t GetNode(uint64_t key) {
    uint64_t hash_key = utils::Hash(key);
    auto it = ring_.lower_bound(hash_key);
    if (it == ring_.end()) {
      return ring_.begin()->second;
    } else {
      return it->second;
    }
  }
  uint32_t NextNode(uint32_t node) { return (node + 1) % server_nr_; }

 private:
  static const uint32_t VIRT_uint32_tODE_MULTIPLIER = 64;
  uint32_t server_nr_;
  std::map<uint64_t, uint32_t> ring_;
  std::unordered_map<uint32_t, std::vector<uint64_t>> virtual_nodes_;
};

// Check https://arxiv.org/pdf/1406.2294, not consecutive, can not be used in scan
inline int32_t JumpConsistentHash(uint64_t key, int32_t num_buckets) {
  int64_t b = -1, j = 0;
  while (j < num_buckets) {
    b = j;
    key = key * 2862933555777941757ULL + 1;
    j = (b + 1) * (double(1LL << 31) / double((key >> 33) + 1));
  }
  return b;
}
}  // namespace ycsbc