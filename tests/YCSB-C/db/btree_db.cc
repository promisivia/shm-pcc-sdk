#include "db/btree_db.h"

#include "core/utils.h"

namespace ycsbc {

BTreeDB::BTreeDB(int thread_num)
    : DB(), tree(GetEmptyBTree()), bits(thread_num), thread_num(thread_num) {
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

BTreeDB::~BTreeDB() {
#ifdef USE_MSG_QUEUE
  this->ThreadClose(this->db_thread_id);
#endif
  DestroyBTree(tree);
}

void BTreeDB::Init() {}

void BTreeDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int BTreeDB::PoolThreadInit() {
  int thread_id = allocate();
  // tree->AssignGCID(thread_id);
  return thread_id;
}

void BTreeDB::PoolThreadClose(int thread_id) {
  // tree->UnregisterThread(thread_id);
  release(thread_id);
}

int BTreeDB::ThreadInit() {
  int thread_id = allocate();
  // tree->AssignGCID(thread_id);
  return thread_id;
}

void BTreeDB::ThreadClose(int thread_id) {
  // tree->UnregisterThread(thread_id);
  release(thread_id);
}

int BTreeDB::Read(const std::string& table, const std::string& key,
                  const std::vector<std::string>* fields,
                  std::vector<KVPair>& result) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long int key_index = convert_std_hash(key);
    rwlock.lock_shared();
    auto ret = tree->find(key_index);
    rwlock.unlock_shared();
#ifdef RETURN_SYNC
    finish = true;
#endif
    (void)ret;
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

int BTreeDB::ReadInternal(uint64_t key, uint64_t& value) {
  rwlock.lock_shared();
  auto result = tree->find(key);
  rwlock.unlock_shared();
  value = result.data();
  return DB::kOK;
}

int BTreeDB::Scan(const std::string& table, const std::string& key, int len,
                  const std::vector<std::string>* fields,
                  std::vector<std::vector<KVPair>>& result) {
  throw "Scan: function not implemented!";
}

int BTreeDB::Update(const std::string& table, const std::string& key,
                    std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = (long)convert_std_hash(key);
    rwlock.lock();
    tree->insert((long)key_index, (long)key_index);
    rwlock.unlock();
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

int BTreeDB::UpdateInternal(uint64_t key, uint64_t value) {
  rwlock.lock();
  tree->insert((long)key, (long)value);
  rwlock.unlock();
  return DB::kOK;
}

int BTreeDB::Insert(const std::string& table, const std::string& key,
                    std::vector<KVPair>& values) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = (long)convert_std_hash(key);
    rwlock.lock();
    tree->insert((long)key_index, (long)key_index);
    rwlock.unlock();
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

int BTreeDB::InsertInternal(uint64_t key, uint64_t value) {
  rwlock.lock();
  tree->insert((long)key, (long)value);
  rwlock.unlock();
  return DB::kOK;
}

int BTreeDB::Delete(const std::string& table, const std::string& key) {
  bool finish = false;
  auto task = [this, key, &finish]() {
    long key_index = convert_std_hash(key);
    auto ret = tree->erase(key_index) ? DB::kOK : DB::kErrorNoData;
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

int BTreeDB::DeleteInternal(uint64_t key) {
  rwlock.lock();
  auto ret = tree->erase(key) ? DB::kOK : DB::kErrorNoData;
  rwlock.unlock();
  return ret;
}

void BTreeDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
}

void BTreeDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
}

int BTreeDB::allocate() {
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

void BTreeDB::release(int id) {
  if (id >= 0 && id < thread_num) {
    bits[id].store(0);
  } else {
    fprintf(stderr, "[%s:%d][%s] thread %lx exit 1\n", __FILE__, __LINE__,
            __func__, pthread_self());
    exit(1);
  }
}

}  // namespace ycsbc