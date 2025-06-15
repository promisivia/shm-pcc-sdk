//
//  shm_ds_db.h
//  YCSB-C
//
//  Created by FangnuoWu on 12/26/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_RADIX_ART_ROWEX_DB_H_
#define YCSB_C_RADIX_ART_ROWEX_DB_H_

#include <iostream>

#include "core/db.h"
#include "db/utils.h"

using namespace std;

#include "ROWEX/Tree.h"

using std::string;
using std::vector;

namespace ycsbc {

class RadixARTROWEXDB : public DB {
 public:
  RadixARTROWEXDB() : DB(), tree(loadKey) {}

  ~RadixARTROWEXDB() {}

  int Read(const std::string& table, const std::string& key,
           const std::vector<std::string>* fields,
           std::vector<KVPair>& result) {
    Key k;
    TID tid = convert(key);
    loadKey(tid, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.lookup(k, t);
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

 private:
  ART_ROWEX::Tree tree;
};

}  // namespace ycsbc

#endif  // YCSB_C_RADIX_ART_DB_H_
