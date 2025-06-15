#pragma once

#include <hot/rowex/HOTRowex.hpp>
#include <idx/contenthelpers/IdentityKeyExtractor.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>

#include "core/db.h"

namespace ycsbc {

typedef struct IntKeyVal {
  uint64_t key;
  uint64_t value;
} IntKeyVal;

template <typename ValueType = IntKeyVal *>
class IntKeyExtractor {
 public:
  typedef uint64_t KeyType;

  inline KeyType operator()(ValueType const &value) const { return value->key; }
};

class HotDB : public DB {
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

  int ReadInternal(uint64_t key, uint64_t &value) override;

  int UpdateInternal(uint64_t key, uint64_t value) override;

  int InsertInternal(uint64_t key, uint64_t value) override;

  int DeleteInternal(uint64_t key) override;

  void Init() override;

  void Close() override;

  int PoolThreadInit() override;

  void PoolThreadClose(int thread_id) override;

  void InitStats() override;

  void GetStats() override;

  HotDB(int thread_num);
  ~HotDB();

private:
  hot::rowex::HOTRowex<IntKeyVal *, IntKeyExtractor> tree;
  int thread_num;
  std::vector<std::atomic<uint8_t>> bits;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};

  int allocate();
  void release(int id);
};

}  // namespace ycsbc
