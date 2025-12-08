#ifndef HELP_UPDATE_H
#define HELP_UPDATE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include <cassert>
#include "utils/atomic_pointer.h"
#include "utils/atomic_variable.h"

// #define cas(old_val, new_val)  (old_val, new_val)

// 辅助类：封装指针值和锁位操作
template <typename T>
class LockedPointer {
private:
  T* ptr_;

  static inline uintptr_t clear_lock_bit(uintptr_t val) {
    return val & ~static_cast<uintptr_t>(1);
  }

  static inline uintptr_t set_lock_bit(uintptr_t val) {
    return val | static_cast<uintptr_t>(1);
  }

public:
  LockedPointer(T* ptr = nullptr) : ptr_(ptr) {}

  // 从带锁位的指针创建
  static LockedPointer from_locked(T* locked_ptr) {
    uintptr_t val = reinterpret_cast<uintptr_t>(locked_ptr);
    return LockedPointer(reinterpret_cast<T*>(clear_lock_bit(val)));
  }

  // 获取清除锁位后的指针
  T* get() const { return ptr_; }
  operator T*() const { return ptr_; }

  // 获取带锁位的指针
  T* with_lock() const {
    uintptr_t val = reinterpret_cast<uintptr_t>(ptr_);
    return reinterpret_cast<T*>(set_lock_bit(val));
  }

  // 检查指针是否有锁位
  static bool has_lock(T* ptr) {
    if (ptr == nullptr) return false;
    uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
    return (val & static_cast<uintptr_t>(1)) != 0;
  }

  // 清除锁位
  static T* clear_lock(T* ptr) {
    if (ptr == nullptr) return nullptr;
    uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T*>(clear_lock_bit(val));
  }

  // 比较操作
  bool operator==(const LockedPointer& other) const {
    return ptr_ == other.ptr_;
  }

  bool operator==(T* other) const {
    return ptr_ == other;
  }
};

template <typename T>
class HelpUpdate {
private:
  // 指向全局指针的指针（使用nt类型）
  nt_pointer<T>* global_ptr_;
  // 指向副本指针数组的指针（使用nt类型）
  std::vector<nt_pointer<T>>* replica_ptrs_;
  // 副本数量
  size_t replica_num_;

  static bool current_cas(nt_pointer<T> &ptr, T* old_val, T* new_val) {
    T* expected = old_val;
    if (ptr.compare_exchange_strong(expected, new_val)) {
      return true;
    }
    return ptr.load(std::memory_order_seq_cst) == new_val;
  }

public:
  HelpUpdate(nt_pointer<T> &global_ptr,
             std::vector<nt_pointer<T>> &replica_ptrs,
             size_t replica_num)
      : global_ptr_(&global_ptr), replica_ptrs_(&replica_ptrs),
        replica_num_(replica_num) {
          printf("HelpUpdate constructor\n");
          printf("global_ptr: %p\n", global_ptr_);
          printf("replica_ptrs: %p\n", replica_ptrs_);
        }

  // 更新所有副本一轮
  // @expected_val: 期望的值
  // @start_check_id: 开始检查的线程id
  // @return: 只要返回就说明这一轮的update成功了，返回true说明expected_val没有变，返回false说明expected_val变了
  bool help_update(T* expected_val, uint64_t start_check_id) {
    LockedPointer<T> expected(expected_val);
    for (size_t i = start_check_id; i < replica_num_; ++i) {
      T* cur_global_ptr = global_ptr_->load();
      LockedPointer<T> cur_global = LockedPointer<T>::from_locked(cur_global_ptr);
      if (cur_global == expected && !LockedPointer<T>::has_lock(cur_global_ptr)) {
        return true;
      }

      T* local_r = (*replica_ptrs_)[i].load();
      LockedPointer<T> local = LockedPointer<T>::from_locked(local_r);
      if (local != expected) {
        cur_global_ptr = global_ptr_->load();
        cur_global = LockedPointer<T>::from_locked(cur_global_ptr);
        if (cur_global != expected) {
          return false;
        }
        if (!current_cas((*replica_ptrs_)[i], local_r, expected.with_lock())) {
          return false;
        }
      }
    }

    // 先尝试将全局指针设置为带锁位的值
    T* cur_global = global_ptr_->load();
    T* expected_global = cur_global;
    if (global_ptr_->compare_exchange_strong(expected_global, expected.with_lock())) {
      // 成功设置锁位，现在清除锁位
      help_update(expected_val, 0);
      return true;
    }
    // CAS失败，检查是否已经是期望值（清除锁位后）
    cur_global = global_ptr_->load();
    LockedPointer<T> final_global = LockedPointer<T>::from_locked(cur_global);
    if (final_global == expected && !LockedPointer<T>::has_lock(cur_global)) {
      return true;
    }

    return false;
  }

  // 更新全局指针
  // 算法：写G为A.1，然后依次写每个R[i]为A.1，全写完了写G为A.0
  bool cas_ptr(T* old_val, T* new_val) {
    T* cur_global = global_ptr_->load();

    if (LockedPointer<T>::has_lock(cur_global)) {
      help_update(LockedPointer<T>::clear_lock(cur_global), 0);
    }

    T* expected = old_val;
    LockedPointer<T> new_ptr(new_val);
    if (!global_ptr_->compare_exchange_strong(expected, new_ptr.with_lock())) {
      return false;
    }

    help_update(new_val, 0);
    return true;
  }

  // 加载指针（线程thread_id的副本指针）
  // 算法：
  // (1) 如果R[i]以.0结尾，直接继续
  // (2) 如果R[i]以.1结尾，帮助更新R[i+1]到R[N]为A.1
  // 返回正确的ptr（清除锁位）
  T* load_ptr(size_t thread_id) {
    T* local_r = (*replica_ptrs_)[thread_id].load();

    if (!LockedPointer<T>::has_lock(local_r)) {
      return local_r;
    }

    T* cleared_val = LockedPointer<T>::clear_lock(local_r);
    help_update(cleared_val, thread_id + 1);
    current_cas((*replica_ptrs_)[thread_id], local_r, cleared_val);
    return cleared_val;
  }
};

#endif // HELP_UPDATE_H
