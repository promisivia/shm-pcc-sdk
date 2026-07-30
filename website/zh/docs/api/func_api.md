<!-- AUTO-GENERATED BELOW. DO NOT EDIT BY HAND. -->

## `shm-lib/include/clstale/stale.h`

- `void create_stale_list(void* buffer, size_t size);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*
- `void apply_stale(void* addr);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*
- `void add_stale(void* addr);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*
- `void* process_stale(void* arg);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*
- `void* recycle_stale(void* arg);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*

## `shm-lib/include/clstale/stale_test.h`

_（未发现自由函数声明）_

## `shm-lib/include/connection/establish.h`

_（未发现自由函数声明）_

## `shm-lib/include/msg/mpmc_queue.h`

_（未发现自由函数声明）_

## `shm-lib/include/msg/msg_collector.h`

_（未发现自由函数声明）_

## `shm-lib/include/msg/msg_dispatcher.h`

_（未发现自由函数声明）_

## `shm-lib/include/msg/msg_queue.h`

_（未发现自由函数声明）_

## `shm-lib/include/msg/spsc_queue.h`

_（未发现自由函数声明）_

## `shm-lib/include/occ/occ.h`

_（未发现自由函数声明）_

## `shm-lib/include/replica_help_update/help_update.h`

_（未发现自由函数声明）_

## `shm-lib/include/shm/cxl_type.h`

_（未发现自由函数声明）_

## `shm-lib/include/shm/memkind.h`

_（未发现自由函数声明）_

## `shm-lib/include/shm/mempool.h`

- `void notify_destruct(void *ptr);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)))*
- `void initialize_shm_and_queue();`
- `void initialize_comm(int machine_no);`
- `void add_dispatcher(msg_type_t type, MsgHandler handler);`
- `void marker_free_cross_machine(memkind_t kind, void* ptr);`
- `int get_ptr_machine_index(void* ptr);`

## `shm-lib/include/shm/mm.h`

- `void init_cacheable_allocator(const char *shm_path, void *base, size_t size);`
- `void init_cxl_cacheable_allocator(const char *shm_path, void *base, size_t size);`
- `void init_uncacheable_allocator();`

## `shm-lib/include/utils/atomic_pointer.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/atomic_queued_pointer.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/atomic_queued_variable.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/atomic_variable.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/bypass_cache.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/clp.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/cmd_parser.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/compare.h`

- `template <typename T> bool key_equal(const T& a, const T& b);`
- `template <> bool key_equal<const char*>(const char* const& a, const char* const& b);`
- `template <typename T> int key_compare(const T& a, const T& b);`
- `template <> int key_compare<const char*>(const char* const& a, const char* const& b);`
- `template <typename T> void assign_to_shm(T& dst, const T& src);`
- `template <> void assign_to_shm<const char*>(const char*& dst, const char* const& src);`
- `template <typename T> void assign_to_local(T& dst, const T& src);`
- `template <> void assign_to_local<const char*>(const char*& dst, const char* const& src);`

## `shm-lib/include/utils/compiler.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/config.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/cpu_dist.h`

- `int get_cpu_nr_numa(int node);`
- `int get_first_cpu_of_numa_node(int node);`
- `int get_available_cpu_server();`
- `int set_pthread_affinity_attr(int cpu, pthread_attr_t *attr);`
- `int set_pthread_affinity(int cpu);`

## `shm-lib/include/utils/helper.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/init.h`

- `void initialize_shm_related(int machine_no);`

## `shm-lib/include/utils/rlock.h`

- `void rlock_lock(rlock_t* l, lock_t lock, owner_t owner);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*
- `void rlock_unlock(rlock_t* l);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*
- `void rlock_st(rlock_t* l, const lock_t value);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*
- `lock_t rlock_ld(rlock_t* l);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*
- `lock_t rlock_cas(rlock_t* l, lock_t expected, lock_t desired);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*
- `lock_t rlock_tas(rlock_t* l);`  *(条件: (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) && (NO_CC))*

## `shm-lib/include/utils/sim_api.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/sim_id.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/timing.h`

- `extern void InitStatistics();`  *(条件: (COUNTING))*
- `extern void PrintStatistics();`  *(条件: (COUNTING))*
