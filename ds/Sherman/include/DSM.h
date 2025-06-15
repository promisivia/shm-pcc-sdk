#pragma once

#include <atomic>

#include "Cache.h"
#include "Config.h"
#include "GlobalAddress.h"
#include "RdmaBuffer.h"

class DSMKeeper;
class Directory;

class DSM {

public:
  struct CXLOpRegion {
    uint64_t source;
    uint64_t dest;
    uint64_t size;
  };
  // obtain netowrk resources for a thread
  void registerThread();

  // clear the network resources for all threads
  void resetThread() { appID.store(0); }

  static DSM *getInstance(const DSMConfig &conf);

  uint16_t getMyNodeID() { return myNodeID; }
  uint16_t getMyThreadID() { return thread_id; }
  uint16_t getClusterSize() { return conf.machineNR; }
  uint64_t getThreadTag() { return thread_tag; }

  // RDMA operations
  // buffer is registered memory
  void read(char *buffer, GlobalAddress gaddr, size_t size, bool signal = true,
            CoroContext *ctx = nullptr);
  void read_sync(char *buffer, GlobalAddress gaddr, size_t size,
                 CoroContext *ctx = nullptr);

  void write(const char *buffer, GlobalAddress gaddr, size_t size,
             bool signal = true, CoroContext *ctx = nullptr);
  void write_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                  CoroContext *ctx = nullptr);

  void write_batch(CXLOpRegion *rs, int k, bool signal = true,
                   CoroContext *ctx = nullptr);
  void write_batch_sync(CXLOpRegion *rs, int k, CoroContext *ctx = nullptr);

  // void write_faa(CXLOpRegion &write_ror, CXLOpRegion &faa_ror, uint64_t add_val,
  //                bool signal = true, CoroContext *ctx = nullptr);
  // void write_faa_sync(CXLOpRegion &write_ror, CXLOpRegion &faa_ror,
  //                     uint64_t add_val, CoroContext *ctx = nullptr);

  // void write_cas(CXLOpRegion &write_ror, CXLOpRegion &cas_ror, uint64_t equal,
  //                uint64_t val, bool signal = true, CoroContext *ctx = nullptr);
  // void write_cas_sync(CXLOpRegion &write_ror, CXLOpRegion &cas_ror,
  //                     uint64_t equal, uint64_t val, CoroContext *ctx = nullptr);

  // void cas(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //          uint64_t *rdma_buffer, bool signal = true,
  //          CoroContext *ctx = nullptr);
  // bool cas_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //               uint64_t *rdma_buffer, CoroContext *ctx = nullptr);

  // void cas_read(CXLOpRegion &cas_ror, CXLOpRegion &read_ror, uint64_t equal,
  //               uint64_t val, bool signal = true, CoroContext *ctx = nullptr);
  // bool cas_read_sync(CXLOpRegion &cas_ror, CXLOpRegion &read_ror,
  //                    uint64_t equal, uint64_t val, CoroContext *ctx = nullptr);

  // void cas_mask(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //               uint64_t *rdma_buffer, uint64_t mask = ~(0ull),
  //               bool signal = true);
  // bool cas_mask_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //                    uint64_t *rdma_buffer, uint64_t mask = ~(0ull));

  // void faa_boundary(GlobalAddress gaddr, uint64_t add_val,
  //                   uint64_t *rdma_buffer, uint64_t mask = 63,
  //                   bool signal = true, CoroContext *ctx = nullptr);
  // void faa_boundary_sync(GlobalAddress gaddr, uint64_t add_val,
  //                        uint64_t *rdma_buffer, uint64_t mask = 63,
  //                        CoroContext *ctx = nullptr);

  // for on-chip device memory
  void read_dm(char *buffer, GlobalAddress gaddr, size_t size,
               bool signal = true, CoroContext *ctx = nullptr);
  void read_dm_sync(char *buffer, GlobalAddress gaddr, size_t size,
                    CoroContext *ctx = nullptr);

  void write_dm(const char *buffer, GlobalAddress gaddr, size_t size,
                bool signal = true, CoroContext *ctx = nullptr);
  void write_dm_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                     CoroContext *ctx = nullptr);

  // void cas_dm(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //             uint64_t *rdma_buffer, bool signal = true,
  //             CoroContext *ctx = nullptr);
  // bool cas_dm_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //                  uint64_t *rdma_buffer, CoroContext *ctx = nullptr);

  // void cas_dm_mask(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //                  uint64_t *rdma_buffer, uint64_t mask = ~(0ull),
  //                  bool signal = true);
  // bool cas_dm_mask_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
  //                       uint64_t *rdma_buffer, uint64_t mask = ~(0ull));

  // void faa_dm_boundary(GlobalAddress gaddr, uint64_t add_val,
  //                      uint64_t *rdma_buffer, uint64_t mask = 63,
  //                      bool signal = true, CoroContext *ctx = nullptr);
  // void faa_dm_boundary_sync(GlobalAddress gaddr, uint64_t add_val,
  //                           uint64_t *rdma_buffer, uint64_t mask = 63,
  //                           CoroContext *ctx = nullptr);

private:
  DSM(const DSMConfig &conf);
  ~DSM();

  DSMConfig conf;
  std::atomic_int appID;
  Cache cache;

  static thread_local int thread_id;
  static thread_local char *rdma_buffer;
  static thread_local RdmaBuffer rbuf[define::kMaxCoro];
  static thread_local uint64_t thread_tag;

  uint64_t baseAddr;
  uint32_t myNodeID = 0;

  Directory *dirAgent[NR_DIRECTORY];

public:
  bool is_register() { return thread_id != -1; }

  char *get_rdma_buffer() { return rdma_buffer; }
  RdmaBuffer &get_rbuf(int coro_id) { return rbuf[coro_id]; }

  GlobalAddress alloc(size_t size);
  void free(GlobalAddress addr);
};

inline GlobalAddress DSM::alloc(size_t size) {
  char *taddr;
// #ifdef USE_CXL
//   taddr = reinterpret_cast<char*>(cacheable.malloc(size));
// #else
  taddr = reinterpret_cast<char*>(malloc(size));
// #endif

  GlobalAddress addr;
  addr.val = reinterpret_cast<uint64_t>(taddr);

  return addr;
}

inline void DSM::free(GlobalAddress addr) {
// #ifdef USE_CXL
//   cacheable.free(reinterpret_cast<char *>(addr.val));
// #else
  std::free(reinterpret_cast<char *>(addr.val));
// #endif
}
