// #pragma once
// #include "linkedlist/linkedlist.h"
// #include "lock/shm_lock.h"
// #include "shm/mempool.h"

// template <typename KeyType, typename ValueType, typename LockType = ShmLock>
// class LockListPerNode : public LLBase<KeyType, ValueType> {
//  public:
//   struct alignas(CACHE_LINE_SIZE) node_t {
//     KeyType key;
//     ValueType val;
//     LockType lock;
//     volatile node_t* next;
//     node_t() : key(NULL), val(NULL), lock(), next(NULL) {}
//     node_t(KeyType key, ValueType val, node_t* next)
//         : key(key), val(val), lock(), next(next) {}

//     void* operator new(size_t size) {
//       void* ptr;
//       if (cacheable.clalign(memkind_pool, &ptr, size) != 0) {
//         throw std::bad_alloc();
//       }
//       return ptr;
//     }

//     void operator delete(void* ptr) noexcept {
//       memkind_free(memkind_pool, ptr);
//     }
//   };

//   struct alignas(CACHE_LINE_SIZE) list_t {
//     node_t *head, *tail;
//     list_t() : {
//       head = new node_t();
//       tail = new node_t();
//       head->next = tail;
//     }
//     void* operator new(size_t size) {
//       void* ptr;
//       if (cacheable.clalign(memkind_pool, &ptr, size) != 0) {
//         throw std::bad_alloc();
//       }
//       return ptr;
//     }
//     void operator delete(void* ptr) noexcept {
//       memkind_free(memkind_pool, ptr);
//     }
//   };

//   LockListPerNode() { the_list = new list_t(); }

//   bool find(const KeyType key, ValueType* val) override {
//     node_t* current;

//     if (the_list == nullptr) {
//       return false;  // Return NULL if the list or key is invalid
//     }

//     current = the_list->head->next;

//     while (current != the_list->tail) {
//       current->lock.lock();
//       // Compare the current node's key with the provided key
//       if (key_equal(current->key, key)) {
//         *val = current->value;
//         current->lock.unlock();
//         return true;  // Return the value if the key matches
//       }
//       current->lock.unlock();
//       current = current->next;
//     }
//     return false;
//   }

//   void insert(const KeyType key, ValueType val) override {
//     node_t* head = the_list->head;

//     head->lock.lock();

//     // prev->key < key < current->key
//     node_t* new_elem = new node(key, val, head->next);
//     head->next = new_elem;

//     head->lock.unlock();
//     return true;
//   }

//   void remove(const KeyType key) override {}

//   void destroy() override {}

//  private:
//   linkedlist_t* the_list;
// };