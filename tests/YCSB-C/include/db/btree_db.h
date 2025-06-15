#pragma once

#include <cassert>

// #include <functional>
// #include <future>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "core/db.h"
#include "db/utils.h"
#include <shared_mutex>
// using namespace std;

// #include "future_queue.h"
#include "test_suite.h"

namespace ycsbc {

class BTreeDB : public DB {
 public:
  BTreeDB(int thread_num = 1);
  ~BTreeDB();

  void Init() override;
  void Close() override;

  int PoolThreadInit();
  void PoolThreadClose(int thread_id);

  int ThreadInit() override;
  void ThreadClose(int thread_id) override;

  int Read(const std::string& table, const std::string& key,
           const std::vector<std::string>* fields,
           std::vector<KVPair>& result) override;

  int Scan(const std::string& table, const std::string& key, int len,
           const std::vector<std::string>* fields,
           std::vector<std::vector<KVPair>>& result) override;

  int Update(const std::string& table, const std::string& key,
             std::vector<KVPair>& values) override;

  int Insert(const std::string& table, const std::string& key,
             std::vector<KVPair>& values) override;

  int Delete(const std::string& table, const std::string& key) override;

  int ReadInternal(uint64_t key, uint64_t& value) override;

  int UpdateInternal(uint64_t key, uint64_t value) override;

  int InsertInternal(uint64_t key, uint64_t value) override;

  int DeleteInternal(uint64_t key) override;

  void InitStats() override;
  void GetStats() override;

 private:
  BTreeType* tree;
  std::vector<std::atomic<uint8_t>> bits;
  int thread_num;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};

  std::shared_mutex rwlock{};

  int allocate();
  void release(int id);
};

}  // namespace ycsbc