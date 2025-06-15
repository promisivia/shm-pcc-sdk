#pragma once

#define LOCAL_NO_CC

#include "core/db.h"
#include "utils/config.h"
#ifdef LOCAL_NO_CC
// #ifdef NO_CC
#include "libpmemobj++/experimental/clevel_hash.hpp"
#else
#include "libpmemobj++/experimental/clevel_hash_original.hpp"
namespace nvobj = pmem::obj;
#endif

namespace ycsbc {

class CLevelHashDB : public DB {
public:
  // int Read(const std::string &table, const std::string &key,
  //          const std::vector<std::string> *fields, std::vector<KVPair>
  //          &result);
  // int Scan(const std::string &table, const std::string &key, int len,
  //          const std::vector<std::string> *fields,
  //          std::vector<std::vector<KVPair>> &result);
  // int Update(const std::string &table, const std::string &key,
  //            std::vector<KVPair> &values);
  // int Insert(const std::string &table, const std::string &key,
  //            std::vector<KVPair> &values);
  // int Delete(const std::string &table, const std::string &key);

  int ReadInternal(uint64_t key, uint64_t &value) override;

  int UpdateInternal(uint64_t key, uint64_t value) override;

  int InsertInternal(uint64_t key, uint64_t value) override;

  int DeleteInternal(uint64_t key) override;

  // void Init() override;

  // void Close() override;

  // void InitStats() override;

  // void GetStats() override;

  CLevelHashDB(int thread_num);

  ~CLevelHashDB();

private:
// #ifdef NO_CC
#ifdef LOCAL_NO_CC
  using clevel_hash_64 = clevel_hash<uint64_t, uint64_t>;
  clevel_hash_64 *level;
#else
  typedef nvobj::experimental::clevel_hash<nvobj::p<uint64_t>,
                                           nvobj::p<uint64_t>>
      persistent_map_type;
  struct root {
    nvobj::persistent_ptr<persistent_map_type> cons;
  };
  nvobj::pool<root> pop;
#endif
  int thread_num;
};

} // namespace ycsbc