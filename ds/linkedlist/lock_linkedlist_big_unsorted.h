#pragma once
#include "linkedlist/lock_linkedlist_big.h"

template <typename KeyType, typename ValueType, typename LockType = ShmLock>
class LockListBigUnsorted : public LockListBig<KeyType, ValueType, LockType> {
 public:
  using LockListBig<KeyType, ValueType, LockType>::LockListBig;
  using node_t = typename LockListBig<KeyType, ValueType, LockType>::node_t;
  using list_t = typename LockListBig<KeyType, ValueType, LockType>::list_t;
  bool find(const KeyType key, ValueType* val) override {
    if (this->the_list == nullptr) {
      return false;
    }

    node_t* current;

    this->lock->r_lock();

    current = this->the_list->head->next;
    while (current != this->the_list->tail) {
      if (key_equal(current->key, key)) {
        assign_to_local(*val, current->val);
        this->lock->r_unlock();
        return true;
      }
      current = current->next;
    }
    this->lock->r_unlock();
    return false;
  }

  void insert(const KeyType key, ValueType val) override {
    node_t* head = this->the_list->head;

    this->lock->lock();

    node_t* new_elem = new node_t(key, val, head->next);
    head->next = new_elem;

    this->lock->unlock();
  }
};