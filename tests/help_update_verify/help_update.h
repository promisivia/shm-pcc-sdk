#ifndef HELP_UPDATE_H
#define HELP_UPDATE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>

// 假设指针是8字节对齐的，所以最后一位总是0
// 使用最后一位作为锁位：1表示有正在进行的副本更新

// 清除锁位（设置最后一位为0）
inline uintptr_t clear_lock_bit(uintptr_t ptr) {
  return ptr & ~static_cast<uintptr_t>(1);
}

// 设置锁位（设置最后一位为1）
inline uintptr_t set_lock_bit(uintptr_t ptr) {
  return ptr | static_cast<uintptr_t>(1);
}

// 检查是否有锁位
inline bool has_lock_bit(uintptr_t ptr) {
  return (ptr & static_cast<uintptr_t>(1)) != 0;
}

// 获取实际的指针值（清除锁位）
template<typename T>
inline T* get_actual_ptr(uintptr_t ptr) {
  return reinterpret_cast<T*>(clear_lock_bit(ptr));
}

// 帮助更新机制类
template<typename T>
class HelpUpdateMechanism {
public:
  // 全局指针（使用最后一位作为锁位）
  std::atomic<uintptr_t> global_ptr;
  
  // 每个线程的副本指针数组
  std::vector<std::atomic<uintptr_t>> replica_ptrs;
  
  // 线程数量
  size_t num_threads;
  
  HelpUpdateMechanism(size_t num_threads) 
    : global_ptr(0), replica_ptrs(num_threads), num_threads(num_threads) {
    // 初始化所有副本为0
    for (size_t i = 0; i < num_threads; ++i) {
      replica_ptrs[i].store(0, std::memory_order_relaxed);
    }
  }
  
  // 更新全局指针（使用CAS）
  // 更新者负责更新所有副本，确保一致性
  // 返回是否成功
  bool update_global_ptr(T* new_ptr) {
    uintptr_t new_val = reinterpret_cast<uintptr_t>(new_ptr);
    uintptr_t expected = global_ptr.load(std::memory_order_acquire);
    
    do {
      // 如果当前有锁位，先帮助更新（等待其他更新完成）
      if (has_lock_bit(expected)) {
        help_update_all_replicas();
        expected = global_ptr.load(std::memory_order_acquire);
        continue;
      }
      
      // 尝试CAS更新：设置新值 + 锁位
      uintptr_t new_val_with_lock = set_lock_bit(new_val);
      if (global_ptr.compare_exchange_weak(expected, new_val_with_lock, 
                                          std::memory_order_acq_rel)) {
        // CAS成功，现在更新所有副本
        uintptr_t current_global = new_val_with_lock;
        
        // 持续更新直到所有副本都一致
        while (true) {
          uintptr_t global_without_lock = clear_lock_bit(current_global);
          bool all_updated = true;
          
          // 检查并更新所有副本
          for (size_t i = 0; i < num_threads; ++i) {
            uintptr_t replica = replica_ptrs[i].load(std::memory_order_relaxed);
            if (replica != global_without_lock) {
              replica_ptrs[i].store(global_without_lock, std::memory_order_relaxed);
              all_updated = false;
            }
          }
          
          // 检查global_ptr是否在更新过程中被修改
          uintptr_t new_global = global_ptr.load(std::memory_order_acquire);
          if (new_global != current_global) {
            // global_ptr被修改了，重新开始
            current_global = new_global;
            continue;
          }
          
          // 如果所有副本都更新完成，清除锁位
          if (all_updated) {
            uintptr_t unlocked = clear_lock_bit(current_global);
            if (global_ptr.compare_exchange_weak(current_global, unlocked,
                                                std::memory_order_acq_rel)) {
              return true;
            } else {
              // CAS失败，说明global_ptr又被修改了，重新开始
              current_global = global_ptr.load(std::memory_order_acquire);
              continue;
            }
          }
        }
      }
      
      // CAS失败，重试
    } while (true);
  }
  
  // 帮助更新所有副本
  // 只有在锁位被设置时才执行帮助更新
  void help_update_all_replicas() {
    uintptr_t current_global = global_ptr.load(std::memory_order_acquire);
    
    // 如果锁位没有被设置，没有需要帮助的更新，直接返回
    if (!has_lock_bit(current_global)) {
      return;
    }
    
    // 持续更新直到所有副本都一致，且锁位被清除
    while (true) {
      uintptr_t global_without_lock = clear_lock_bit(current_global);
      bool all_updated = true;
      
      // 检查并更新所有副本
      for (size_t i = 0; i < num_threads; ++i) {
        uintptr_t replica = replica_ptrs[i].load(std::memory_order_relaxed);
        if (replica != global_without_lock) {
          replica_ptrs[i].store(global_without_lock, std::memory_order_relaxed);
          all_updated = false;
        }
      }
      
      // 检查global_ptr是否在更新过程中被修改
      uintptr_t new_global = global_ptr.load(std::memory_order_acquire);
      if (new_global != current_global) {
        // global_ptr被修改了
        // 如果新的global_ptr没有锁位，说明更新已完成，返回
        if (!has_lock_bit(new_global)) {
          return;
        }
        // 否则，继续帮助新的更新
        current_global = new_global;
        continue;
      }
      
      // 如果所有副本都更新完成，尝试清除锁位
      if (all_updated) {
        uintptr_t unlocked = clear_lock_bit(current_global);
        if (global_ptr.compare_exchange_weak(current_global, unlocked,
                                            std::memory_order_acq_rel)) {
          // 成功清除锁位，帮助完成
          return;
        } else {
          // CAS失败，说明global_ptr又被修改了
          current_global = global_ptr.load(std::memory_order_acquire);
          // 如果新的global_ptr没有锁位，说明其他线程已经完成了，返回
          if (!has_lock_bit(current_global)) {
            return;
          }
          continue;
        }
      }
    }
  }
  
  // 获取当前线程的副本指针
  T* get_replica_ptr(size_t thread_id) {
    // 先检查是否有锁位
    uintptr_t global = global_ptr.load(std::memory_order_acquire);
    if (has_lock_bit(global)) {
      // 有锁位，帮助更新
      help_update_all_replicas();
      global = global_ptr.load(std::memory_order_acquire);
    }
    
    // 获取副本
    uintptr_t replica = replica_ptrs[thread_id].load(std::memory_order_relaxed);
    uintptr_t global_clear = clear_lock_bit(global);
    
    // 如果副本不一致，更新它
    if (replica != global_clear) {
      replica_ptrs[thread_id].store(global_clear, std::memory_order_relaxed);
      replica = global_clear;
    }
    
    return reinterpret_cast<T*>(replica);
  }
  
  // 读取全局指针（不触发帮助更新）
  T* read_global_ptr() {
    uintptr_t ptr = global_ptr.load(std::memory_order_acquire);
    return get_actual_ptr<T>(ptr);
  }
};

#endif // HELP_UPDATE_H

