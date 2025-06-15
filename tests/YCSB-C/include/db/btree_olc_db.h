#pragma once

#include <assert.h>

#include <atomic>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "BTreeOLC/BTreeOLC.h"
#include "core/db.h"
#include "db/utils.h"
#include "utils/id_pair.h"

using namespace std;

namespace ycsbc {

class BTreeOLCDB : public DB {
 public:
  BTreeOLCDB(int thread_num = 1);
  ~BTreeOLCDB();

  void Init() override;
  void Close() override;

  int PoolThreadInit();
  void PoolThreadClose(int thread_id);

  int Read(const std::string& table, const std::string& key,
           const std::vector<std::string>* fields, std::vector<KVPair>& result) override;

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
  struct btreeolc::BTree<uint64_t, uint64_t>* tree;
  int thread_num;
  std::vector<std::atomic<uint8_t>> bits;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};

  int allocate();
  void release(int id);
};

}  // namespace ycsbc