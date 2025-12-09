#include "utils/timing.h"

#include <filesystem>
#include <fstream>

#include "utils/config.h"

std::unordered_map<std::string, std::atomic<uint64_t>>
    TimerAverage::elapsed_times_;
std::unordered_map<std::string, std::atomic<uint64_t>>
    TimerAverage::elapsed_counts_;

std::unordered_map<std::string, uint64_t> TimerAverageRdtsc::elapsed_times_;
std::unordered_map<std::string, uint64_t> TimerAverageRdtsc::elapsed_counts_;

std::unordered_map<std::string,
                   tbb::concurrent_vector<std::chrono::nanoseconds>>
    TimerDetailed::elapsed_times_;

std::unordered_map<std::string, std::atomic<uint64_t>> CallCounter::counts_;

template <typename T>
std::unordered_map<std::string, tbb::concurrent_vector<T>> Logger<T>::values_;

std::string GetCurrentTimeString() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
  return ss.str();
}

void InitStatistics() {
#ifdef QUEUE_LEN_PERF
  Logger<size_t>::AddNewEntry("queue_len");
#endif
#ifdef QUEUE_LATENCY
  TimerAverage::AddNewEntry("queue_latency");
#endif
#ifdef TRX_LATENCY
  TimerAverage::AddNewEntry("read_latency");
  TimerAverage::AddNewEntry("insert_latency");
  TimerAverage::AddNewEntry("update_latency");
#endif
#ifdef QUEUE_LATENCY_BREAKDOWN
  TimerAverage::AddNewEntry("queue_latency_dequeue");
  TimerAverage::AddNewEntry("queue_latency_poll");
#endif
#ifdef BWTREE_RETRY_COUNT
  CallCounter::AddNewEntry("bwtree_retry_count");
  CallCounter::AddNewEntry("bwtree_half_retry_count");
  CallCounter::AddNewEntry("bwtree_traverse_count");
#endif
#ifdef CLEVEL_DOUBLE_READ_COUNT
  CallCounter::AddNewEntry("clevel_double_read_count");
  CallCounter::AddNewEntry("clevel_next_level_count");
#endif
  // Insert/Update 时间统计（用于 load 阶段和 workload tx 阶段）
  TimerAverage::AddNewEntry("insert_load");
  TimerDetailed::AddNewEntry("insert_load");
  TimerAverage::AddNewEntry("update_workload");
  TimerDetailed::AddNewEntry("update_workload");
#ifdef PLOAD_GC_STAT
  TimerAverage::AddNewEntry("pload_gc_stat");
#endif
#ifdef PLOAD_ROOT_STAT
  TimerAverage::AddNewEntry("pload_root_stat");
#endif
#ifdef PLOAD_COMMON_STAT
  TimerAverage::AddNewEntry("pload_common_stat");
#endif
#ifdef MSG_FUNC_TIMING
  TimerAverage::AddNewEntry("msg_timing_func");
#endif
  // TimerAverage::AddNewEntry("bwtree_read");
  // TimerAverage::AddNewEntry("bwtree_gc");
}

void PrintStatistics() {
  std::filesystem::create_directories("log/timing");
  std::string dir = "log/timing/";

  std::string current_time = GetCurrentTimeString();
#ifdef QUEUE_LEN_PERF
  std::ofstream file(dir + "queue_len_perf" + ".log");
  Logger<size_t>::Print("queue_len", file);
#endif
#ifdef QUEUE_LATENCY
  std::ofstream file(dir + "queue_latency" + ".log");
  TimerAverage::Print("queue_latency", file);
#endif
#ifdef TRX_LATENCY
  std::ofstream file(dir + current_time + ".log");
  TimerAverage::Print("read_latency", file);
  TimerAverage::Print("insert_latency", file);
  TimerAverage::Print("update_latency", file);
#endif
#ifdef QUEUE_LATENCY_BREAKDOWN
  std::ofstream file(dir + current_time + ".log");
  TimerAverage::Print("queue_latency_dequeue", file);
  TimerAverage::Print("queue_latency_poll", file);
#endif
#ifdef BWTREE_RETRY_COUNT
  std::ofstream file(dir + current_time + ".log");
  CallCounter::Print("bwtree_retry_count", file);
  CallCounter::Print("bwtree_half_retry_count", file);
  CallCounter::Print("bwtree_traverse_count", file);
#endif
#ifdef CLEVEL_DOUBLE_READ_COUNT
  std::ofstream file(dir + current_time + ".log");
  CallCounter::Print("clevel_double_read_count", file);
  CallCounter::Print("clevel_next_level_count", file);
#endif
  // 打印 Insert/Update 时间统计
  {
    std::ofstream file(dir + current_time + "_insert_update.log");
    file << "========== Insert Statistics (Load Phase) ==========" << std::endl;
    TimerAverage::Print("insert_load", file);
    file << std::endl;
    file << "Insert Histogram (Load Phase):" << std::endl;
    TimerDetailed::PrintHistogram("insert_load", 10, 50);
    file << std::endl;
    file << "========== Update Statistics (Workload TX) ==========" << std::endl;
    TimerAverage::Print("update_workload", file);
    file << std::endl;
    file << "Update Histogram (Workload TX):" << std::endl;
    TimerDetailed::PrintHistogram("update_workload", 10, 50);
    file << std::endl;
  }
  // 同时在控制台打印 insert 延迟
  std::cout << "========== Insert Latency (Load Phase) ==========" << std::endl;
  TimerAverage::Print("insert_load", std::cout);
#ifdef PLOAD_GC_STAT
  std::ofstream file_pload_gc_stat(dir + "pload_gc_stat" + ".log");
  TimerAverage::Print("pload_gc_stat", file_pload_gc_stat);
#endif
#ifdef PLOAD_ROOT_STAT
  std::ofstream file_pload_root_stat(dir + "pload_root_stat" + ".log");
  TimerAverage::Print("pload_root_stat", file_pload_root_stat);
#endif
#ifdef PLOAD_COMMON_STAT
  std::ofstream file_pload_common_stat(dir + "pload_common_stat" + ".log");
  TimerAverage::Print("pload_common_stat", file_pload_common_stat);
#endif
#ifdef MSG_FUNC_TIMING
  std::ofstream file_msg_func_timing(dir + "msg_timing_func" + ".log");
  TimerAverage::Print("msg_timing_func", file_msg_func_timing);
#endif
  // std::ofstream file_bwtree_read(dir + "bwtree_read" + ".log");
  // TimerAverage::Print("bwtree_read", file_bwtree_read);
  // std::ofstream file_bwtree_read_gc(dir + "bwtree_read_gc" + ".log");
  // TimerAverage::Print("bwtree_gc", file_bwtree_read_gc);
}
