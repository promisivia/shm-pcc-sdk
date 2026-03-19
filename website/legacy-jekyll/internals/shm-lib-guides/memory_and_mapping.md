# Memory & Mapping

这一节覆盖 `shm-lib` 的共享内存 allocator 初始化与映射参数语义（`shm/mm.h`），用于指导你在自己的系统里完成“可运行 + 可复现”的接入。

## 核心接口

`shm-lib/include/shm/mm.h`：

- `init_cacheable_allocator(shm_path, base, size)`
- `init_cxl_cacheable_allocator(shm_path, base, size)`
- `cacheable.malloc/free/clalign/posix_memalign`

## 初始化顺序（建议）

1. （可选）`initialize_shm_related(machine_no)`（如果你的工程需要 queue/comm 组件）
2. 选择后端并初始化 `cacheable`：
   - 本地 mmap 文件 / DAX：`init_cacheable_allocator(...)`
   - CXL：`init_cxl_cacheable_allocator(...)`
3. 之后才允许任何 `cacheable.*` 分配与基于 `CXLAllocator` 的容器构造

## 集成 Checklist（你在工程里需要做的事）

- 在进程启动早期完成 `init_cacheable_allocator(...)` / `init_cxl_cacheable_allocator(...)`
- 明确 `shm_path/base/size` 的来源（配置文件/命令行/环境变量），并在日志里打印最终值，便于复现
- 确保所有“要落在 shm 上”的对象构造发生在初始化之后（尤其是全局对象/单例）

## 工程侧最小 include（常用三件套）

- `shm/mm.h`：初始化 allocator + `cacheable`
- `shm/cxl_type.h`：`cxl_vector/cxl_string`
- `utils/helper.h`：`ALLOC_AND_CONSTRUCT` / `DESTROY_AND_DEALLOC` / `NEW_CLASS_ON_SHM`

## 约束与排错建议

- 如果你传了固定 `base` 但映射失败，优先检查：
  - 地址是否与 ASLR/已有映射冲突
  - `size` 是否过大或对齐要求不满足（例如 DAX 场景）
- 建议在程序启动日志里打印最终的 `shm_path/base/size`，把“实验可复现”当作默认要求。

## 真实用法：YCSB-C `PrepareShmEnv()`

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: void PrepareShmEnv
:end-before: auto build_dbs
```

## 参数约束要点

- `shm_path`：后端路径（mmap 文件/设备路径），语义由 `SystemMemoryMmapper` 实现决定。
- `base`：期望映射基址；传 `nullptr` 代表让 OS 选择。固定基址可减少跨进程/跨机指针传递的复杂度，但也更容易映射失败。
- `size`：字节数；在 YCSB-C 的 ini 里常用 MB 配置再换算。
