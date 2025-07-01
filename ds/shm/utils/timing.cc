#include "utils/timing.h"

#include <filesystem>
#include <fstream>

#include "utils/config.h"

std::unordered_map<std::string, std::atomic<uint64_t>>
    TimerAverage::elapsed_times_;
std::unordered_map<std::string, std::atomic<uint64_t>>
    TimerAverage::elapsed_counts_;

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
}

void PrintStatistics() {
  std::filesystem::create_directories("log/timing");
  std::string dir = "log/timing/";

  std::string current_time = GetCurrentTimeString();
#ifdef QUEUE_LEN_PERF
  std::ofstream file(dir + current_time + ".log");
  Logger<size_t>::Print("queue_len", file);
#endif
#ifdef QUEUE_LATENCY
  std::ofstream file(dir + current_time + ".log");
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
}
