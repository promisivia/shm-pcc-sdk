#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "DSM.h"
#include "Tree.h"
#include "core/db.h"
#include "db/utils.h"
using namespace std;

namespace ycsbc {

class ShermanDB : public DB {
 public:
  ShermanDB(int thread_num = 1);
  ~ShermanDB();

  void Init() override;
  void Close() override;

  int PoolThreadInit() override;

  int ThreadInit() override;

  int ReadInternal(uint64_t key, uint64_t& value) override;

  int UpdateInternal(uint64_t key, uint64_t value) override;

  int InsertInternal(uint64_t key, uint64_t value) override;

  int DeleteInternal(uint64_t key) override;

  void InitStats() override;
  void GetStats() override;

 private:
  Tree* tree;
  DSM* dsm;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};
  uint32_t thread_num;
};

}  // namespace ycsbc