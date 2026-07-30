# Func API（完整清单）

```{note}
本页面由 `website/tools/gen_func_api.py` 从 `shm-lib/include/**` 自动生成“自由函数签名清单”。

你可以在本页顶部的“手写说明区”补充关键参数语义、约束与示例；其余部分保持自动生成，避免漏项与过时。
```

## 手写说明区（建议保留）

- 初始化相关：`init_cacheable_allocator` / `init_cxl_cacheable_allocator` / `initialize_shm_related`
- 跨机/队列相关：`initialize_shm_and_queue` / `initialize_comm` / `add_dispatcher`

---

<!-- AUTO-GENERATED BELOW. DO NOT EDIT BY HAND. -->

## `shm-lib/include/clstale/stale.h`

- `void create_stale_list(void* buffer, size_t size);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void apply_stale(void* addr);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void add_stale(void* addr);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void* process_stale(void* arg);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void* recycle_stale(void* arg);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*

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

- `void notify_destruct(void *ptr);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && CROSS_MACHINE_LOCK_DELE)*
- `void initialize_shm_and_queue();`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void initialize_comm(int machine_no);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void add_dispatcher(msg_type_t type, MsgHandler handler);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `void marker_free_cross_machine(memkind_t kind, void* ptr);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*
- `int get_ptr_machine_index(void* ptr);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))*

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

- `template <> bool key_equal<const char*>(const char* const& a, const char* const& b);`
- `template <typename T> int key_compare(const T& a, const T& b);`
- `template <> int key_compare<const char*>(const char* const& a, const char* const& b);`
- `template <typename T> void assign_to_shm(T& dst, const T& src);`
- `template <> void assign_to_shm<const char*>(const char*& dst, const char* const& src);`
- `template <typename T> void assign_to_local(T& dst, const T& src);`
- `template <> void assign_to_local<const char*>(const char*& dst, const char* const& src);`

## `shm-lib/include/utils/compiler.h`

- `template <typename T> struct value_prefetcher { void operator()(T) {} };`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && !MASSTREE_COMPILER_HH && HAVE_OFF_T_IS_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG && defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && defined(__aarch64__) || defined(__arm__) && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_SHORT == 2 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_INT == 4 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_LONG_LONG == 8 && SIZEOF_LONG == 8 && __x86_64__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && __i386__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 && ALLOW___SYNC_BUILTINS && __x86_64__ && __x86_64__ || HAVE___SYNC_FETCH_AND_ADD_8 && __x86_64__ || HAVE___SYNC_FETCH_AND_OR_8 && !PREFETCH_DEFINED && NOPREFETCH)*
- `\ } MAKE_ALIASABLE(unsigned char);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && !MASSTREE_COMPILER_HH && HAVE_OFF_T_IS_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG && defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && defined(__aarch64__) || defined(__arm__) && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_SHORT == 2 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_INT == 4 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_LONG_LONG == 8 && SIZEOF_LONG == 8 && __x86_64__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && __i386__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 && ALLOW___SYNC_BUILTINS && __x86_64__ && __x86_64__ || HAVE___SYNC_FETCH_AND_ADD_8 && __x86_64__ || HAVE___SYNC_FETCH_AND_OR_8 && !PREFETCH_DEFINED && NOPREFETCH && __i386__ && SIZEOF_LONG_LONG == 16 && SIZEOF_LONG == 4 && SIZEOF_LONG == 4)*
- `template <typename T> struct is_trivially_copyable : public integral_constant<bool, __has_trivial_copy(T)> {};`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && !MASSTREE_COMPILER_HH && HAVE_OFF_T_IS_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG && defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && defined(__aarch64__) || defined(__arm__) && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_SHORT == 2 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_INT == 4 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_LONG_LONG == 8 && SIZEOF_LONG == 8 && __x86_64__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && __i386__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 && ALLOW___SYNC_BUILTINS && __x86_64__ && __x86_64__ || HAVE___SYNC_FETCH_AND_ADD_8 && __x86_64__ || HAVE___SYNC_FETCH_AND_OR_8 && !PREFETCH_DEFINED && NOPREFETCH && __i386__ && SIZEOF_LONG_LONG == 16 && SIZEOF_LONG == 4 && SIZEOF_LONG == 4 && HAVE_INDIFFERENT_ALIGNMENT && HAVE_INDIFFERENT_ALIGNMENT && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && \ && HAVE___HAS_TRIVIAL_COPY)*
- `template <typename T> struct is_trivially_destructible : public integral_constant<bool, __has_trivial_destructor(T)> {};`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && !MASSTREE_COMPILER_HH && HAVE_OFF_T_IS_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG && defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && defined(__aarch64__) || defined(__arm__) && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_SHORT == 2 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_INT == 4 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_LONG_LONG == 8 && SIZEOF_LONG == 8 && __x86_64__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && __i386__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 && ALLOW___SYNC_BUILTINS && __x86_64__ && __x86_64__ || HAVE___SYNC_FETCH_AND_ADD_8 && __x86_64__ || HAVE___SYNC_FETCH_AND_OR_8 && !PREFETCH_DEFINED && NOPREFETCH && __i386__ && SIZEOF_LONG_LONG == 16 && SIZEOF_LONG == 4 && SIZEOF_LONG == 4 && HAVE_INDIFFERENT_ALIGNMENT && HAVE_INDIFFERENT_ALIGNMENT && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && \ && HAVE___HAS_TRIVIAL_COPY && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && \ && HAVE___HAS_TRIVIAL_DESTRUCTOR)*
- `template <typename T, bool use_reference = (!is_reference<T>::value && (!is_trivially_copyable<T>::value || sizeof(T) > sizeof(void*)))> struct fast_argument;`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && !MASSTREE_COMPILER_HH && HAVE_OFF_T_IS_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG_LONG && HAVE_SIZE_T_IS_UNSIGNED_LONG && defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && defined(__aarch64__) || defined(__arm__) && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_SHORT == 2 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_INT == 4 && __x86__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP && ALLOW___SYNC_BUILTINS && __x86__ && (PREFER_X86 || !HAVE___SYNC_FETCH_AND_ADD) && __x86__ && SIZEOF_LONG_LONG == 8 && SIZEOF_LONG == 8 && __x86_64__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && __i386__ && (PREFER_X86 || !HAVE___SYNC_VAL_COMPARE_AND_SWAP_8) && HAVE___SYNC_BOOL_COMPARE_AND_SWAP_8 && ALLOW___SYNC_BUILTINS && __x86_64__ && __x86_64__ || HAVE___SYNC_FETCH_AND_ADD_8 && __x86_64__ || HAVE___SYNC_FETCH_AND_OR_8 && !PREFETCH_DEFINED && NOPREFETCH && __i386__ && SIZEOF_LONG_LONG == 16 && SIZEOF_LONG == 4 && SIZEOF_LONG == 4 && HAVE_INDIFFERENT_ALIGNMENT && HAVE_INDIFFERENT_ALIGNMENT && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && \ && HAVE___HAS_TRIVIAL_COPY && HAVE_CXX_TEMPLATE_ALIAS && HAVE_TYPE_TRAITS && \ && HAVE___HAS_TRIVIAL_DESTRUCTOR)*

## `shm-lib/include/utils/config.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/cpu_dist.h`

- `int get_first_cpu_of_numa_node(int node);`
- `int get_available_cpu_server();`
- `int set_pthread_affinity_attr(int cpu, pthread_attr_t *attr);`
- `int set_pthread_affinity(int cpu);`

## `shm-lib/include/utils/helper.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/init.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/rlock.h`

- `void rlock_lock(rlock_t* l, lock_t lock, owner_t owner);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*
- `void rlock_unlock(rlock_t* l);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*
- `void rlock_st(rlock_t* l, const lock_t value);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*
- `lock_t rlock_ld(rlock_t* l);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*
- `lock_t rlock_cas(rlock_t* l, lock_t expected, lock_t desired);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*
- `lock_t rlock_tas(rlock_t* l);`  *(条件: defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) && NO_CC)*

## `shm-lib/include/utils/sim_api.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/sim_id.h`

_（未发现自由函数声明）_

## `shm-lib/include/utils/timing.h`

- `extern void InitStatistics();`  *(条件: COUNTING)*
- `extern void PrintStatistics();`  *(条件: COUNTING)*
