#ifndef HELP_UPDATE_H
#define HELP_UPDATE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include <cassert>

#define cas(old_val, new_val) compare_exchange_weak(old_val, new_val)

static inline bool current_cas(std::atomic<uintptr_t> &ptr, uintptr_t old_val, uintptr_t new_val) {
  if (ptr.cas(old_val, new_val)) {
    return true;
  } else {
    if (ptr.load() == new_val) {
      return true;
    } else {
      return false;
    }
  }
}

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

// 帮助更新机制类
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

  // 更新所有副本一轮
  // @expected_val: 期望的值
  // @start_check_id: 开始检查的线程id
  // @return: 只要返回就说明这一轮的update成功了，返回true说明expected_val没有变，返回false说明expected_val变了
  bool help_update(uint64_t expected_val, uint64_t start_check_id) {
    // 从第start_check_id线程开始，依次帮助更新
    for (size_t i = start_check_id; i < num_threads; ++i) {
      // (1) 检查global_ptr是否没有锁位且没有被修改，如果满足则返回
      uintptr_t cur_global_ptr = global_ptr.load();
      if (clear_lock_bit(cur_global_ptr) == expected_val && !has_lock_bit(cur_global_ptr)) {
        return true;
      }
      // (2) 尝试依次更新每个replica
      uintptr_t local_r = replica_ptrs[i].load();
      if (clear_lock_bit(local_r) != expected_val) {
        // 需要帮助更新
        // (2.1) 先判断是不是因为global变了导致的这个replica被修改
        cur_global_ptr = global_ptr.load();
        if (clear_lock_bit(cur_global_ptr) != expected_val) {
          // 立刻返回失败
          return false;
        }
        // (2.2) 不是因为global变了导致的这个replica被修改，说明这个replica更旧，需要帮助更新
        if (!current_cas(replica_ptrs[i], local_r, set_lock_bit(expected_val))) {
          // 帮忙失败，返回当前replica的值
          return false;
        }
      }
    }

    // 第一个成功帮助所有人更新的人，修改全局的global_ptr为expected_val.0
    if (current_cas(global_ptr, set_lock_bit(expected_val), expected_val)) {
      // !!! COMMIT POINT of writer !!!
      return true;
    } else {
      // CAS失败，说明global_ptr被其他人更新了，重新开始
      return false;
    }
  }
  
  // 更新全局指针
  // 算法：写G为A.1，然后依次写每个R[i]为A.1，全写完了写G为A.0
  bool cas_ptr(uintptr_t old_val, uintptr_t new_val) {
    uintptr_t cur_global = global_ptr.load();
    
    // (0) 如果当前有锁位，先帮助更新（等待其他更新完成）
    if (has_lock_bit(cur_global)) {
      help_update(clear_lock_bit(cur_global), 0);
    }

    // (1) CAS全局指针为new_val.1
    if (!global_ptr.cas(old_val, set_lock_bit(new_val))) {
      // CAS失败，直接返回false交给外面的逻辑处理
      return false;
    }
    // (2) CAS成功（commit point）
    // (2.1) 依次写每个R[i]为A.1
    help_update(new_val, 0);
    // CAS失败，且不是因为有人提前清理了global_ptr；说明global_ptr被修改了
    // 但只可能是因为这一轮的global已经成功更新，但又被改了，还是可以返回true
    return true;
  }
  
  // 加载指针（线程thread_id的副本指针）
  // 算法：
  // (1) 如果R[i]以.0结尾，直接继续
  // (2) 如果R[i]以.1结尾，帮助更新R[i+1]到R[N]为A.1
  // 返回正确的ptr（清除锁位）
  uintptr_t load_ptr(size_t thread_id) {
    // 读取自己的replica
    uintptr_t local_r = replica_ptrs[thread_id].load();
      
    // Case (1): 如果R[i]以.0结尾，说明当前的R[i]不是更新的G，可以直接从R[i]开始
    if (!has_lock_bit(local_r)) {
      // !!! COMMIT POINT of reader !!!
      return local_r;
    } else {
      // Case (2): 如果R[i]以.1结尾，帮助更新直到所有的replicas和global都是同一个值
      help_update(clear_lock_bit(local_r), thread_id + 1);
      // !!! COMMIT POINT of reader !!!
      replica_ptrs[thread_id].cas(local_r, clear_lock_bit(local_r));
      return clear_lock_bit(local_r);
    }
  }
};

#endif // HELP_UPDATE_H
