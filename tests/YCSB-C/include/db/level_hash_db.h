//
//  LevelHash_db.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/24/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_LevelHash_DB_H_
#define YCSB_C_LevelHash_DB_H_

#include "core/db.h"
#include "level_hashing.h"

namespace ycsbc {

class LevelHashDB : public DB {
 public:
  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields, std::vector<KVPair> &result);
  int Scan(const std::string &table, const std::string &key, int len,
           const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result);
  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);
  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);
  int Delete(const std::string &table, const std::string &key);

  int Read(uint64_t key, uint64_t &value) override;

  int Update(uint64_t key, uint64_t value) override;

  int Insert(uint64_t key, uint64_t value) override;

  int Delete(uint64_t key) override;

  void Init() override;

  void Close() override;

  void InitStats() override;

  void GetStats() override;

  LevelHashDB(int thread_num);

  ~LevelHashDB();

 private:
  level_hash *level;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};
};

}  // namespace ycsbc

#endif  // YCSB_C_LevelHash_DB_H_
