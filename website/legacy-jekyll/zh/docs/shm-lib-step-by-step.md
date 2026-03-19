---
layout: default
title: shm-lib：一步一步创建最小程序
nav_order: 4
parent: 用户文档
description: 以最小可运行示例，按步骤演示如何初始化 shm-lib、在共享内存上分配/构造对象、并正确清理资源
lang: zh
permalink: /zh/docs/shm-lib-step-by-step.html
---

{% include language-switcher.html %}

## 目标

本页按照“像写 NCCL communicator 示例那样”的风格，给出一个 **从 0 到 1** 的 `shm-lib` 最小程序：  
完成初始化 → 在共享内存上分配/构造对象与容器 → 演示两种释放方式（显式析构 + free / `delete`）→ 退出。

你可以把这里的 `main.cpp` 直接复制到自己的工程里，然后按下面的 CMake 方式链接 `shm-lib`。

## 背景：这个最小程序到底做了什么？

在 `shm-lib` 里，最常见的使用路径是：

- 先调用 `init_cacheable_allocator(...)` 或 `init_cxl_cacheable_allocator(...)` 初始化全局的 `cacheable` 内存池（见 `shm-lib/include/shm/mm.h`）。
- 用 `cacheable.malloc(...)` 进行共享内存分配，然后用 placement new 构造对象；或者用库提供的宏 `ALLOC_AND_CONSTRUCT(...)` 把这两步合并（见 `shm-lib/include/utils/helper.h`）。
- 如果你想让某个 C++ 类默认用共享内存的 `new/delete`，可在类里使用 `NEW_CLASS_ON_SHM`（仅当编译时启用 `USE_CXL` 时生效）。
- 释放时：
  - placement new 构造出来的对象：需要显式调用析构，再 `cacheable.free(...)`（可以用 `DESTROY_AND_DEALLOC(...)` 宏）。
  - `NEW_CLASS_ON_SHM` + `new` 构造出来的对象：直接 `delete`。

## Step 0：先构建 `shm-lib`

如果你还没构建过 `shm-lib`，按中文用户指南里的方式先把静态库构建出来（`libshm-lib.a`）：

```bash
cd shm-lib
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"
cd ../..
```

（对应文档：`website/zh/docs/user-guide.md` 的 “Build shm-lib”。）

## Step 1：准备一个最小工程目录

假设你创建一个目录（示例名叫 `my-shm-app/`）：

```text
my-shm-app/
  CMakeLists.txt
  main.cpp
```

## Step 2：写 `main.cpp`（最小可运行）

这个例子刻意覆盖最常用的三件事：

- 初始化共享内存 allocator（这里用 `init_cacheable_allocator`；你也可以切换到 `init_cxl_cacheable_allocator`）。
- 在共享内存里构造一个 `cxl_vector<int>`（vector 的底层分配器会走 `cacheable`）。
- 演示两种对象生命周期管理方式：
  - `ALLOC_AND_CONSTRUCT` + `DESTROY_AND_DEALLOC`
  - `NEW_CLASS_ON_SHM`（启用 `USE_CXL` 时）+ `new/delete`

```cpp
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "shm/mm.h"
#include "shm/cxl_type.h"
#include "utils/helper.h"
#include "utils/init.h"

struct alignas(64) MyNode {
  NEW_CLASS_ON_SHM
  uint64_t key;
  uint64_t value;
};

int main(int argc, char** argv) {
  // Step 2.1: 选择共享内存后端路径、基址与大小
  // - shm_path: 由 shm-lib 的 SystemMemoryMmapper 使用（通常是 mmap 文件、设备路径等）
  // - base: nullptr 表示让系统选择映射基址
  // - size: 字节数
  const std::string shm_path = (argc >= 2) ? argv[1] : "/tmp/shm-lib-demo.mmap";
  void* base = nullptr;
  const size_t size = 256UL * 1024 * 1024;  // 256 MiB

  // Step 2.2: 初始化 shm-lib 相关全局状态（如果你的工程需要 multi-machine/queue 等能力）
  // 这里只传一个示例 machine_no=0；你的多机编号策略由上层系统决定。
  initialize_shm_related(/*machine_no=*/0);

  // Step 2.3: 初始化 cacheable allocator（local 场景）
  init_cacheable_allocator(shm_path.c_str(), base, size);

  // Step 2.4: 在共享内存上构造 cxl_vector<int>
  // cxl_vector<T> = std::vector<T, CXLAllocator<T>>，其动态扩容内存来自 cacheable
  auto* v = ALLOC_AND_CONSTRUCT(cxl_vector<int>, cacheable.malloc);
  v->reserve(4);
  v->push_back(1);
  v->push_back(2);
  v->push_back(3);

  std::cout << "vector size=" << v->size() << ", v[0]=" << (*v)[0] << std::endl;

  // Step 2.5: 演示 NEW_CLASS_ON_SHM（仅 USE_CXL 下生效；否则这个宏为空）
  // 如果启用了 USE_CXL，new/delete 将走 cacheable.clalign/cacheable.free
  auto* n = new MyNode{.key = 42, .value = 7};
  std::cout << "node key=" << n->key << ", value=" << n->value << std::endl;
  delete n;

  // Step 2.6: 正确销毁 placement-new 构造的对象
  DESTROY_AND_DEALLOC(v, cxl_vector<int>, cacheable.free);

  return 0;
}
```

## Step 3：写 `CMakeLists.txt`（链接 shm-lib）

这里给一个最直接的写法：把你的 app 作为一个可执行文件目标，并链接仓库里的 `shm-lib` 子目录。

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_shm_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../shm-lib ${CMAKE_BINARY_DIR}/shm-lib)

add_executable(my_shm_app main.cpp)
target_link_libraries(my_shm_app PRIVATE shm-lib)
target_include_directories(my_shm_app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../shm-lib/include
)
```

如果你不想 `add_subdirectory`，也可以改成链接已构建好的 `libshm-lib.a`；但那会牵涉更多系统库依赖（`numa` / `memkind` / `tbb` / `ssh` 等），推荐先沿用 `add_subdirectory` 的方式，让库的依赖跟随它自己的 CMake 配置。

## Step 4：构建与运行

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

# 运行：可选传入 shm_path（默认 /tmp/shm-lib-demo.mmap）
./my_shm_app /tmp/shm-lib-demo.mmap
```

## 常见问题

### 1) 为什么一定要“先 init allocator，再在 shm 上构造对象”？

因为 `cxl_vector` 的底层分配器 `CXLAllocator<T>` 会使用全局的 `cacheable` 去分配内存；如果你在初始化之前就构造/扩容容器，分配路径会不正确或直接崩溃。

### 2) `NEW_CLASS_ON_SHM` 为什么“有时生效，有时不生效”？

它在 `shm-lib/include/utils/helper.h` 中被 `USE_CXL` 宏控制：

- 定义了 `USE_CXL`：宏会生成 `operator new/delete`，从而把 `new/delete` 绑定到共享内存 allocator。
- 未定义 `USE_CXL`：宏为空，你的类仍然使用默认堆分配。

