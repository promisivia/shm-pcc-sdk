#include "db/bwtree_db.h"

#include <numa.h>

#include <cassert>

#include "utils/cpu_dist.h"
#include "utils/timing.h"
#include "core/utils.h"
#include "utils/sim_id.h"
#include "utils/timing.h"
#include "db/utils.h"


namespace ycsbc {
#ifdef TIMING_LOAD_BALANCE
int ThreadPool::pool_index = 0;
#endif

BwTreeDB::BwTreeDB(int thread_num)
    : DB(), tree(GetEmptyTree(true)), thread_num(thread_num), bits(thread_num) {
#ifdef NT_SIM
  SimThreadInfo::setup_worker_db_id();
#endif
  if (tree != nullptr) {
    // Update the GC array
    tree->UpdateThreadLocal(thread_num);
    for (int i = 0; i < thread_num; ++i) {
      bits[i].store(false);
    }
#ifdef USE_MSG_QUEUE
    this->db_thread_id = this->ThreadInit();
    pool = new ThreadPool(
        thread_num - 1, SimThreadInfo::dispatcher_thread_count, [this]() { return this->PoolThreadInit(); },
        [this](int thread_id) { this->PoolThreadClose(thread_id); });
#endif
  }
}

BwTreeDB::~BwTreeDB() {
  this->ThreadClose(this->db_thread_id);
  tree->UpdateThreadLocal(1);
  DestroyTree(tree, true);
}

void BwTreeDB::Init() {}

void BwTreeDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int BwTreeDB::PoolThreadInit() {
  int thread_id = allocate();
  tree->AssignGCID(thread_id);
  return thread_id;
}

void BwTreeDB::PoolThreadClose(int thread_id) {
  tree->UnregisterThread(thread_id);
  release(thread_id);
}

int BwTreeDB::ThreadInit() {
  int thread_id = allocate();
  tree->AssignGCID(thread_id);
  return thread_id;
}

void BwTreeDB::ThreadClose(int thread_id) {
  tree->UnregisterThread(thread_id);
  release(thread_id);
}

int BwTreeDB::Read(const std::string& table, const std::string& key,
                   const std::vector<std::string>* fields,
                   std::vector<KVPair>& result) {
  int finish = false;
  auto task = [this, key, &finish]() {
    long key_index = convert_std_hash(key);
    std::vector<long> val;
    tree->GetValue(key_index, val);
    auto ret = (*val.begin() > 0) ? DB::kOK : DB::kErrorNoData;
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

int BwTreeDB::ReadInternal(uint64_t key, uint64_t &value) {
#ifdef TRX_TYPE_STAT
  read_cnt.fetch_add(1);
#endif
  std::vector<long> val;
  tree->GetValue(key, val);
  if (val.size() == 0) {
    return DB::kErrorNoData;
  }
  value = val[0];
  return DB::kOK;
}

int BwTreeDB::Scan(const std::string& table, const std::string& key, int len,
                   const std::vector<std::string>* fields,
                   std::vector<std::vector<KVPair>>& result) {
#ifdef USE_MSG_QUEUE
  throw "Scan: function not implemented!";

// #ifdef RETURN_SYNC
//   int return_value = 0xF980AB;
//   pool->enqueue(RandomThreadNum() % thread_num, SCAN, tree, key,
//   &return_value);
// #ifdef USE_MWAIT
//   _umonitor(&return_value);
//   while (return_value == 0xF980AB) {
//     _umwait(1, 0);
//   }
// #else
//   while (return_value == 0xF980AB);
// #endif
//   auto task = [this, key]() {
//     for (int i = 0; i < len; i++) {
//       if (element_iterator.IsEnd()) {
//         break;
//       }
//       auto& element = *element_iterator;
//       if (element.second != element.first + 1) {
//         printf("Scan Error: %ld %ld\n", element.first + 1, element.second);
//       }
//       element_iterator++;
//     }
//     return DB::kOK;
//   };
//   auto future = pool->enqueue(RandomThreadNum() % thread_num, task);
//   future_queue.push(future);
// #else
// #endif
#else
  long key_index = convert_std_hash(key);
  auto element_iterator = tree->Begin(key_index);
  for (int i = 0; i < len; i++) {
    if (element_iterator.IsEnd()) {
      break;
    }
    auto& element = *element_iterator;
    if (element.second != element.first + 1) {
      std::cerr << "Read Error: " << element.second << " " << element.first + 1
                << std::endl;
      return DB::kErrorNoData;
    }
    result.emplace_back();
    element_iterator++;
  }
#endif
  return DB::kOK;
}

int BwTreeDB::Update(const std::string& table, const std::string& key,
                     std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = convert_std_hash(key);
    // long rand_key = utils::RandomValueNum();    
    auto ret = tree->Insert(key_index, key_index + 1) ? DB::kOK : DB::kErrorNoData;
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

int BwTreeDB::UpdateInternal(uint64_t key, uint64_t value) {
#ifdef TRX_TYPE_STAT
  update_cnt.fetch_add(1);
#endif
  // TODO: Update with the real key is extremely slow, checking the reason
  auto ret = tree->Insert(utils::Random64(), value) ? DB::kOK : DB::kErrorNoData;
  return ret;
}

int BwTreeDB::Insert(const std::string& table, const std::string& key,
                     std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = convert_std_hash(key);
    auto ret = tree->Insert(key_index, key_index + 1) ? DB::kOK : DB::kErrorNoData;
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

int BwTreeDB::InsertInternal(uint64_t key, uint64_t value) {
#ifdef TRX_TYPE_STAT
  insert_cnt.fetch_add(1);
#endif
  auto ret = tree->Insert(key, value) ? DB::kOK : DB::kErrorNoData;
  return ret;
}

int BwTreeDB::Delete(const std::string& table, const std::string& key) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = convert_std_hash(key);
    auto ret = tree->Delete(key_index, key_index + 1);
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

int BwTreeDB::DeleteInternal(uint64_t key) {
  auto ret = tree->Delete(key, key);
  return ret;
}

void BwTreeDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
  // tree->total_count = 0;
  // tree->total_read = 0;
}

void BwTreeDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
  // std::cerr << "Average GetNode: " << tree->GetTime();
  // tree->reportTimers();
}

int BwTreeDB::allocate() {
  for (int i = 0; i < thread_num; ++i) {
    uint8_t expected = 0;
    if (bits[i].compare_exchange_strong(expected, 1)) {
      return i;
    }
  }
  fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__, __func__, pthread_self());
  exit(1);
  return -1;
}

void BwTreeDB::release(int id) {
  if (id >= 0 && id < thread_num) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__, __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc