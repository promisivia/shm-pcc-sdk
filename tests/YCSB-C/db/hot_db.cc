#include "db/hot_db.h"

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

HotDB::HotDB(int thread_num) : thread_num(thread_num), bits(4 * thread_num) {
  // Update the GC array
  for (int i = 0; i < thread_num; ++i) {
    bits[i].store(false);
  }
#ifdef USE_MSG_QUEUE
  pool = new ThreadPool(
      thread_num - 1, [this]() { return 0; },
      [this](int thread_id) { return; });
#endif
}

HotDB::~HotDB() {}

void HotDB::Init() {}

void HotDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int HotDB::PoolThreadInit() {
  int thread_id = allocate();
  // fprintf(stderr, "[%p] init at thread %d\n", (void *)pthread_self(),
  // thread_id);
  return thread_id;
}

void HotDB::PoolThreadClose(int thread_id) { release(thread_id); }

int HotDB::Read(const std::string &table, const std::string &key,
                const std::vector<std::string> *fields,
                std::vector<KVPair> &result) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    idx::contenthelpers::OptionalValue<IntKeyVal *> result =
        tree.lookup(key_index);
    auto ret = result.mIsValid ? DB::kOK : DB::kErrorNoData;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef USE_MSG_QUEUE
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
  return task();
#endif
}

int HotDB::ReadInternal(uint64_t key, uint64_t &value) {
  idx::contenthelpers::OptionalValue<IntKeyVal *> result = tree.lookup(key);
  if(!result.mIsValid) {
    return DB::kErrorNoData;
  }
  value = result.mValue->value;
  return DB::kOK;
}

int HotDB::Scan(const std::string &table, const std::string &key, int len,
                const std::vector<std::string> *fields,
                std::vector<std::vector<KVPair>> &result) {
  return 0;
}

int HotDB::Update(const std::string &table, const std::string &key,
                  std::vector<KVPair> &values) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    IntKeyVal *k;
    int ret = posix_memalign((void **)&k, 64, sizeof(IntKeyVal));
    if (ret != 0 || k == nullptr)[[unlikely]] {
      return DB::kErrorNoData;
    }
    k->key = key_index;
    k->value = key_index;
    idx::contenthelpers::OptionalValue<IntKeyVal *> result = tree.upsert(k);
    (void)result;
    ret = DB::kOK;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef USE_MSG_QUEUE
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
  return task();
#endif
}

int HotDB::UpdateInternal(uint64_t key, uint64_t value) {
  IntKeyVal *k;
  posix_memalign((void **)&k, 64, sizeof(IntKeyVal));
  k->key = key;
  k->value = value;
  idx::contenthelpers::OptionalValue<IntKeyVal *> result = tree.upsert(k);
  (void)result;
  return DB::kOK;
}

int HotDB::Insert(const std::string &table, const std::string &key,
                  std::vector<KVPair> &values) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    IntKeyVal *k;
    posix_memalign((void **)&k, 64, sizeof(IntKeyVal));
    k->key = key_index;
    k->value = key_index;
    auto ret = (tree.insert(k)) ? DB::kOK : DB::kErrorConflict;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret;
  };
#ifdef USE_MSG_QUEUE
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
  return task();
#endif
}

int HotDB::InsertInternal(uint64_t key, uint64_t value) {
  IntKeyVal *k;
  posix_memalign((void **)&k, 64, sizeof(IntKeyVal));
  k->key = key;
  k->value = value;
  auto ret = (tree.insert(k)) ? DB::kOK : DB::kErrorConflict;
  return ret;
}

int HotDB::Delete(const std::string &table, const std::string &key) {
  int finish = false;
  auto task = [this, key, &finish]() {
#ifdef RETURN_SYNC
    finish = true;
#endif
    return DB::kOK;
  };
#ifdef USE_MSG_QUEUE
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
  return task();
#endif
}

int HotDB::DeleteInternal(uint64_t key) { return DB::kOK; }

void HotDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
}

void HotDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
}

int HotDB::allocate() {
  for (int i = 0; i < (int)bits.size(); ++i) {
    uint8_t expected = 0;
    if (bits[i].compare_exchange_strong(expected, 1)) {
      return i;
    }
  }
  fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
          __func__, pthread_self());
  exit(1);
  return -1;
}

void HotDB::release(int id) {
  if (id >= 0 && id < (int)bits.size()) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
            __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc
