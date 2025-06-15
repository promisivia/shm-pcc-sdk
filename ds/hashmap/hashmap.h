#pragma once
#include <vector>

#include "linkedlist/linkedlist.h"
#include "shm/mempool.h"

template <typename KeyType, typename ValueType,
          template <typename, typename, typename...> class _ListType,
          typename... ListArgs>
class HashMap {
  using ListType = _ListType<KeyType, ValueType, ListArgs...>;

 public:
  HashMap() {
    hashmap_ = (void **)cacheable.malloc(memkind_pool, BARREL * sizeof(void *));
    for (int i = 0; i < BARREL; i++) {
      lists.emplace_back(ListType());
      hashmap_[i] = lists[i].the_list;
    }
  }
  HashMap(void **hashmap) : hashmap_(hashmap) {
    for (int i = 0; i < BARREL; i++) {
      lists.emplace_back(ListType(hashmap_[i]));
    }
  }
  bool get(KeyType key, ValueType *val) {
    return lists[hash(key)].find(key, val);
  }
  void insert(KeyType key, ValueType value) {
    lists[hash(key)].insert(key, value);
  }
  void remove(KeyType key) { lists[hash(key)].remove(key); }
  ~HashMap() {}

  const int BARREL = 4096;

  void **hashmap_;
  std::vector<ListType> lists;

 protected:
  int hash(KeyType key) {
    if constexpr (std::is_same<KeyType, const char *>::value) {
      unsigned long hash = 5381;  // Starting value for the hash
      int c;
      while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
      }
      return hash % BARREL;  // Simple modulo hash function
    } else if constexpr (std::is_integral<KeyType>::value) {
      return key % BARREL;  // For integers, just return the value modulo BARREL
    } else {
      static_assert(false, "Unsupported type for hash function");
    }
  }
};