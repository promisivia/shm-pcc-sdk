#pragma once
#include "linkedlist/lockfree_linkedlist.h"

template <typename KeyType, typename ValueType>
class LockFreeSortedList : public LockFreeList<KeyType, ValueType> {
 public:
  using LockFreeList<KeyType, ValueType>::LockFreeList;
  using typename LockFreeList<KeyType, ValueType>::node_t;
  using typename LockFreeList<KeyType, ValueType>::list_t;

  struct SearchResult {
    node_t* left_node;
    node_t* right_node;
  };

  SearchResult list_search(KeyType key) {
    node_t *left_node_next, *right_node;
    left_node_next = right_node = NULL;
    SearchResult result;
    while (1) {
      clwb(this->the_list->head, sizeof(node_t));
      _mm_mfence();
      node_t* iter = this->the_list->head;
      node_t* t_next = iter->next.load();
      while (this->is_marked_ref(t_next) || (iter->key < key)) {
        if (!this->is_marked_ref(t_next)) {
          result.left_node = iter;
          left_node_next = t_next;
        }
        iter = (node_t*)this->get_unmarked_ref(t_next);
        if (iter == this->the_list->tail) break;
        t_next = iter->next.load();
      }
      right_node = iter;

      if (left_node_next == right_node) {
        if (!this->is_marked_ref(right_node->next.load())) {
          result.right_node = right_node;
          return result;
        }
      } else {
        if (result.left_node->next.compare_exchange_strong(left_node_next,
                                                           right_node)) {
          if (!this->is_marked_ref(right_node->next.load())) {
            result.right_node = right_node;
            return result;
          }
        }
      }
    }
  }

  bool find(const KeyType key, ValueType* val) override {
    node_t* iter = (node_t*)this->get_unmarked_ref(
        static_cast<node_t*>(this->the_list->head->next.load()));
    while (iter != this->the_list->tail) {
      if (!this->is_marked_ref(iter->next.load()) && iter->key >= key) {
        /* either we found it, or found the first larger element */
        *val = iter->val;
        return iter->key == key;
      }

      /* always get unmarked pointer */
      iter = (node_t*)this->get_unmarked_ref(iter->next.load());
    }
    return false;
  }

  void insert(const KeyType key, ValueType val) override {
    node_t* left_node = NULL;
    node_t* new_elem = new node_t(key, val, NULL);
    while (1) {
      SearchResult result = list_search(key);
      left_node = result.left_node;
      node_t* right_node = result.right_node;
      if (right_node != this->the_list->tail && right_node->key == key) {
        // right_node->val = val;
        // WRITE_NT_64(&right_node->val, val);
        return;
      }
      //   new_elem->next = right_node;
      WRITE_NT_64((uintptr_t*)&new_elem->next, (uintptr_t)right_node);
      if (left_node->next.compare_exchange_strong(right_node, new_elem)) {
        return;
      }
    }
  }
};