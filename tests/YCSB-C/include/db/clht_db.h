#pragma once

#include "clht_lb_res.h"
#include "core/db.h"
#include "ssmem.h"

namespace ycsbc {

class CLHTDB : public DB {
 public:
  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result) override;
  int Scan(const std::string &table, const std::string &key, int len,
           const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) override;
  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) override;
  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) override;
  int Delete(const std::string &table, const std::string &key) override;

  int Read(uint64_t key, uint64_t &value) override;

  int Update(uint64_t key, uint64_t value) override;

  int Insert(uint64_t key, uint64_t value) override;

  int Delete(uint64_t key) override;

  void Init() override;

  void Close() override;

  int PoolThreadInit() override;

  void PoolThreadClose(int thread_id) override;

  void InitStats() override;

  void GetStats() override;

  CLHTDB(int thread_num, int num_buckets);
  ~CLHTDB();

 private:
  clht_t *hashtable;
  int thread_num;
  std::vector<std::atomic<uint8_t>> bits;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};

  int allocate();
  void release(int id);
};

}  // namespace ycsbc
