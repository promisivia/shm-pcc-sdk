#pragma once
#include "linkedlist/lock_linkedlist_big.h"

template <typename KeyType, typename ValueType, typename LockType = ShmLock>
class LockListBigSorted : public LockListBig<KeyType, ValueType, LockType> {
 public:
  using LockListBig<KeyType, ValueType, LockType>::LockListBig;
  using node_t = typename LockListBig<KeyType, ValueType, LockType>::node_t;
  using list_t = typename LockListBig<KeyType, ValueType, LockType>::list_t;

  bool find(const KeyType key, ValueType* val) override {
    if (this->the_list == nullptr) {
      return false;
    }
    node_t* current;

    clwb(this->lock, sizeof(LockType));
    _mm_mfence();
    this->lock->r_lock();
    clwb(this->lock, sizeof(LockType));
    _mm_mfence();

    clwb(this->the_list->head, sizeof(node_t));
    _mm_mfence();
    current = this->the_list->head->next;
    while (current != this->the_list->tail) {
      clwb(current, sizeof(node_t));
      _mm_mfence();
      if (current->key == key) {
        *val = current->val;
        this->lock->r_unlock();
        clwb(current, sizeof(node_t));
        _mm_mfence();
        return true;
      } else if (current->key > key) {
        break;
      }
      current = current->next;
    }
    this->lock->r_unlock();
    clwb(this->lock, sizeof(LockType));
    _mm_mfence();
    return false;
  }

  void insert(const KeyType key, ValueType val) override {
    if (this->the_list == nullptr) {
      return;
    }
    node_t *current, *prev;
    clwb(this->lock, sizeof(LockType));
    _mm_mfence();
    this->lock->lock();
    clwb(this->lock, sizeof(LockType));
    _mm_mfence();

    clwb(this->the_list->head, sizeof(node_t));
    _mm_mfence();
    prev = this->the_list->head;
    current = prev->next;
    while (current != this->the_list->tail) {
      clwb(current, sizeof(node_t));
      _mm_mfence();
      if (current->key == key) {
        WRITE_NT_64(&current->val, val);
        this->lock->unlock();
        return;
      } else if (current->key > key) {
        break;
      }
      prev = current;
      current = current->next;
    }
    node_t* new_elem = new node_t(key, val, current);
    clwb(new_elem, sizeof(node_t));
    _mm_mfence();
    WRITE_NT_64((uintptr_t*)&prev->next, (uintptr_t)new_elem);
    this->lock->unlock();
    clwb(this->lock, sizeof(LockType));
    _mm_mfence();
    return;
  }
};