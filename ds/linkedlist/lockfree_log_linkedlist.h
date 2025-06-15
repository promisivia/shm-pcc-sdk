#pragma once
#include "linkedlist/lockfree_linkedlist.h"

template <typename KeyType, typename ValueType>
class LockFreeLogList : public LockFreeList<KeyType, ValueType> {
 public:
  using LockFreeList<KeyType, ValueType>::LockFreeList;
  bool insert(const KeyType key, ValueType val) override {
    node_t* new_elem = new node_t(key, val, NULL);
    node_t* head = the_list->head;
    node_t* first = head->next;

    while (1) {
      new_elem->next = first;
      if (CAS_PTR(&(head->next), first, new_elem) == first) {
        add_stale(head);
        add_stale(new_elem);
        return;
      }
      first = head->next;
    }
  }
};