#include <random>

#include "connection/establish.h"
#include "core/utils.h"
#include "utils/sim_id.h"

#include "db/btree_olc_db.h"

namespace ycsbc {

BTreeOLCDB::BTreeOLCDB(int thread_num)
    : DB(), thread_num(thread_num), bits(thread_num), tree(new btreeolc::BTree<uint64_t, uint64_t>()) {
  for (int i = 0; i < thread_num; ++i) {
    bits[i].store(false);
  }
#ifdef USE_MSG_QUEUE
  this->db_thread_id = this->ThreadInit();
  pool = new ThreadPool(
      thread_num - 1, [this]() { return this->PoolThreadInit(); },
      [this](int thread_id) { this->PoolThreadClose(thread_id); });
#endif
}

BTreeOLCDB::~BTreeOLCDB() {
#ifdef USE_MSG_QUEUE
  this->ThreadClose(this->db_thread_id);
#endif
  delete tree;
}

void BTreeOLCDB::Init() {}

void BTreeOLCDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int BTreeOLCDB::PoolThreadInit() {
  int thread_id = allocate();
  // int machine_id = thread_id / (thread_num);
  // tree->AssignId(machine_id, thread_id);
  return thread_id;
}

void BTreeOLCDB::PoolThreadClose(int thread_id) {
  // tree->UnregisterThread(thread_id);
  release(thread_id);
}

#ifdef TIMING_BTREEOLC
std::atomic<size_t> total_read_time = 0;
std::atomic<size_t> total_update_time = 0;
#endif

int BTreeOLCDB::Read(const std::string& table, const std::string& key,
                     const std::vector<std::string>* fields,
                     std::vector<KVPair>& result) {
  bool finish = false;
  auto task = [this, key, &finish]() {
#ifdef TIMING_BTREEOLC
    auto start = std::chrono::high_resolution_clock::now();
#endif
    long key_index = convert_std_hash(key);
    uint64_t val;
    auto success = tree->lookup(key_index, val);
    auto ret = (success) ? DB::kOK : DB::kErrorNoData;
#ifdef TIMING_BTREEOLC
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    total_read_time += duration;
#endif
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

int BTreeOLCDB::ReadInternal(uint64_t key, uint64_t& value) {
#ifdef USE_MSG_QUEUE
  bool finish = false;
  auto task = [this, key, &finish]() {
#ifdef TIMING_BTREEOLC
    auto start = std::chrono::high_resolution_clock::now();
#endif
    std::string str_key = std::to_string(key);
    auto result = tree->Get(nullptr, str_key.c_str(), str_key.length(),
                            utils::IDPair(1,
                                          SimThreadInfo::dispatcher_thread_id));
    auto ret = (result == nullptr) ? DB::kErrorNoData : DB::kOK;
#ifdef TIMING_BTREEOLC
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    total_read_time += duration;
#endif
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
  auto success = tree->lookup(key, value);
  return (success)? DB::kOK : DB::kErrorNoData;
#endif
}

int BTreeOLCDB::Scan(const std::string& table, const std::string& key, int len,
                     const std::vector<std::string>* fields,
                     std::vector<std::vector<KVPair>>& result) {
  throw "Scan: function not implemented!";
}

int BTreeOLCDB::Update(const std::string& table, const std::string& key,
                       std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, &finish]() {
#ifdef TIMING_BTREEOLC
    auto start = std::chrono::high_resolution_clock::now();
#endif
    long key_index = convert_std_hash(key);
    tree->insert(key_index, key_index);
#ifdef TIMING_BTREEOLC
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    total_update_time += duration;
#endif
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

int BTreeOLCDB::UpdateInternal(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  bool finish = false;
  auto task = [this, key, &finish]() {
    std::string str_key = std::to_string(key);
#ifdef TIMING_BTREEOLC
    auto start = std::chrono::high_resolution_clock::now();
#endif
    auto ret = tree->Put(str_key.c_str(), str_key.c_str());
#ifdef TIMING_BTREEOLC
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    total_update_time += duration;
#endif
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret ? DB::kErrorNoData : DB::kOK;
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
  tree->insert(key, key);
  return DB::kOK;
#endif
}

int BTreeOLCDB::Insert(const std::string& table, const std::string& key,
                       std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, values, &finish]() {
    long key_index = convert_std_hash(key);
    tree->insert(key_index, key_index);
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

int BTreeOLCDB::InsertInternal(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  bool finish = false;
  auto task = [this, key, &finish]() {
    std::string str_key = std::to_string(key);
    auto ret = tree->insert(key, key);
#ifdef RETURN_SYNC
    finish = true;
#endif
    return ret ? DB::kErrorNoData : DB::kOK;
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
  tree->insert(key, key);
  return DB::kOK;
#endif
}

int BTreeOLCDB::Delete(const std::string& table, const std::string& key) {
  throw "Delete: function not implemented!";
}

int BTreeOLCDB::DeleteInternal(uint64_t key) {
  throw "Delete: function not implemented!";
}

void BTreeOLCDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
#ifdef TIMING_BTREEOLC
  total_read_time = 0;
  total_update_time = 0;
#endif
}

void BTreeOLCDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
#ifdef TIMING_BTREEOLC
  if (read_cnt.load() != 0) {
    std::cerr << "Average read time: " << total_read_time / read_cnt.load()
              << std::endl;
  }
  if (update_cnt.load() != 0) {
    std::cerr << "Average write time: " << total_update_time / update_cnt.load()
              << std::endl;
  }
#endif
}

int BTreeOLCDB::allocate() {
  for (int i = 0; i < thread_num; ++i) {
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

void BTreeOLCDB::release(int id) {
  if (id >= 0 && id < thread_num) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
            __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc