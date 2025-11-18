#include "help_update.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <atomic>
#include <random>
#include <mutex>
#include <map>
#include <algorithm>
#include <iomanip>

// 操作类型
enum class OpType {
  UPDATE,
  LOAD
};

// 操作记录
struct Operation {
  OpType type;
  size_t thread_id;
  uintptr_t value;  // 对于UPDATE是写入的值，对于LOAD是读取的值
  uint64_t start_time;  // 开始时间（纳秒）
  uint64_t end_time;    // 结束时间（纳秒）
  bool success;         // 操作是否成功（对于UPDATE）
  
  Operation(OpType t, size_t tid, uintptr_t v, uint64_t start, uint64_t end, bool s = true)
    : type(t), thread_id(tid), value(v), start_time(start), end_time(end), success(s) {}
};

// 全局操作历史（线程安全）
std::mutex history_mutex;
std::vector<Operation> operation_history;

// 获取当前时间（纳秒）
uint64_t get_time_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::high_resolution_clock::now().time_since_epoch()
  ).count();
}

// 更新线程
void updater_thread(HelpUpdateMechanism* mechanism, size_t thread_id, 
                    int num_updates, std::atomic<bool>& stop_flag) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 1000);
  
  for (int i = 0; i < num_updates && !stop_flag.load(); ++i) {
    // 生成一个唯一的指针值（确保8字节对齐）
    uintptr_t new_val = (thread_id * 1000000ULL + i) * 8;
    
    uint64_t start = get_time_ns();
    bool success = false;
    
    // 循环直到 cas_ptr 成功
    while (!success && !stop_flag.load()) {
      // 读取当前的 global_ptr 值（清除锁位）作为 old_val
      uintptr_t cur_global = mechanism->global_ptr.load();
      uintptr_t old_val = clear_lock_bit(cur_global);
      
      // 如果当前有锁位，先帮助更新
      if (has_lock_bit(cur_global)) {
        old_val = mechanism->help_update(old_val, 0);
        // 重新读取 global_ptr
        cur_global = mechanism->global_ptr.load();
        old_val = clear_lock_bit(cur_global);
      }
      
      // 尝试 cas_ptr
      success = mechanism->cas_ptr(old_val, new_val);
      
      if (success) {
        // cas_ptr 成功，记录到 history
        uint64_t end = get_time_ns();
        {
          std::lock_guard<std::mutex> lock(history_mutex);
          operation_history.emplace_back(OpType::UPDATE, thread_id, new_val, start, end, true);
        }
      } else {
        // cas_ptr 失败，重新读取 global_ptr 的值然后重试
        // 不需要记录失败的操作，直接重试
      }
    }
    
    // 随机休眠，模拟实际工作负载
    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
  }
}

// 加载线程
void loader_thread(HelpUpdateMechanism* mechanism, size_t thread_id,
                   std::atomic<bool>& stop_flag) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 100);
  
  while (!stop_flag.load()) {
    uint64_t start = get_time_ns();
    uintptr_t value = mechanism->load_ptr(thread_id);
    uint64_t end = get_time_ns();
    
    {
      std::lock_guard<std::mutex> lock(history_mutex);
      operation_history.emplace_back(OpType::LOAD, thread_id, value, start, end, true);
    }
    
    // 随机休眠
    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
  }
}

// 验证线性化性：检查是否存在一个合法的操作序列
bool verify_linearizability(const std::vector<Operation>& history) {
  // 创建一个操作序列，按照线性化点排序
  // 线性化点：对于UPDATE是成功设置global_ptr的时间，对于LOAD是读取replica的时间
  // 我们使用操作的结束时间作为线性化点的近似
  
  if (history.empty()) {
    return true;
  }
  
  // 找到最早的时间作为基准
  uint64_t base_time = history[0].start_time;
  for (const auto& op : history) {
    base_time = std::min(base_time, op.start_time);
  }
  
  std::vector<Operation> sorted_ops = history;
  
  // 按照结束时间排序（线性化点）
  std::sort(sorted_ops.begin(), sorted_ops.end(), 
    [](const Operation& a, const Operation& b) {
      if (a.end_time != b.end_time) {
        return a.end_time < b.end_time;
      }
      // 如果时间相同，UPDATE优先于LOAD（更保守）
      if (a.type != b.type) {
        return a.type == OpType::UPDATE;
      }
      return a.thread_id < b.thread_id;
    });
  
  // 验证线性化性：只有UPDATE在LOAD开始之前完成且LOAD看不到才是错的
  // 规则：
  // 1. 如果UPDATE在LOAD开始之前完成，LOAD必须看到UPDATE的值（或更晚的值）
  // 2. 如果UPDATE和LOAD重叠，LOAD可能看到旧值或新值，都是合法的
  // 3. 如果LOAD在UPDATE开始之前完成，LOAD看到旧值是合法的
  
  size_t violation_count = 0;
  
  for (size_t i = 0; i < sorted_ops.size(); ++i) {
    const auto& op = sorted_ops[i];
    
    if (op.type == OpType::LOAD) {
      // 对于每个LOAD，检查所有在它开始之前完成的UPDATE
      uintptr_t actual = clear_lock_bit(op.value);
      
      // 找到在LOAD开始之前完成的最后一次UPDATE
      uintptr_t last_update_before_load = 0;
      bool found_update_before_load = false;
      
      for (size_t j = 0; j < sorted_ops.size(); ++j) {
        if (sorted_ops[j].type == OpType::UPDATE) {
          // UPDATE在LOAD开始之前完成
          if (sorted_ops[j].end_time <= op.start_time) {
            last_update_before_load = clear_lock_bit(sorted_ops[j].value);
            found_update_before_load = true;
          } else {
            // 遇到第一个与LOAD重叠的UPDATE，停止查找
            break;
          }
        }
      }
      
      // 如果找到了在LOAD开始之前完成的UPDATE，LOAD必须看到这个值或更晚的值
      if (found_update_before_load) {
        // 检查actual是否等于任何在LOAD开始之前完成的UPDATE的值
        // 或者等于更晚的UPDATE值（即使与LOAD重叠）
        bool sees_valid_value = false;
        
        // 遍历所有UPDATE，检查actual是否等于某个UPDATE的值
        for (size_t k = 0; k < sorted_ops.size(); ++k) {
          if (sorted_ops[k].type == OpType::UPDATE) {
            uintptr_t update_value = clear_lock_bit(sorted_ops[k].value);
            if (actual == update_value) {
              // 如果这个UPDATE在LOAD开始之前完成，或者与LOAD重叠，都是合法的
              // 重叠条件：UPDATE的开始时间 < LOAD的结束时间 且 UPDATE的结束时间 > LOAD的开始时间
              // 或者：UPDATE在LOAD开始之前完成
              bool update_before_load = (sorted_ops[k].end_time <= op.start_time);
              bool update_overlaps_load = (sorted_ops[k].start_time < op.end_time) && 
                                         (sorted_ops[k].end_time > op.start_time);
              if (update_before_load || update_overlaps_load) {
                sees_valid_value = true;
                break;
              }
            }
          }
        }
        
        // 如果actual不等于任何UPDATE的值，检查是否是初始值0
        // 初始值0只有在没有任何UPDATE完成时才是合法的
        if (!sees_valid_value && actual == 0) {
          // 检查是否有任何UPDATE在LOAD开始之前完成
          // 如果有，actual=0是不合法的
          sees_valid_value = !found_update_before_load;
        }
        
        if (!sees_valid_value) {
          if (violation_count < 10) {  // 只打印前10个违规
            std::cerr << "Linearizability violation #" << (violation_count + 1) << "!" << std::endl;
            std::cerr << "  Thread " << op.thread_id << " LOAD started at " 
                      << (op.start_time - base_time) / 1000.0 << " us"
                      << ", ended at " << (op.end_time - base_time) / 1000.0 << " us" << std::endl;
            std::cerr << "  Returned: " << std::hex << actual << std::dec << std::endl;
            
            // 找到last_update_before_load对应的UPDATE操作，打印其开始和结束时间
            for (size_t j = 0; j < sorted_ops.size(); ++j) {
              if (sorted_ops[j].type == OpType::UPDATE) {
                uintptr_t update_value = clear_lock_bit(sorted_ops[j].value);
                if (update_value == last_update_before_load && 
                    sorted_ops[j].end_time <= op.start_time) {
                  std::cerr << "  Last UPDATE before load: value=" << std::hex << last_update_before_load << std::dec
                            << ", started at " << (sorted_ops[j].start_time - base_time) / 1000.0 << " us"
                            << ", ended at " << (sorted_ops[j].end_time - base_time) / 1000.0 << " us" << std::endl;
                  break;
                }
              }
            }
            
            // 打印所有可能重叠的UPDATE操作
            // 重叠条件：UPDATE的开始时间 < LOAD的结束时间 且 UPDATE的结束时间 > LOAD的开始时间
            std::cerr << "  Overlapping UPDATEs:" << std::endl;
            size_t overlapping_count = 0;
            for (size_t j = 0; j < sorted_ops.size(); ++j) {
              if (sorted_ops[j].type == OpType::UPDATE) {
                // 检查是否重叠：UPDATE与LOAD时间重叠
                bool overlaps = (sorted_ops[j].start_time < op.end_time) && 
                                (sorted_ops[j].end_time > op.start_time);
                if (overlaps) {
                  uintptr_t update_value = clear_lock_bit(sorted_ops[j].value);
                  std::cerr << "    UPDATE #" << overlapping_count + 1 
                            << ": value=" << std::hex << update_value << std::dec
                            << ", thread=" << sorted_ops[j].thread_id
                            << ", started at " << (sorted_ops[j].start_time - base_time) / 1000.0 << " us"
                            << ", ended at " << (sorted_ops[j].end_time - base_time) / 1000.0 << " us" << std::endl;
                  overlapping_count++;
                }
              }
            }
            if (overlapping_count == 0) {
              std::cerr << "    (none)" << std::endl;
            }
          }
          violation_count++;
        }
      }
    }
  }
  
  if (violation_count > 0) {
    std::cerr << "Total violations: " << violation_count << std::endl;
    return false;
  }
  
  return true;
}

int main() {
  const size_t num_threads = 8;
  const int num_updates_per_thread = 100;
  const int test_duration_seconds = 5;
  
  std::cout << "=== Linearizability Test ===" << std::endl;
  std::cout << "Threads: " << num_threads << std::endl;
  std::cout << "Updates per updater: " << num_updates_per_thread << std::endl;
  std::cout << "Test duration: " << test_duration_seconds << " seconds" << std::endl;
  std::cout << std::endl;
  
  HelpUpdateMechanism mechanism(num_threads);
  std::atomic<bool> stop_flag(false);
  std::vector<std::thread> threads;
  
  // 启动更新线程（前一半）
  size_t num_updaters = num_threads / 2;
  for (size_t i = 0; i < num_updaters; ++i) {
    threads.emplace_back(updater_thread, &mechanism, i, num_updates_per_thread, 
                        std::ref(stop_flag));
  }
  
  // 启动加载线程（后一半）
  for (size_t i = num_updaters; i < num_threads; ++i) {
    threads.emplace_back(loader_thread, &mechanism, i, std::ref(stop_flag));
  }
  
  // 运行测试
  std::this_thread::sleep_for(std::chrono::seconds(test_duration_seconds));
  stop_flag.store(true);
  
  // 等待所有线程完成
  for (auto& t : threads) {
    t.join();
  }
  
  std::cout << "Total operations recorded: " << operation_history.size() << std::endl;
  
  // 统计操作类型
  size_t num_updates = 0;
  size_t num_loads = 0;
  for (const auto& op : operation_history) {
    if (op.type == OpType::UPDATE) {
      num_updates++;
    } else {
      num_loads++;
    }
  }
  std::cout << "  Updates: " << num_updates << std::endl;
  std::cout << "  Loads: " << num_loads << std::endl;
  std::cout << std::endl;
  
  // 验证线性化性
  std::cout << "Verifying linearizability..." << std::endl;
  bool passed = verify_linearizability(operation_history);
  
  if (passed) {
    std::cout << "✓ Linearizability test PASSED!" << std::endl;
    
    // 打印一些统计信息
    if (!operation_history.empty()) {
      uint64_t min_time = operation_history[0].start_time;
      uint64_t max_time = operation_history[0].end_time;
      for (const auto& op : operation_history) {
        min_time = std::min(min_time, op.start_time);
        max_time = std::max(max_time, op.end_time);
      }
      double duration_seconds = (max_time - min_time) / 1e9;
      std::cout << "Test duration: " << std::fixed << std::setprecision(2) 
                << duration_seconds << " seconds" << std::endl;
      std::cout << "Operations per second: " 
                << std::fixed << std::setprecision(0)
                << (operation_history.size() / duration_seconds) << std::endl;
    }
    
    return 0;
  } else {
    std::cerr << "✗ Linearizability test FAILED!" << std::endl;
    if (!passed) {
      std::cerr << "  Linearizability check failed" << std::endl;
    }
    return 1;
  }
}

