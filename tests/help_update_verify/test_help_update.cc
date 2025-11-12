#include "help_update.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <atomic>
#include <random>

// 简单的测试数据结构
struct TestData {
  int value;
  int version;
  
  TestData(int v = 0, int ver = 0) : value(v), version(ver) {}
};

// 全局计数器，用于验证
std::atomic<int> update_count(0);
std::atomic<int> help_count(0);
std::atomic<bool> stop_flag(false);

// 测试函数：持续更新全局指针
void updater_thread(HelpUpdateMechanism<TestData>* mechanism, int thread_id, 
                    int num_updates) {
  for (int i = 0; i < num_updates; ++i) {
    // 创建新的数据
    TestData* new_data = new TestData(thread_id * 1000 + i, i);
    
    // 更新全局指针
    bool success = mechanism->update_global_ptr(new_data);
    if (success) {
      update_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    // 短暂休眠，模拟实际工作负载
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
}

// 测试函数：读取并验证一致性
void reader_thread(HelpUpdateMechanism<TestData>* mechanism, int thread_id,
                   int num_reads) {
  int consistent_reads = 0;
  int inconsistent_reads = 0;
  
  for (int i = 0; i < num_reads; ++i) {
    // 获取副本指针
    TestData* replica = mechanism->get_replica_ptr(thread_id);
    
    // 读取全局指针
    TestData* global = mechanism->read_global_ptr();
    
    // 检查一致性
    if (replica == global) {
      consistent_reads++;
    } else {
      inconsistent_reads++;
      // 如果检测到不一致，帮助更新
      mechanism->help_update_all_replicas();
      help_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    // 短暂休眠
    std::this_thread::sleep_for(std::chrono::microseconds(5));
  }
  
  std::cout << "Thread " << thread_id << " - Consistent: " << consistent_reads 
            << ", Inconsistent: " << inconsistent_reads << std::endl;
}

// 测试函数：验证block机制
void block_test_thread(HelpUpdateMechanism<TestData>* mechanism, int thread_id) {
  // 这个线程会快速连续更新，验证是否能正确block
  for (int i = 0; i < 1000; ++i) {
    TestData* new_data = new TestData(thread_id * 10000 + i, i);
    
    // 更新全局指针（更新者会确保所有副本都更新完成才返回）
    mechanism->update_global_ptr(new_data);
    
    // update_global_ptr返回后，锁位应该已经被清除
    // 但是，如果在我们返回后，另一个线程立即更新了global_ptr，
    // 锁位可能又被设置了，所以我们需要检查并帮助更新
    uintptr_t global = mechanism->global_ptr.load(std::memory_order_acquire);
    if (has_lock_bit(global)) {
      // 有其他线程正在更新，帮助它
      mechanism->help_update_all_replicas();
      global = mechanism->global_ptr.load(std::memory_order_acquire);
    }
    
    // 验证所有副本都已更新到当前的global_ptr值
    // 注意：由于可能有其他线程更新，我们验证的是当前global_ptr的值
    uintptr_t global_clear = clear_lock_bit(global);
    bool all_consistent = true;
    for (size_t j = 0; j < mechanism->num_threads; ++j) {
      uintptr_t replica = mechanism->replica_ptrs[j].load(std::memory_order_relaxed);
      if (replica != global_clear) {
        all_consistent = false;
        std::cerr << "Error: Thread " << thread_id 
                  << " - Replica " << j << " not updated. Expected: " 
                  << global_clear << ", Got: " << replica << std::endl;
      }
    }
    
    if (all_consistent) {
      // std::cout << "Thread " << thread_id << " - Update " << i 
      //           << " completed, all replicas consistent" << std::endl;
    }
  }
}

int main() {
  const int num_threads = 4;
  const int num_updates = 1000;
  const int num_reads = 2000;
  
  std::cout << "=== Help Update Mechanism Test ===" << std::endl;
  std::cout << "Threads: " << num_threads << std::endl;
  std::cout << "Updates per updater: " << num_updates << std::endl;
  std::cout << "Reads per reader: " << num_reads << std::endl;
  std::cout << std::endl;
  
  HelpUpdateMechanism<TestData> mechanism(num_threads);
  
  std::vector<std::thread> threads;
  
  // 启动更新线程
  for (int i = 0; i < 2; ++i) {
    threads.emplace_back(updater_thread, &mechanism, i, num_updates);
  }
  
  // 启动读取线程
  for (int i = 2; i < num_threads; ++i) {
    threads.emplace_back(reader_thread, &mechanism, i, num_reads);
  }
  
  // 等待所有线程完成
  for (auto& t : threads) {
    t.join();
  }
  
  std::cout << std::endl;
  std::cout << "Total updates: " << update_count.load() << std::endl;
  std::cout << "Total help operations: " << help_count.load() << std::endl;
  
  // 验证最终一致性
  std::cout << std::endl << "=== Final Consistency Check ===" << std::endl;
  TestData* final_global = mechanism.read_global_ptr();
  std::cout << "Final global pointer: " << final_global << std::endl;
  
  bool all_consistent = true;
  for (size_t i = 0; i < num_threads; ++i) {
    TestData* replica = mechanism.get_replica_ptr(i);
    if (replica != final_global) {
      std::cerr << "Error: Replica " << i << " inconsistent. Global: " 
                << final_global << ", Replica: " << replica << std::endl;
      all_consistent = false;
    } else {
      std::cout << "Replica " << i << " is consistent: " << replica << std::endl;
    }
  }
  
  if (all_consistent) {
    std::cout << "✓ All replicas are consistent!" << std::endl;
  } else {
    std::cerr << "✗ Some replicas are inconsistent!" << std::endl;
    return 1;
  }
  
  // Block测试
  std::cout << std::endl << "=== Block Test ===" << std::endl;
  std::cout << "Testing if updates properly block until all replicas are updated..." 
            << std::endl;
  
  HelpUpdateMechanism<TestData> block_mechanism(num_threads);
  std::vector<std::thread> block_threads;
  
  // 启动多个快速更新线程
  for (int i = 0; i < num_threads; ++i) {
    block_threads.emplace_back(block_test_thread, &block_mechanism, i);
  }
  
  // 等待所有线程完成
  for (auto& t : block_threads) {
    t.join();
  }
  
  // 最终一致性检查
  std::cout << "Final consistency check for block test..." << std::endl;
  TestData* block_final_global = block_mechanism.read_global_ptr();
  std::cout << "Final global pointer: " << block_final_global << std::endl;
  
  all_consistent = true;
  for (size_t i = 0; i < num_threads; ++i) {
    TestData* replica = block_mechanism.get_replica_ptr(i);
    if (replica != block_final_global) {
      std::cerr << "Error: Replica " << i << " inconsistent. Global: " 
                << block_final_global << ", Replica: " << replica << std::endl;
      all_consistent = false;
    }
  }
  
  if (all_consistent) {
    std::cout << "✓ Block test passed! All replicas are consistent!" << std::endl;
    return 0;
  } else {
    std::cerr << "✗ Block test failed! Some replicas are inconsistent!" << std::endl;
    return 1;
  }
}

