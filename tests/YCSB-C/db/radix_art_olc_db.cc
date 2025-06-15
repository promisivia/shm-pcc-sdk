#include "db/radix_art_olc_db.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "core/utils.h"
#include "db/utils.h"
#include "tbb/tbb.h"

namespace ycsbc {

RadixARTOLCDB::RadixARTOLCDB(int thread_num) : DB(), tree(loadKey) {
#ifdef USE_MSG_QUEUE
  pool = new ThreadPool(
      thread_num - 1, [this]() { return 0; },
      [this](int thread_id) { return; });
#endif
}

RadixARTOLCDB::~RadixARTOLCDB() {}

void RadixARTOLCDB::Init() {}

void RadixARTOLCDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

void RadixARTOLCDB::InitStats() {}

void RadixARTOLCDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
}

int RadixARTOLCDB::Read(uint64_t key, uint64_t &value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    Key k;
    loadKey(key, k);
    static thread_local auto t = tree.getThreadInfo();
    auto val = tree.lookup(k, t);
    auto ret = (val == key) ? DB::kOK : DB::kErrorNoData;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef RETURN_SYNC
  pool->enqueue(utils::RandomValueNum(), task);
#ifdef USE_MWAIT
  _umonitor(&finish);
  while (finish != true) {
    _umwait(0, 1);
  }
#else
  while (finish != true);
#endif
  return DB::kOK;
#else
  pool->enqueue(utils::RandomValueNum(), task);
  return DB::kOK;
#endif
#else
  Key k;
  loadKey(key, k);
  static thread_local auto t = tree.getThreadInfo();
  auto val = tree.lookup(k, t);
  value = val;
  auto ret = (val == key) ? DB::kOK : DB::kErrorNoData;
  return ret;
#endif
}

int RadixARTOLCDB::Update(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    Key k;
    loadKey(key, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.insert(k, key, t);
    auto ret = DB::kOK;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef RETURN_SYNC
  pool->enqueue(utils::RandomValueNum(), task);
#ifdef USE_MWAIT
  _umonitor(&finish);
  while (finish != true) {
    _umwait(0, 1);
  }
#else
  while (finish != true);
#endif
  return DB::kOK;
#else
  pool->enqueue(utils::RandomValueNum(), task);
  return DB::kOK;
#endif
#else
  Key k;
  loadKey(key, k);
  static thread_local auto t = tree.getThreadInfo();
  tree.insert(k, key, t);
  auto ret = DB::kOK;
  return ret;
#endif
}

int RadixARTOLCDB::Insert(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    Key k;
    loadKey(key, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.insert(k, key, t);
    auto ret = DB::kOK;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef RETURN_SYNC
  pool->enqueue(utils::RandomValueNum(), task);
#ifdef USE_MWAIT
  _umonitor(&finish);
  while (finish != true) {
    _umwait(0, 1);
  }
#else
  while (finish != true);
#endif
  return DB::kOK;
#else
  pool->enqueue(utils::RandomValueNum(), task);
  return DB::kOK;
#endif
#else
  Key k;
  loadKey(key, k);
  static thread_local auto t = tree.getThreadInfo();
  tree.insert(k, key, t);
  auto ret = DB::kOK;
  return ret;
#endif
}

int RadixARTOLCDB::Delete(uint64_t key) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    Key k;
    loadKey(key, k);
    static thread_local auto t = tree.getThreadInfo();
    tree.remove(k, key, t);
    auto ret = DB::kOK;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef RETURN_SYNC
  pool->enqueue(utils::RandomValueNum(), task);
#ifdef USE_MWAIT
  _umonitor(&finish);
  while (finish != true) {
    _umwait(0, 1);
  }
#else
  while (finish != true);
#endif
  return DB::kOK;
#else
  pool->enqueue(utils::RandomValueNum(), task);
  return DB::kOK;
#endif
#else
  Key k;
  loadKey(key, k);
  static thread_local auto t = tree.getThreadInfo();
  tree.remove(k, key, t);
  auto ret = DB::kOK;
  return ret;
#endif
}

}  // namespace ycsbc
