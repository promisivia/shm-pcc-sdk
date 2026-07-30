#include "db/clht_db.h"

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

thread_local bool finish_gc_init = false;

CLHTDB::CLHTDB(int thread_num, int num_buckets)
    : thread_num(thread_num), bits(4 * thread_num) {
  hashtable = clht_create(num_buckets);
  if (hashtable != nullptr) {
    // Update the GC array
    for (int i = 0; i < thread_num; ++i) {
      bits[i].store(false);
    }
#ifdef USE_MSG_QUEUE
    pool = new ThreadPool(
        thread_num - 1, [this]() { return this->PoolThreadInit(); },
        [this](int thread_id) { this->PoolThreadClose(thread_id); });
#else
    this->db_thread_id = this->PoolThreadInit();
#endif
  }
}

CLHTDB::~CLHTDB() { clht_gc_destroy(hashtable); }

void CLHTDB::Init() {}

void CLHTDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
  PoolThreadClose(this->db_thread_id);
}

int CLHTDB::PoolThreadInit() {
  int thread_id = allocate();
  clht_gc_thread_init(hashtable, thread_id);
  // fprintf(stderr, "[%p] init at thread %d\n", (void *)pthread_self(),
  // thread_id);
  return thread_id;
}

void CLHTDB::PoolThreadClose(int thread_id) { release(thread_id); }

int CLHTDB::Read(const std::string &table, const std::string &key,
                 const std::vector<std::string> *fields,
                 std::vector<KVPair> &result) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    clht_get(this->hashtable->data.fields.ht, key_index);
// auto ret = (val != 0) ? DB::kOK : DB::kErrorNoData;
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

int CLHTDB::Read(uint64_t key, uint64_t &value) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    clht_get(this->hashtable->data.fields.ht, key);
// auto ret = (val != 0) ? DB::kOK : DB::kErrorNoData;
#ifdef RETURN_SYNC
    finish = true;
#endif
    return DB::kOK;
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
  value = clht_get(this->hashtable->data.fields.ht, key);
  return DB::kOK;
#endif
}

int CLHTDB::Scan(const std::string &table, const std::string &key, int len,
                 const std::vector<std::string> *fields,
                 std::vector<std::vector<KVPair>> &result) {
  return 0;
}

int CLHTDB::Update(const std::string &table, const std::string &key,
                   std::vector<KVPair> &values) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    clht_put(this->hashtable, key_index, key_index);
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

int CLHTDB::Update(uint64_t key, uint64_t value) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    clht_put(this->hashtable, key, key);
#ifdef RETURN_SYNC
    finish = true;
#endif
    return DB::kOK;
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
  clht_put(this->hashtable, key, value);
  return DB::kOK;
#endif
}

int CLHTDB::Insert(const std::string &table, const std::string &key,
                   std::vector<KVPair> &values) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    clht_put(this->hashtable, key_index, key_index);
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

int CLHTDB::Insert(uint64_t key, uint64_t value) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
#ifndef USE_MSG_QUEUE
  clht_put(this->hashtable, key, value);
  return DB::kOK;
#else // if defined USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    clht_put(this->hashtable, key, key);
#ifdef RETURN_SYNC
    finish = true;
#endif
    return DB::kOK;
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
#endif
}

int CLHTDB::Delete(const std::string &table, const std::string &key) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    clht_remove(this->hashtable, key_index);
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

int CLHTDB::Delete(uint64_t key) {
  if (unlikely(!finish_gc_init)) {
    PoolThreadInit();
    finish_gc_init = true;
  }
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    clht_remove(this->hashtable, key);
#ifdef RETURN_SYNC
    finish = true;
#endif
    return DB::kOK;
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
  clht_remove(this->hashtable, key);
  return DB::kOK;
#endif
}

void CLHTDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
}

void CLHTDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
}

int CLHTDB::allocate() {
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

void CLHTDB::release(int id) {
  if (id >= 0 && id < (int)bits.size()) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
            __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc
