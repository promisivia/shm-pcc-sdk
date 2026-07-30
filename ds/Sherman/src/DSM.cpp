#include "DSM.h"

#include <algorithm>
#include <immintrin.h>  // For AVX2/AVX-512
#include <emmintrin.h>  // For SSE2

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

// 针对1KB对齐内存的极致优化版本
#ifdef __AVX512F__
// AVX-512版本：使用64字节块（缓存行大小）
void nt_memcpy(void* dst, const void* src, size_t size)
{
    const char* src_c = (const char*)src;
    char* dst_c = (char*)dst;
    
    // 假设1KB对齐，可以直接使用64字节（缓存行）对齐的块
    const __m512i* src_vec = (const __m512i*)src;
    __m512i* dst_vec = (__m512i*)dst;
    size_t n64 = size / 64;
    
    // 预取第一批数据（提前预取2个缓存行）
    if (n64 > 0) {
        _mm_prefetch(src_c + 128, _MM_HINT_T0);
    }
    
    // 循环展开：每次处理8个64字节块（512字节 = 8个缓存行）
    size_t i = 0;
    for (; i + 7 < n64; i += 8) {
        // 预取未来数据（提前预取8个缓存行）
        if (i + 8 < n64) {
            _mm_prefetch((const char*)src_vec + i * 64 + 512, _MM_HINT_T0);
        }
        
        // 展开8次操作，充分利用流水线
        __m512i v0 = _mm512_loadu_si512(src_vec + i);
        __m512i v1 = _mm512_loadu_si512(src_vec + i + 1);
        __m512i v2 = _mm512_loadu_si512(src_vec + i + 2);
        __m512i v3 = _mm512_loadu_si512(src_vec + i + 3);
        __m512i v4 = _mm512_loadu_si512(src_vec + i + 4);
        __m512i v5 = _mm512_loadu_si512(src_vec + i + 5);
        __m512i v6 = _mm512_loadu_si512(src_vec + i + 6);
        __m512i v7 = _mm512_loadu_si512(src_vec + i + 7);
        
        _mm512_stream_si512(dst_vec + i, v0);
        _mm512_stream_si512(dst_vec + i + 1, v1);
        _mm512_stream_si512(dst_vec + i + 2, v2);
        _mm512_stream_si512(dst_vec + i + 3, v3);
        _mm512_stream_si512(dst_vec + i + 4, v4);
        _mm512_stream_si512(dst_vec + i + 5, v5);
        _mm512_stream_si512(dst_vec + i + 6, v6);
        _mm512_stream_si512(dst_vec + i + 7, v7);
    }
    
    // 处理剩余64字节块
    for (; i < n64; ++i) {
        __m512i v = _mm512_loadu_si512(src_vec + i);
        _mm512_stream_si512(dst_vec + i, v);
    }
    
    // 处理剩余字节（<64字节）
    size_t offset = n64 * 64;
    for (size_t j = offset; j < size; ++j)
        dst_c[j] = src_c[j];
    
    _mm_sfence();
}
#elif defined(__AVX2__)
// AVX2版本：使用64字节块（2个32字节AVX2寄存器 = 1个缓存行）
void nt_memcpy(void* dst, const void* src, size_t size)
{
    const char* src_c = (const char*)src;
    char* dst_c = (char*)dst;
    
    // 假设1KB对齐，使用64字节（缓存行）对齐的块
    const __m256i* src_vec = (const __m256i*)src;
    __m256i* dst_vec = (__m256i*)dst;
    size_t n64 = size / 64;  // 64字节 = 2个AVX2寄存器
    
    // 预取第一批数据（提前预取2个缓存行）
    if (n64 > 0) {
        _mm_prefetch(src_c + 128, _MM_HINT_T0);
    }
    
    // 循环展开：每次处理8个64字节块（512字节 = 8个缓存行）
    size_t i = 0;
    for (; i + 7 < n64; i += 8) {
        // 预取未来数据（提前预取8个缓存行）
        if (i + 8 < n64) {
            _mm_prefetch((const char*)src_vec + i * 64 + 512, _MM_HINT_T0);
        }
        
        // 每个64字节块需要2个AVX2寄存器（32字节×2）
        // 展开8个块 = 16个load + 16个store
        size_t base = i * 2;  // 每个64字节块 = 2个AVX2寄存器
        
        // 块0-3
        __m256i v0_0 = _mm256_loadu_si256(src_vec + base);
        __m256i v0_1 = _mm256_loadu_si256(src_vec + base + 1);
        __m256i v1_0 = _mm256_loadu_si256(src_vec + base + 2);
        __m256i v1_1 = _mm256_loadu_si256(src_vec + base + 3);
        __m256i v2_0 = _mm256_loadu_si256(src_vec + base + 4);
        __m256i v2_1 = _mm256_loadu_si256(src_vec + base + 5);
        __m256i v3_0 = _mm256_loadu_si256(src_vec + base + 6);
        __m256i v3_1 = _mm256_loadu_si256(src_vec + base + 7);
        
        _mm256_stream_si256(dst_vec + base, v0_0);
        _mm256_stream_si256(dst_vec + base + 1, v0_1);
        _mm256_stream_si256(dst_vec + base + 2, v1_0);
        _mm256_stream_si256(dst_vec + base + 3, v1_1);
        _mm256_stream_si256(dst_vec + base + 4, v2_0);
        _mm256_stream_si256(dst_vec + base + 5, v2_1);
        _mm256_stream_si256(dst_vec + base + 6, v3_0);
        _mm256_stream_si256(dst_vec + base + 7, v3_1);
        
        // 块4-7
        base += 8;
        __m256i v4_0 = _mm256_loadu_si256(src_vec + base);
        __m256i v4_1 = _mm256_loadu_si256(src_vec + base + 1);
        __m256i v5_0 = _mm256_loadu_si256(src_vec + base + 2);
        __m256i v5_1 = _mm256_loadu_si256(src_vec + base + 3);
        __m256i v6_0 = _mm256_loadu_si256(src_vec + base + 4);
        __m256i v6_1 = _mm256_loadu_si256(src_vec + base + 5);
        __m256i v7_0 = _mm256_loadu_si256(src_vec + base + 6);
        __m256i v7_1 = _mm256_loadu_si256(src_vec + base + 7);
        
        _mm256_stream_si256(dst_vec + base, v4_0);
        _mm256_stream_si256(dst_vec + base + 1, v4_1);
        _mm256_stream_si256(dst_vec + base + 2, v5_0);
        _mm256_stream_si256(dst_vec + base + 3, v5_1);
        _mm256_stream_si256(dst_vec + base + 4, v6_0);
        _mm256_stream_si256(dst_vec + base + 5, v6_1);
        _mm256_stream_si256(dst_vec + base + 6, v7_0);
        _mm256_stream_si256(dst_vec + base + 7, v7_1);
    }
    
    // 处理剩余64字节块
    for (; i < n64; ++i) {
        size_t base = i * 2;
        __m256i v0 = _mm256_loadu_si256(src_vec + base);
        __m256i v1 = _mm256_loadu_si256(src_vec + base + 1);
        _mm256_stream_si256(dst_vec + base, v0);
        _mm256_stream_si256(dst_vec + base + 1, v1);
    }
    
    // 处理剩余字节（<64字节）
    size_t offset = n64 * 64;
    for (size_t j = offset; j < size; ++j)
        dst_c[j] = src_c[j];
    
    _mm_sfence();
}
#else
// SSE2版本：使用64字节块（4个16字节SSE寄存器 = 1个缓存行）
void nt_memcpy(void* dst, const void* src, size_t size)
{
    const char* src_c = (const char*)src;
    char* dst_c = (char*)dst;
    
    // 假设1KB对齐，使用64字节（缓存行）对齐的块
    const __m128i* src_vec = (const __m128i*)src;
    __m128i* dst_vec = (__m128i*)dst;
    size_t n64 = size / 64;  // 64字节 = 4个SSE寄存器
    
    // 预取第一批数据（提前预取2个缓存行）
    if (n64 > 0) {
        _mm_prefetch(src_c + 128, _MM_HINT_T0);
    }
    
    // 循环展开：每次处理8个64字节块（512字节 = 8个缓存行）
    size_t i = 0;
    for (; i + 7 < n64; i += 8) {
        // 预取未来数据（提前预取8个缓存行）
        if (i + 8 < n64) {
            _mm_prefetch((const char*)src_vec + i * 64 + 512, _MM_HINT_T0);
        }
        
        // 每个64字节块需要4个SSE寄存器（16字节×4）
        // 展开8个块 = 32个load + 32个store
        size_t base = i * 4;  // 每个64字节块 = 4个SSE寄存器
        
        // 块0-3（每个块4个寄存器）
        for (size_t block = 0; block < 4; ++block) {
            __m128i v0 = _mm_loadu_si128(src_vec + base);
            __m128i v1 = _mm_loadu_si128(src_vec + base + 1);
            __m128i v2 = _mm_loadu_si128(src_vec + base + 2);
            __m128i v3 = _mm_loadu_si128(src_vec + base + 3);
            
            _mm_stream_si128(dst_vec + base, v0);
            _mm_stream_si128(dst_vec + base + 1, v1);
            _mm_stream_si128(dst_vec + base + 2, v2);
            _mm_stream_si128(dst_vec + base + 3, v3);
            
            base += 4;
        }
        
        // 块4-7
        for (size_t block = 0; block < 4; ++block) {
            __m128i v0 = _mm_loadu_si128(src_vec + base);
            __m128i v1 = _mm_loadu_si128(src_vec + base + 1);
            __m128i v2 = _mm_loadu_si128(src_vec + base + 2);
            __m128i v3 = _mm_loadu_si128(src_vec + base + 3);
            
            _mm_stream_si128(dst_vec + base, v0);
            _mm_stream_si128(dst_vec + base + 1, v1);
            _mm_stream_si128(dst_vec + base + 2, v2);
            _mm_stream_si128(dst_vec + base + 3, v3);
            
            base += 4;
        }
    }
    
    // 处理剩余64字节块
    for (; i < n64; ++i) {
        size_t base = i * 4;
        for (size_t j = 0; j < 4; ++j) {
            __m128i v = _mm_loadu_si128(src_vec + base + j);
            _mm_stream_si128(dst_vec + base + j, v);
        }
    }
    
    // 处理剩余字节（<64字节）
    size_t offset = n64 * 64;
    for (size_t j = offset; j < size; ++j)
        dst_c[j] = src_c[j];
    
    _mm_sfence();
}
#endif

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
