#include "db/sherman_db.h"

#include <numa.h>

#include <cassert>

#include "Config.h"
#include "core/utils.h"
#include "utils/cpu_dist.h"
#include "utils/sim_id.h"
#include "utils/timing.h"

namespace ycsbc {
#ifdef TIMING_LOAD_BALANCE
int ThreadPool::pool_index = 0;
#endif

ShermanDB::ShermanDB(int thread_num) : DB(), thread_num(thread_num) {
#ifdef NT_SIM
  SimThreadInfo::setup_worker_db_id();
#endif
  DSMConfig config;
  config.machineNR = 1;
  config.cacheConfig.cacheSize = 2; // GB
  dsm = DSM::getInstance(config);
  dsm->registerThread();
  tree = new Tree(dsm);
#ifdef USE_MSG_QUEUE
  this->db_thread_id = this->ThreadInit();
  pool = new ThreadPool(
      thread_num - 1, [this]() { return this->PoolThreadInit(); },
      [this](int thread_id) { this->PoolThreadClose(thread_id); });
#endif
}

ShermanDB::~ShermanDB() {}

void ShermanDB::Init() {}

void ShermanDB::Close() {
#ifdef USE_MSG_QUEUE
  pool->close();
#endif
}

int ShermanDB::PoolThreadInit() {
  dsm->registerThread();
  tree->register_thread();
  return 0;
}

int ShermanDB::ThreadInit() {
  dsm->registerThread();
  tree->register_thread();
  return 0;
}

int ShermanDB::ReadInternal(uint64_t key, uint64_t& value) {
#ifdef TRX_TYPE_STAT
  read_cnt.fetch_add(1);
#endif
  auto res = tree->search(key, value);
  return res ? DB::kOK : DB::kErrorNoData;
}

int ShermanDB::UpdateInternal(uint64_t key, uint64_t value) {
#ifdef TRX_TYPE_STAT
  update_cnt.fetch_add(1);
#endif
  // TODO: Update with the real key is extremely slow, checking the reason
  tree->insert(utils::Random64(), value);
  return DB::kOK;
}

int ShermanDB::InsertInternal(uint64_t key, uint64_t value) {
#ifdef TRX_TYPE_STAT
  insert_cnt.fetch_add(1);
#endif
  tree->insert(key, value);
  return DB::kOK;
}

int ShermanDB::DeleteInternal(uint64_t key) {
  tree->del(key);
  return DB::kOK;
}

void ShermanDB::InitStats() {
  read_cnt.store(0);
  update_cnt.store(0);
  insert_cnt.store(0);
  // tree->total_count = 0;
  // tree->total_read = 0;
}

void ShermanDB::GetStats() {
  std::cerr << "Read Count: " << read_cnt.load() << " ";
  std::cerr << "Update Count: " << update_cnt.load() << " ";
  std::cerr << "Insert Count: " << insert_cnt.load() << std::endl;
  // std::cerr << "Average GetNode: " << tree->GetTime();
}

}  // namespace ycsbc