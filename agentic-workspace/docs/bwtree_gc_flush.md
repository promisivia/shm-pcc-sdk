# BwTree GC Flush Thread

This note describes the current BwTree GC flush implementation.

## How It Is Enabled

The feature is controlled by `BWTREE_GC_FLUSH_THREAD_DEFAULT`.

By default it is disabled in `ds/BwTree/src/bwtree.h`:

```cpp
#ifndef BWTREE_GC_FLUSH_THREAD_DEFAULT
#define BWTREE_GC_FLUSH_THREAD_DEFAULT false
#endif
```

The YCSB build target that enables it is:

```cmake
add_ycsbc_executable(
  ycsbc_cc_mq_gc_flush
  "USE_MSG_QUEUE;BWTREE_GC_FLUSH_THREAD_DEFAULT=true")
```

Build it with:

```bash
cd /home/wfn/shm-pcc-sdk/tests/YCSB-C
./build.sh cc_mq_gc_flush
```

This produces `build_cc_mq_gc_flush/ycsbc_cc_mq_gc_flush` and links
`./ycsbc` to that binary.

## Runtime Structure

Each `BwTree` owns one `GCFlushWorker` object. When the option is enabled, the
worker starts one dedicated thread in the tree constructor. The worker thread
waits on a condition variable for GC flush requests.

The request payload is a `std::vector<const BaseNode *>`. These are the BwTree
nodes that `PerformGC(thread_id)` has already determined are safe to reclaim.

## PerformGC Flow

At the beginning of `PerformGC(thread_id)`, BwTree computes the minimum active
epoch:

```cpp
uint64_t min_epoch = SummarizeGCEpoch();
```

Then it reads the current thread's garbage chain:

```cpp
GarbageNode *header_p = &GetGCMetaData(thread_id)->header;
GarbageNode *first_p = header_p->next_p;
```

If GC flush is enabled, `PerformGC` walks the reclaimable prefix of the garbage
chain before freeing anything:

```cpp
for (auto *node = first_p;
     node != nullptr && node->delete_epoch < min_epoch;
     node = node->next_p) {
  nodes_to_flush.push_back((const BaseNode *)node->node_p);
}
```

This list is submitted to the dedicated worker:

```cpp
gc_flush_worker.SubmitAndWait(std::move(nodes_to_flush));
```

The worker thread then calls `FlushNodeForGC()` for every listed node, which
currently calls:

```cpp
node_p->FlushNode();
```

Only after the worker reports completion does `PerformGC` continue into the
normal reclaim loop that unlinks garbage nodes and frees memory.

## Synchronization Semantics

The implementation is synchronous from the caller's point of view:

1. `PerformGC` builds the reclaim list.
2. `PerformGC` sends the list to the worker.
3. `PerformGC` waits until all nodes in the list have been flushed.
4. `PerformGC` reclaims the nodes.

This is intentional. The flush worker never races with node deallocation because
the GC thread does not free those nodes until `SubmitAndWait()` returns.

The worker thread is still useful because the cacheline flush work is moved to a
dedicated thread, but the current design does not make reclamation fully
asynchronous.

## Current Granularity

The list is built from all currently reclaimable nodes at the head of the
thread-local GC chain:

```cpp
node != nullptr && node->delete_epoch < min_epoch
```

The vector reserves 1024 entries because the normal BwTree GC trigger threshold
is 1024 garbage nodes, but it does not hard-cap the list at 1024. If more than
1024 nodes are reclaimable in that prefix, they are also included.

## Current Limitations

- The option is per `BwTree` instance, not a global runtime switch.
- The YCSB enabled build currently uses a compile-time macro.
- The flush worker processes one request at a time.
- `PerformGC` waits for flush completion before freeing, so the implementation
  prioritizes correctness over fully asynchronous reclamation.
- The flushed object is the `BaseNode` behind each `GarbageNode`; the
  `GarbageNode` metadata object itself is not separately flushed by this path.

# Results
  也就是当前小样本下，flush 1024 个 node 的 cacheline 大约：

  - 平均：441 us
  - 最大：671 us
  - 平均每 node：431 ns