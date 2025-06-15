#pragma once
#include <limits>

#include "linkedlist/linkedlist.h"
#include "shm/mempool.h"
#include "shm/mm.h"
#include "utils/atomic_variable.h"

template <typename KeyType, typename ValueType>
class LockFreeList : public LLBase<KeyType, ValueType> {
 public:
  struct alignas(CACHE_LINE_SIZE) node_t {
    KeyType key;    // Pointer to the key (dynamic allocation)
    ValueType val;  // Pointer to the value
    nt<node_t*> next;  // Pointer to the next node
    node_t() : key(), val(), next() {}
    node_t(KeyType key, ValueType val, node_t* next)
        : key(key), val(val), next(next) {}
    void* operator new(size_t size) {
      void* ptr;
      if (cacheable.clalign(&ptr, size) != 0) {
        throw std::bad_alloc();
      }
      return ptr;
    }

    void operator delete(void* ptr) noexcept {
      cacheable.free(ptr);
    }
  };

  struct alignas(CACHE_LINE_SIZE) list_t {
    node_t *head, *tail;
    list_t() {
      head = new node_t(std::numeric_limits<ValueType>::min(), 0, NULL);
      tail = new node_t(std::numeric_limits<KeyType>::max(), 0, NULL);
      head->next.store(tail);
    }
    void* operator new(size_t size) {
      void* ptr;
      if (cacheable.clalign(&ptr, size) != 0) {
        throw std::bad_alloc();
      }
      return ptr;
    }
    void operator delete(void* ptr) noexcept {
      cacheable.free( ptr);
    }
  };

  LockFreeList() { the_list = new list_t(); }
  LockFreeList(void* list) { the_list = (list_t*)list; }

  bool find(const KeyType key, ValueType* val) override {
    node_t* next = the_list->head->next.load();
    node_t* iterator = (node_t*)get_unmarked_ref(next);
    while (iterator != the_list->tail) {
      if (!is_marked_ref(iterator->next.load()) && iterator->key == key) {
        *val = iterator->val;
        return true;
      }
      iterator = (node_t*)get_unmarked_ref(iterator->next.load());
    }
    return false;
  }
  void insert(const KeyType key, ValueType val) override {
    node_t* new_elem = new node_t(key, val, NULL);
    node_t* head = the_list->head;
    node_t* first = head->next.load();

    while (1) {
      new_elem->next.store(first);
      if (head->next.compare_exchange_strong(first, new_elem)) {
        return;
      }
      first = head->next.load();
    }
  }
  void remove(const KeyType key) override {}
  void destroy() override {};

  list_t* the_list;

 protected:
  // The following functions handle the low-order mark bit that indicates
  // whether a node is logically deleted (1) or not (0).
  inline bool is_marked_ref(void* i) { return (bool)((uintptr_t)i & 0x1L); }

  inline void* get_unmarked_ref(void* w) {
    return (void*)((uintptr_t)w & ~0x1L);
  }

  inline void* get_marked_ref(void* w) { return (void*)((uintptr_t)w | 0x1L); }
};