#include "db/masstree_db.h"

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

MasstreeDB::MasstreeDB(int thread_num)
    : thread_num(thread_num), bits(4 * thread_num) {
  // Update the GC array
  tree = std::make_unique<masstree::masstree>();
  for (int i = 0; i < thread_num; ++i) {
    bits[i].store(false);
  }
#ifdef USE_MSG_QUEUE
  pool = new ThreadPool(
      thread_num - 1, [this]() { return 0; },
      [this](int thread_id) { return; });
#endif
}

MasstreeDB::~MasstreeDB() {}

void MasstreeDB::Init() {}

void MasstreeDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int MasstreeDB::PoolThreadInit() {
  int thread_id = allocate();
  // fprintf(stderr, "[%p] init at thread %d\n", (void *)pthread_self(),
  // thread_id);
  return thread_id;
}

void MasstreeDB::PoolThreadClose(int thread_id) { release(thread_id); }

int MasstreeDB::Read(const std::string &table, const std::string &key,
                     const std::vector<std::string> *fields,
                     std::vector<KVPair> &result) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    static thread_local auto t = tree->getThreadInfo();
    auto result = tree->get(key_index, t);
    auto ret = result != nullptr ? DB::kOK : DB::kErrorNoData;
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

int MasstreeDB::ReadInternal(uint64_t key, uint64_t &value) {
  static thread_local auto t = tree->getThreadInfo();
  value = (uint64_t)tree->get(key, t);
  auto ret = value != 0 ? DB::kOK : DB::kErrorNoData;
  return ret;
}

int MasstreeDB::Scan(const std::string &table, const std::string &key, int len,
                     const std::vector<std::string> *fields,
                     std::vector<std::vector<KVPair>> &result) {
  return 0;
}

int MasstreeDB::Update(const std::string &table, const std::string &key,
                       std::vector<KVPair> &values) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    static thread_local auto t = tree->getThreadInfo();
    auto val = new uint64_t(key_index);
    tree->put(key_index, (void *)val, t);
    auto ret = DB::kOK;
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

int MasstreeDB::UpdateInternal(uint64_t key, uint64_t value) {
  static thread_local auto t = tree->getThreadInfo();
  auto val = new uint64_t(value);
  tree->put(key, (void *)val, t);
  auto ret = DB::kOK;
  return ret;
}

int MasstreeDB::Insert(const std::string &table, const std::string &key,
                       std::vector<KVPair> &values) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    static thread_local auto t = tree->getThreadInfo();
    auto val = new uint64_t(key_index);
    tree->put(key_index, (void *)val, t);
    auto ret = DB::kOK;
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

int MasstreeDB::InsertInternal(uint64_t key, uint64_t value) {
  static thread_local auto t = tree->getThreadInfo();
  auto val = new uint64_t(value);
  tree->put(key, (void *)val, t);
  auto ret = DB::kOK;
  return ret;
}

int MasstreeDB::Delete(const std::string &table, const std::string &key) {
  int finish = false;
  auto task = [this, key, &finish]() {
    uint64_t key_index = convert_std_hash(key);
    static thread_local auto t = tree->getThreadInfo();
    tree->del(key_index, t);
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

int MasstreeDB::DeleteInternal(uint64_t key) {
  static thread_local auto t = tree->getThreadInfo();
  tree->del(key, t);
  return DB::kOK;
}

void MasstreeDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
}

void MasstreeDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;

  // Add Masstree-specific statistics
  auto [totalSize, statistics] = tree->getStatistics();
  std::cerr << "=== Masstree Statistics ===" << std::endl;
  std::cerr << "Max B+Tree Height: " << static_cast<uint32_t>(statistics.at("maxBTreeHeight")) << std::endl;
  std::cerr << "Max Trie Depth: " << static_cast<uint32_t>(statistics.at("maxTrieDepth")) << std::endl;
  std::cerr << "Total Nodes: " << static_cast<size_t>(statistics.at("nodeCount")) << std::endl;
  std::cerr << "Leaf Nodes: " << static_cast<size_t>(statistics.at("leafCount")) << std::endl;
  std::cerr << "Internal Nodes: " << static_cast<size_t>(statistics.at("internalCount")) << std::endl;
  std::cerr << "Total Size: " << totalSize << " bytes" << std::endl;
  std::cerr << "===========================" << std::endl;
}

int MasstreeDB::allocate() {
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

void MasstreeDB::release(int id) {
  if (id >= 0 && id < (int)bits.size()) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
            __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc
