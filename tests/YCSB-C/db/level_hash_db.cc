#include "db/level_hash_db.h"

#include <string>
#include <vector>

#include "db/utils.h"
#include "core/utils.h"
// #include "lib/string_hashtable.h"

using std::string;
using std::vector;

namespace ycsbc {

LevelHashDB::LevelHashDB(int thread_num) {
  level = level_init(19);
  level->thread_num = thread_num;
#ifdef USE_MSG_QUEUE
  pool = new ThreadPool(
      thread_num - 1, [this]() { return 0; },
      [this](int thread_id) { return; });
#endif
}

LevelHashDB::~LevelHashDB() {}

void LevelHashDB::Init() {}

void LevelHashDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int LevelHashDB::Read(const string &table, const string &key,
                      const vector<string> *fields, vector<KVPair> &result) {
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
  convert2(key, convert_key);
  if (level_query(level, convert_key, value) == 0) {
    return DB::kOK;
  } else {
    return DB::kErrorNoData;
  }
}

int LevelHashDB::Read(uint64_t key, uint64_t &value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
    uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
    convert_levelhash(key, convert_key, YCSB_KEY_LEN);
    bool result = level_query(level, convert_key, value);
    auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
    return ret;
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
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *converted_value = new uint8_t[YCSB_VALUE_LEN];
  convert_levelhash(key, convert_key, YCSB_KEY_LEN);
  bool result = level_query(level, convert_key, converted_value);
  auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
  return ret;
#endif
}

int LevelHashDB::Scan(const string &table, const string &key, int len,
                      const vector<string> *fields,
                      vector<vector<KVPair>> &result) {
  throw "Scan: function not implemented!";
}

int LevelHashDB::Update(const string &table, const string &key,
                        vector<KVPair> &values) {
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
  convert2(key, convert_key);
  if (level_update(level, convert_key, value) == 0) {
    return DB::kOK;
  } else {
    return DB::kErrorNoData;
  }
}

int LevelHashDB::Update(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
    uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
    convert_levelhash(key, convert_key, YCSB_KEY_LEN);
    convert_levelhash(key, value, YCSB_VALUE_LEN);
    bool result = level_update(level, convert_key, value);
    auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
    return ret;
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
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *converted_value = new uint8_t[YCSB_VALUE_LEN];
  convert_levelhash(key, convert_key, YCSB_KEY_LEN);
  convert_levelhash(value, converted_value, YCSB_VALUE_LEN);
  bool result = level_update(level, convert_key, converted_value);
  auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
  return ret;
#endif
}

int LevelHashDB::Insert(const string &table, const string &key,
                        vector<KVPair> &values) {
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
  convert2(key, convert_key);
  if (level_insert(level, convert_key, value) == 0) {
    return DB::kOK;
  } else {
    std::cout << "Insert failed" << std::endl;
    return DB::kErrorNoData;
  }
}

int LevelHashDB::Insert(uint64_t key, uint64_t value) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
    uint8_t *value = new uint8_t[YCSB_VALUE_LEN];
    convert_levelhash(key, convert_key, YCSB_KEY_LEN);
    convert_levelhash(key, value, YCSB_VALUE_LEN);
    bool result = level_insert(level, convert_key, value);
    auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
    return ret;
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
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  uint8_t *converted_value = new uint8_t[YCSB_VALUE_LEN];
  convert_levelhash(key, convert_key, YCSB_KEY_LEN);
  convert_levelhash(key, converted_value, YCSB_VALUE_LEN);
  bool result = level_insert(level, convert_key, converted_value);
  auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
  return ret;
#endif
}

int LevelHashDB::Delete(const string &table, const string &key) {
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  convert2(key, convert_key);
  if (level_delete(level, convert_key) == 0) {
    return DB::kOK;
  } else {
    return DB::kErrorNoData;
  }
}

int LevelHashDB::Delete(uint64_t key) {
#ifdef USE_MSG_QUEUE
  int finish = false;
  auto task = [this, key, &finish]() {
    uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
    convert_levelhash(key, convert_key, YCSB_KEY_LEN);
    bool result = level_delete(level, convert_key);
    auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
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
  uint8_t *convert_key = new uint8_t[YCSB_KEY_LEN];
  convert_levelhash(key, convert_key, YCSB_KEY_LEN);
  bool result = level_delete(level, convert_key);
  auto ret = (result == 0) ? DB::kOK : DB::kErrorNoData;
  return ret;
#endif
}

void LevelHashDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
}

void LevelHashDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
}

}  // namespace ycsbc
