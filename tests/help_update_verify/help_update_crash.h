#ifndef HELP_UPDATE_H
#define HELP_UPDATE_H

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include <cassert>

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
  
  // 内部帮助更新函数：帮助更新所有副本直到锁位清除，并且要设置全局global_ptr为expected_val
  // @expected_val_first: 期望的值，lock bit没有设置
  // @start_check_id_first: 调用者线程id
  // @return: 是否成功
  uint64_t help_update(uint64_t expected_val_first, uint64_t start_check_id_first) {
    // 从第start_check_id线程开始，依次帮助更新
    // 成功条件：所有后续的replicas都更新为expected_val，或者global_ptr是expected_val.0
    // 失败条件：global_ptr不是expected_val了
    uint64_t expected_val = expected_val_first;
    uint64_t start_check_id = start_check_id_first;

restart_global:
    if (clear_lock_bit(global_ptr.load()) != expected_val) {
      start_check_id = 0;
      expected_val = clear_lock_bit(global_ptr.load());
      goto restart_global;
    }

    for (size_t i = start_check_id; i < num_threads; ++i) {
restart_single_replica:
      uintptr_t local_r = replica_ptrs[i].load();
      // 可以假设有人在一直更新，可以先check是不是已经更新成功了
      // 这里最后一位已经被清除也没问题
      if (clear_lock_bit(local_r) == expected_val) {
        continue;
      } else {
        // 需要帮助更新
        // 但必须先判断是不是因为global变了导致的这个replica被修改
        if (clear_lock_bit(global_ptr.load()) != expected_val) {
          // 立刻返回失败，需要从新的value开始
          // （TODO：其实这种情况，用上一轮的值也没问题）
          start_check_id = 0;
          expected_val = clear_lock_bit(global_ptr.load());
          goto restart_global;
        }
        // 不是因为global变了导致的这个replica被修改，可以继续帮助更新
        if (replica_ptrs[i].compare_exchange_weak(local_r, set_lock_bit(expected_val))) {
          continue;
        } else {
          // 上次check到CAS中有人更新了replica，重新load replica再试一遍
          goto restart_single_replica;
        }
      }
    }

    // 这一轮检查，所有的replica和global的值都更新为expected_val了
    uintptr_t cur_global_ptr = global_ptr.load();
    if (clear_lock_bit(cur_global_ptr) == expected_val) {
      if (global_ptr.compare_exchange_weak(cur_global_ptr, clear_lock_bit(expected_val))) {
        return expected_val;
      } else {
        if (global_ptr.load() == expected_val) {
          return expected_val;
        }
      }
    }

    // global_ptr被其他人更新了，或者CAS失败了，重新开始
    start_check_id = 0;
    expected_val = clear_lock_bit(global_ptr.load());
    goto restart_global;
  }
  
  // 更新全局指针
  // 算法：写G为A.1，然后依次写每个R[i]为A.1，全写完了写G为A.0
  void update_ptr(uintptr_t new_val) {
restart:
    uintptr_t cur_global = global_ptr.load(std::memory_order_acquire);
    uintptr_t new_val_with_lock = set_lock_bit(new_val);
    
    // 如果当前有锁位，先帮助更新（等待其他更新完成）
    if (has_lock_bit(cur_global)) {
      cur_global = help_update(clear_lock_bit(cur_global), 0);
    }

    // ASSERT: cur_global以0结尾
    // 步骤1: CAS全局指针为new_val.1
    if (!global_ptr.compare_exchange_weak(cur_global, new_val_with_lock)) {
      // CAS失败，说明global_ptr又被修改了，重新开始
      goto restart;
    }
    
    // 步骤2: 依次写每个R[i]为A.1
    for (size_t i = 0; i < num_threads; ++i) {
      uintptr_t local_r = replica_ptrs[i].load();

      // fast check
      if (clear_lock_bit(local_r) == new_val) {
        continue;
      }

      if (!replica_ptrs[i].compare_exchange_weak(local_r, new_val_with_lock)) {
        if (clear_lock_bit(replica_ptrs[i].load()) != new_val) {
          // CAS失败，且不是因为有人成功更新了replica；说明global_ptr被修改了，重新开始
          goto restart;
        }
      }
      // 其他：成功更新or已经被其他人更新
    }
      
    // 步骤3: 所有副本都更新完成，清除G的锁位 (写G为A.0)
    if (!global_ptr.compare_exchange_weak(new_val_with_lock, new_val)) {
      if (global_ptr.load() != new_val) {
        // CAS失败，且不是因为有人提前清理了global_ptr；说明global_ptr被修改了，重新开始
        goto restart;
      }
    }
  }
  
  // 加载指针（线程thread_id的副本指针）
  // 算法：
  // (1) 如果R[i]以.0结尾，直接继续
  // (2) 如果R[i]以.1结尾：
  //     - 记录local_r = R[i]
  //     (2.1) 如果G != local_r：清除R[i]最后一位为0，从local_r开始继续运行
  //     (2.2) 如果G == local_r且G以0结尾：清除R[i]最后一位为0，从local_r开始继续运行
  //     (2.3) 如果G == local_r且G以1结尾：帮助更新R[i+1]到R[N]为A.1
  // 返回正确的ptr（清除锁位）
  uintptr_t load_ptr(size_t thread_id) {
restart:
    // 读取自己的replica
    uintptr_t local_r = replica_ptrs[thread_id].load();
    uintptr_t cleared_local_r = clear_lock_bit(local_r);
      
    // Case (1): 如果R[i]以.0结尾，说明当前的R[i]不是更新的G，可以直接从R[i]开始
    if (!has_lock_bit(local_r)) {
      return local_r;
    }
      
    // Case (2): 如果R[i]以.1结尾，需要检查
    // 读取全局指针
    uintptr_t G = global_ptr.load();
      
    // Case (2.1): 如果G(cleared) != local_r，清除R[i]最后一位为0，直接继续
    if (clear_lock_bit(G) != cleared_local_r) {
      // 需要帮助update所有的replicas到这个新的G，因为可能有人把G更新完后就挂了
      // 不update的的话，后面每次都会进一次这个路线
      return help_update(clear_lock_bit(G), 0);
    }

    // Case (2.2): 如果G == local_r且G以0结尾
    // 说明当前一轮的replica update已经全部完成，可以clean自己的lock_bit并返回了
    if (clear_lock_bit(G) == cleared_local_r && !has_lock_bit(G)) {
      if (replica_ptrs[thread_id].compare_exchange_weak(local_r, cleared_local_r)) {
        // CAS成功，返回
        return cleared_local_r;
      } else {
        // CAS R[i].1 -> R[i].0失败，不过是因为别人成功更新了
        if (replica_ptrs[thread_id].load() == cleared_local_r) {
          return cleared_local_r;
        } else {
          // CAS失败，且不是因为有人成功更新了replica；说明global_ptr被修改了，重新开始
          goto restart;
        }
      }
    }
      
    // Case (2.3): 如果G == local_r且G以1结尾，帮助更新
    // 调用help_update来帮助更新所有副本
    return help_update(clear_lock_bit(G), thread_id + 1);
  }
};

#endif // HELP_UPDATE_H
