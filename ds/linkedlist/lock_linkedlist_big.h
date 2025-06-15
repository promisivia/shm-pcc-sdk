#pragma once
#include "linkedlist/linkedlist.h"
#include "lock/lock_member.h"
#include "lock/shm_lock.h"
#include "shm/mempool.h"
#include "shm/mm.h"

template <typename KeyType, typename ValueType, typename LockType = ShmLock>
class LockListBig : public LLBase<KeyType, ValueType> {
 public:
  struct alignas(CACHE_LINE_SIZE) node_t {
    KeyType key;
    ValueType val;
    node_t* next;
    node_t() : key(), val(), next() {}
    node_t(KeyType key, ValueType val, node_t* next) {
      WRITE_NT_64(&this->key, key);
      WRITE_NT_64(&this->val, val);
      WRITE_NT_64((uintptr_t*)&this->next, (uintptr_t)next);
    }

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
    typename LockTrait<LockType>::type lock;
    list_t() : lock() {
      head = new node_t();
      tail = new node_t();
      WRITE_NT_64((uintptr_t*)&head->next, (uintptr_t)tail);
    }
    void* operator new(size_t size) {
      void* ptr;
      if (cacheable.clalign( &ptr, size) != 0) {
        throw std::bad_alloc();
      }
      return ptr;
    }
    void operator delete(void* ptr) noexcept {
      cacheable.free(ptr);
    }
  };

  LockListBig() {
    the_list = new list_t();
    lock = new LockType(&(the_list->lock));
  }

  LockListBig(void* list) {
    the_list = (list_t*)list;
    lock = new LockType(&(the_list->lock));
  }

  bool find(const KeyType key, ValueType* val) = 0;

  void insert(const KeyType key, ValueType val) = 0;

  void remove(const KeyType key) override {}

  void destroy() override { delete lock; }

  LockType* lock;

  list_t* the_list;
};