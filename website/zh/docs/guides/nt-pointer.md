# `nt<T>`使用指南

本项目里的 `nt<T>` 类似传统C++编程中的原子变量atomic<T>，但是加了绕过cache的语义，当 `nt=true` 时，这些原子访问会触发“绕过/刷新 cache（或用队列/仿真去模拟）”的额外语义，用来在一个机器之间没有缓存一致性的场景下，保证操作的可见性。

## 1. 这几种 “nt 原子/指针” 的区别

头文件边界：

- `nt<T>` / `aligned_nt<T>`：`shm-lib/include/utils/atomic_variable.h`
- `nt_pointer<T>` / `nt_pointer<T[]>`：`shm-lib/include/utils/atomic_pointer.h`

类型与用途一览：

- `nt<T>`：存放“值”的原子封装（接口风格接近 `std::atomic<T>`），提供 `load/store/fetch_add/fetch_sub/fetch_xor/compare_exchange_strong/flush` 等。
- `aligned_nt<T>`：接口基本同 `nt<T>`，但内部值 `alignas(CACHE_LINE_SIZE)`；用于热点变量避免 false sharing（例如计数器/状态字）。
- `nt_pointer<T>`：存放“指针”的原子封装（接口风格接近 `std::atomic<T*>`），提供 `load/store/compare_exchange_strong/flush`，并重载 `->`、`*`、隐式转 `T*` 等。
- `nt_pointer<T[]>`：`nt_pointer` 的数组特化，额外提供 `allocate(size, ...)`、`operator[]`、（部分宏分支下）`flush_elements(size)`、`free()`，用于“数组指针 + 数组元素构造/析构”。

## 2. `nt=true` 到底会做什么？（按编译宏分支解释）

`nt=true` 的“真实含义”取决于编译宏，常见有三条路径：

| 编译配置 | `nt=true` 时的行为（概念） | 说明 |
|---|---|---|
| `QUEUE_SIM_CAS=1` | 把 load/store/CAS 作为“请求”挂到 `AtomicRequestQueue`，用 busy-wait 模拟往返与处理延迟 | 不等价于真实 `clflush/clwb`；用于做延迟建模 |
| `QUEUE_SIM_CAS=0` 且 `NT_SIM=1` | 使用“每机本地 cache 副本”：`nt=true` 访问共享值并刷新本地 cache；`nt=false` 只读写本地 cache（可能陈旧） | 适合做“无相干/弱相干”实验 |
| `QUEUE_SIM_CAS=0` 且 `NT_SIM=0` | `load(nt=true)` 先对该原子对象做 `clflush`；`store/CAS(nt=true)` 对该原子对象做 `clwb` | 这条路径最接近“显式 cache 刷新/写回”直觉（实现见 `utils/bypass_cache.h`） |

### 2.1 默认 `nt` 是 true 还是 false？

在 `atomic_variable.h` / `atomic_pointer.h` 里，很多接口的默认参数形如：

- 若定义了 `NO_CC`（或指针侧的 `USE_NO_CC_QUEUE`），默认 `nt=true`
- 否则默认 `nt=false`

也就是说：**同一份代码在不同宏配置下，“不写 `nt`”可能意味着完全不同的成本/语义**。关键路径建议显式传 `nt`，避免误判。

## 3. `nt<T>` 与 `aligned_nt<T>`：怎么用？

`nt<T>` 适合放在共享结构体/节点里，作为同步原语，同时在需要时触发 `nt` 语义。

```cpp
#include "utils/atomic_variable.h"

nt<uint64_t> counter{0};

counter.fetch_add(1, /*nt=*/true);
auto v = counter.load(std::memory_order_acquire, /*nt=*/true);

uint64_t expected = v;
bool ok = counter.compare_exchange_strong(expected, v + 1,
                                          std::memory_order_acq_rel,
                                          /*nt=*/true);
```

`aligned_nt<T>` 的用法完全一致，主要区别是内部值按 cache line 对齐，用来减少多个热点原子挤在同一 cache line 的干扰。

注意点：

- `nt` 只作用于“这个原子对象本身”的访问，不会替你把其它内存（例如你刚写过的一大段结构体字段）自动写回。
- `flush()` 的含义也随宏变化：在 `NT_SIM` 下相当于让本机 cache 失效；在队列仿真下会产生请求；在 `clflush/clwb` 路径下会执行 cache 操作。

## 4. `nt_pointer<T>`：怎么用？

`nt_pointer<T>` 用来做“指针的原子发布/替换”，常见场景是 lock-free 结构里的 next 指针、根指针、版本指针等。

### 4.1 基本用法

```cpp
#include "utils/atomic_pointer.h"

struct Node { int x; };
nt_pointer<Node> head{nullptr};

Node* p = head.load(std::memory_order_acquire, /*nt=*/true);
head.store(p, std::memory_order_release, /*nt=*/true);
```

### 4.2 重要差异：`compare_exchange_strong()` 的 expected **不会被写回**

`nt_pointer<T>::compare_exchange_strong` 的签名是 `T* expected`（按值传递），不是 `T*& expected` / `T** expected`。这意味着：

- CAS 失败时，**它不会把“观察到的当前指针值”写回给调用方**（不像 `std::atomic<T*>` 那样会更新 expected）。

推荐写法是“失败就自己 reload 一次”：

```cpp
Node* expected = head.load(std::memory_order_acquire, /*nt=*/true);
Node* desired = /*...*/;
while (!head.compare_exchange_strong(expected, desired,
                                     std::memory_order_acq_rel,
                                     /*nt=*/true)) {
  expected = head.load(std::memory_order_acquire, /*nt=*/true);
}
```

### 4.3 `allocate()`/`free()` 不是“共享内存分配器”

`nt_pointer<T>::allocate(args...)` 内部直接 `new T(...)`（进程堆上分配）。如果你的目标是“跨进程共享对象”，不要依赖它：

- 需要自己用共享内存分配器（例如 `cacheable.malloc` + placement new）构造对象，再 `store()` 发布指针；
- 相关背景见 [内存分配与对象生命周期](memory-allocation.md)。

同理，`nt_pointer<T>::free()` 在部分分支里是空实现/不对称释放；把它当作调试便利函数即可，不要当成通用生命周期管理。

## 5. `nt_pointer<T[]>`（数组特化）：怎么用？分配在哪？

数组特化常见接口：

- `allocate(size, ctor_args...)`：分配并构造 `size` 个元素（若元素本身是 `nt_pointer<U>`，会递归调用其 `allocate`）
- `operator[](i)`：访问第 `i` 个元素
- `free()`：析构元素并释放底层数组
- `flush_elements(size)`：**仅在部分宏分支存在**，用于对数组元素所在区域做一次 cache 操作（具体是 `clflush` 还是仿真由实现决定）

### 5.1 最常见的坑：不同宏下，`allocate()` 用的分配器不一样

`nt_pointer<T[]>` 的 `allocate()` 不是固定走共享内存池，具体取决于宏分支：

- `QUEUE_SIM_CAS=1`：用 `::malloc` / `::free`（进程堆），不在共享内存池里
- `QUEUE_SIM_CAS=0` 且 `NT_SIM=0`：用 `cacheable.malloc` / `cacheable.free`（共享内存池）
- `QUEUE_SIM_CAS=0` 且 `NT_SIM=1`：用 `new T[size]`（进程堆）

因此：

- 如果你需要“跨进程共享数组”，优先使用 **`QUEUE_SIM_CAS=0` 且 `NT_SIM=0`** 的构建配置，或干脆不要用 `allocate()`，改为自己显式分配（确保分配器与你的部署形态匹配）。

## 6. 选型建议（什么时候用 `nt`，什么时候不用？）

- 你在做 “NO_CC/弱相干/远端内存” 实验：让默认 `nt` 生效即可，但建议在关键点显式写 `nt` 并在 PR/实验脚本里记录宏组合（例如 `NO_CC=1`、`QUEUE_SIM_CAS=0/1`、`NT_SIM=0/1`）。
- 你只是在普通 cache 相干机器上跑功能/性能：大多数情况下 **不需要** `nt=true`；不要无脑把 `nt` 当成“更正确”。
- 你要做“指针发布 + 数据持久化/写回”：`nt_pointer` 的 `clwb`（或仿真请求）只覆盖“指针自身的 cache line”，不覆盖指针指向的对象；对象字段的写回需要你自己对相应内存区做 flush/writeback。

## 7. 参考实现（想看细节从这里跳）

- `shm-lib/include/utils/atomic_variable.h`
- `shm-lib/include/utils/atomic_pointer.h`
- `shm-lib/include/utils/atomic_queued_variable.h`（`QUEUE_SIM_CAS=1`）
- `shm-lib/include/utils/atomic_queued_pointer.h`（`QUEUE_SIM_CAS=1`）
- `shm-lib/include/utils/bypass_cache.h`（`clflush/clwb/mfence`）
