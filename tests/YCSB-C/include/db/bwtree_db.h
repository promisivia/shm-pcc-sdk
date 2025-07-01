#pragma once

#include <atomic>
#include <vector>

#include "core/db.h"
#include "shm/cxl_type.h"
using namespace std;

#include "test_suite.h"

namespace ycsbc {

class BwTreeDB : public DB {
 public:
  BwTreeDB(int thread_num = 1);
  ~BwTreeDB();

  void Init() override;
  void Close() override;

  int PoolThreadInit() override;
  void PoolThreadClose(int thread_id) override;

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

  int ReadInternal(uint64_t key, uint64_t &value) override;

  int UpdateInternal(uint64_t key, uint64_t value) override;

  int InsertInternal(uint64_t key, uint64_t value) override;

  int DeleteInternal(uint64_t key) override;

  void InitStats() override;
  void GetStats() override;

 private:
  TreeType* tree;
  int thread_num;
  cxl_vector<std::atomic<uint8_t>> bits;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};

  int allocate();
  void release(int id);
};

}  // namespace ycsbc