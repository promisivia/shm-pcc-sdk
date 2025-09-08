//
//  shm_ds_db.h
//  YCSB-C
//
//  Created by FangnuoWu on 12/26/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_RADIX_ART_DB_H_
#define YCSB_C_RADIX_ART_DB_H_

#include <iostream>

#include "../../../ds/RadixART/OptimisticLockCoupling/Tree.h"
#include "core/db.h"
#include "db/utils.h"

using std::string;
using std::vector;

namespace ycsbc {

class RadixARTOLCDB : public DB {
 public:
  RadixARTOLCDB(int thread_num);

  ~RadixARTOLCDB();

  void Init() override;

  void Close() override;

  void InitStats() override;

  void GetStats() override;

  int Read(const std::string& table, const std::string& key,
           const std::vector<std::string>* fields,
           std::vector<KVPair>& result) {
    Key k;
    TID tid = convert(key);
    loadKey(tid, k);
    static thread_local auto t = tree.getThreadInfo();
    auto val = tree.lookup(k, t);
    if (val != tid) {
      return DB::kErrorNoData;
    }
    return DB::kOK;
  }

  int Scan(const std::string& table, const std::string& key, int len,
           const std::vector<std::string>* fields,
           std::vector<std::vector<KVPair>>& result) {
    throw "Scan: function not implemented!";
  }

  int Update(const std::string& table, const std::string& key,
             std::vector<KVPair>& values) {
    Key k;
    TID tid = convert(key);
    loadKey(tid, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.insert(k, tid, t);
    return DB::kOK;
  }

  int Insert(const std::string& table, const std::string& key,
             std::vector<KVPair>& values) {
    Key k;
    TID tid = convert(key);
    loadKey(tid, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.insert(k, tid, t);
    return DB::kOK;
  }

  int Delete(const std::string& table, const std::string& key) {
    Key k;
    TID tid = convert(key);
    loadKey(tid, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.remove(k, tid, t);
    return DB::kOK;
  }

  int Read(uint64_t key, uint64_t& value) override;

  int Update(uint64_t key, uint64_t value) override;

  int Insert(uint64_t key, uint64_t value) override;

  int Delete(uint64_t key) override;

 private:
  ART_OLC::Tree tree;
  std::atomic<size_t> read_cnt{0};
  std::atomic<size_t> update_cnt{0};
  std::atomic<size_t> insert_cnt{0};
};

}  // namespace ycsbc

#endif  // YCSB_C_RADIX_ART_DB_H_
