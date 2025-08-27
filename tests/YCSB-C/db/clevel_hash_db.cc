#include "db/clevel_hash_db.h"

#include <libpmemobj/base.h>

#include <cstdint>
#include <filesystem>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include "utils/sim_id.h"

namespace ycsbc {
CLevelHashDB::CLevelHashDB(int thread_num)
    : DB(),
      thread_num(thread_num) {
#ifndef LOCAL_NO_CC
  std::string clevel_path = "/dev/shm/cxl-clevel";
  std::string layout = "clevel_hash";
  if (std::filesystem::exists(clevel_path)) {
    std::filesystem::remove_all(clevel_path);
  }
  pop = nvobj::pool<root>::create(clevel_path, layout, 1024ul*1024*1024*4,
                                  S_IWUSR | S_IRUSR);
  auto proot = pop.root();

  nvobj::transaction::manual tx(pop);

  proot->cons = nvobj::make_persistent<persistent_map_type>();
  proot->cons->set_thread_num(thread_num);

  nvobj::transaction::commit();
#else
  level = new clevel_hash_64();
  if (level != nullptr) {
    level->set_thread_num(thread_num);
  }
#endif
#ifdef USE_MSG_QUEUE
  this->db_thread_id = this->ThreadInit();
  pool = new ThreadPool(
      thread_num - 1, 12, [this]() { return this->PoolThreadInit(); },
      [this](int thread_id) { this->PoolThreadClose(thread_id); });
#endif
}

CLevelHashDB::~CLevelHashDB() {
#ifdef LOCAL_NO_CC
  delete level;
#else
  pop.close();
#endif
}

int CLevelHashDB::ReadInternal(uint64_t key, uint64_t &value) {
#ifdef LOCAL_NO_CC
  auto ret = level->search(key, SimThreadInfo::worker_thread_id);
#else
  auto map = pop.root()->cons;
  auto ret = map->search(persistent_map_type::key_type(key));
#endif
  if (ret.found) {
    value = ret.value->second;
    return DB::kOK;
  } else {
    return DB::kErrorNoData;
  }
}

int CLevelHashDB::UpdateInternal(uint64_t key, uint64_t value) {
  std::pair<uint64_t, uint64_t> pair(key, value);
#ifdef LOCAL_NO_CC
  level->update(pair, SimThreadInfo::worker_thread_id);
#else
  auto map = pop.root()->cons;
  map->update(persistent_map_type::value_type(pair),
              SimThreadInfo::worker_thread_id);
#endif
  return DB::kOK;
}

int CLevelHashDB::InsertInternal(uint64_t key, uint64_t value) {
  std::pair<uint64_t, uint64_t> pair(key, value);
#ifdef LOCAL_NO_CC
  level->insert(pair, SimThreadInfo::worker_thread_id, key);
#else
  auto map = pop.root()->cons;
  map->insert(persistent_map_type::value_type(pair),
              SimThreadInfo::worker_thread_id, key);
#endif
  return DB::kOK;
}

int CLevelHashDB::DeleteInternal(uint64_t key) {
#ifdef LOCAL_NO_CC
  auto ret = level->erase(key, SimThreadInfo::worker_thread_id);
#else
  auto map = pop.root()->cons;
  auto ret = map->erase(persistent_map_type::key_type(key),
                        SimThreadInfo::worker_thread_id);
#endif
  if (ret.found) {
    return DB::kOK;
  } else {
    return DB::kErrorNoData;
  }
}
}  // namespace ycsbc