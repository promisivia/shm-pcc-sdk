#include "DSM.h"

#include <algorithm>

#include "utils/atomic_variable.h"

thread_local int DSM::thread_id = -1;
thread_local char *DSM::rdma_buffer = nullptr;
thread_local RdmaBuffer DSM::rbuf[define::kMaxCoro];
thread_local uint64_t DSM::thread_tag = 0;

DSM *DSM::getInstance(const DSMConfig &conf) {
  static DSM *dsm = nullptr;
  static WRLock lock;

  lock.wLock();
  if (!dsm) {
    dsm = new DSM(conf);
  } else {
  }
  lock.wUnlock();

  return dsm;
}

DSM::DSM(const DSMConfig &conf)
    : conf(conf), appID(0), cache(conf.cacheConfig) {}

DSM::~DSM() {}

void DSM::registerThread() {
  if (thread_id != -1) return;

  thread_id = appID.fetch_add(1);
  thread_tag = thread_id + (((uint64_t)this->getMyNodeID()) << 32) + 1;

  rdma_buffer = (char *)cache.data + thread_id * 12 * define::MB;

  for (int i = 0; i < define::kMaxCoro; ++i) {
    rbuf[i].set_buffer(rdma_buffer + i * define::kPerCoroRdmaBuf);
  }
}

void nt_memcpy(void* dst, const void* src, size_t size)
{
    uintptr_t p = (uintptr_t)dst;
    uintptr_t s = (uintptr_t)src;
    size_t offset = 0;

    // --- Step 1: fix alignment ---
    // Align dst to 16 bytes
    size_t misalign = p & 0x0F;
    if (misalign) {
        size_t ahead = 16 - misalign;
        if (ahead > size) ahead = size;

        // fall back to safe store
        for (size_t i = 0; i < ahead; ++i)
            ((char*)dst)[i] = ((char*)src)[i];

        offset += ahead;
    }

    // --- Step 2: streaming block copy ---
    size_t n = (size - offset) / 16;

    for (size_t i = 0; i < n; ++i) {
        __m128i v = _mm_loadu_si128((__m128i const*)((char*)src + offset + i*16));
        _mm_stream_si128((__m128i*)((char*)dst + offset + i*16), v);
    }

    // --- Step 3: tail ---
    offset += n * 16;
    for (size_t i = offset; i < size; ++i)
        ((char*)dst)[i] = ((char*)src)[i];

    _mm_sfence();
}

void DSM::read(char *buffer, GlobalAddress gaddr, size_t size, bool signal,
               CoroContext *ctx) {
  // std::memcpy(buffer, reinterpret_cast<char *>(gaddr.val), size);
  nt_memcpy(buffer, reinterpret_cast<char *>(gaddr.val), size);
  if (ctx != nullptr) {
    (*ctx->yield)(*ctx->master);
  }
}

void DSM::read_sync(char *buffer, GlobalAddress gaddr, size_t size,
                    CoroContext *ctx) {
  // std::memcpy(buffer, reinterpret_cast<char *>(gaddr.val), size);
  read(buffer, gaddr, size, true, ctx);
}

void DSM::write(const char *buffer, GlobalAddress gaddr, size_t size,
                bool signal, CoroContext *ctx) {
  // std::memcpy(reinterpret_cast<char *>(gaddr.val), buffer, size);
  nt_memcpy(reinterpret_cast<char *>(gaddr.val), buffer, size);
  if (ctx != nullptr) {
    (*ctx->yield)(*ctx->master);
  }
}

void DSM::write_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                     CoroContext *ctx) {
  write(buffer, gaddr, size, true, ctx);
}

void DSM::write_batch(CXLOpRegion *rs, int k, bool signal, CoroContext *ctx) {
  for (int i = 0; i < k; ++i) {
    char *source = static_cast<char *>(reinterpret_cast<void *>(rs[i].source));
    void *destination = reinterpret_cast<void *>(rs[i].dest);
    // std::memcpy(destination, source, rs[i].size);
    nt_memcpy(destination, source, rs[i].size);
  }
  if (ctx != nullptr) {
    (*ctx->yield)(*ctx->master);
  }
}

void DSM::write_batch_sync(CXLOpRegion *rs, int k, CoroContext *ctx) {
  write_batch(rs, k, true, ctx);
}

// void DSM::cas(GlobalAddress gaddr, uint64_t equal, uint64_t val,
//               uint64_t *rdma_buffer, bool signal, CoroContext *ctx) {

//   if (ctx == nullptr) {
//     rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
//                        remoteInfo[gaddr.nodeID].dsmBase + gaddr.offset,
//                        equal, val, iCon->cacheLKey,
//                        remoteInfo[gaddr.nodeID].dsmRKey[0], signal);
//   } else {
//     rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
//                        remoteInfo[gaddr.nodeID].dsmBase + gaddr.offset,
//                        equal, val, iCon->cacheLKey,
//                        remoteInfo[gaddr.nodeID].dsmRKey[0], true,
//                        ctx->coro_id);
//     (*ctx->yield)(*ctx->master);
//   }
//   uint64_t *remote_ptr = reinterpret_cast<uint64_t *>(gaddr.val);
//   if (ctx == nullptr) {
//     CAS(remote_ptr, equal, val);
//   } else {
//     bool success = CAS(remote_ptr, equal, val);
//     if (success) {
//       ctx->coro_id = success; // 假设 coro_id 用于存储操作结果
//     }
//     (*ctx->yield)(*ctx->master);
//   }
// }

// bool DSM::cas_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
//                    uint64_t *rdma_buffer, CoroContext *ctx) {
//   cas(gaddr, equal, val, rdma_buffer, true, ctx);
//   return equal == *rdma_buffer;
// }

void DSM::write_dm(const char *buffer, GlobalAddress gaddr, size_t size,
                   bool signal, CoroContext *ctx) {
  write(buffer, gaddr, size, signal, ctx);
}

void DSM::write_dm_sync(const char *buffer, GlobalAddress gaddr, size_t size,
                        CoroContext *ctx) {
  write_dm(buffer, gaddr, size, true, ctx);
}

// void DSM::cas_dm(GlobalAddress gaddr, uint64_t equal, uint64_t val,
//                  uint64_t *rdma_buffer, bool signal, CoroContext *ctx) {

//   if (ctx == nullptr) {
//     rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
//                        remoteInfo[gaddr.nodeID].lockBase + gaddr.offset,
//                        equal, val, iCon->cacheLKey,
//                        remoteInfo[gaddr.nodeID].lockRKey[0], signal);
//   } else {
//     rdmaCompareAndSwap(iCon->data[0][gaddr.nodeID], (uint64_t)rdma_buffer,
//                        remoteInfo[gaddr.nodeID].lockBase + gaddr.offset,
//                        equal, val, iCon->cacheLKey,
//                        remoteInfo[gaddr.nodeID].lockRKey[0], true,
//                        ctx->coro_id);
//     (*ctx->yield)(*ctx->master);
//   }
// }

// bool DSM::cas_dm_sync(GlobalAddress gaddr, uint64_t equal, uint64_t val,
//                       uint64_t *rdma_buffer, CoroContext *ctx) {
//   cas_dm(gaddr, equal, val, rdma_buffer, true, ctx);
//   return equal == *rdma_buffer;
// }
