# Containers（把 STL 容器搬到 shm）

`shm-lib/include/shm/cxl_type.h` 提供了基于 `CXLAllocator<T>` 的容器别名，核心目标是让容器内部动态分配也来自 `cacheable`。

## 接口

```{literalinclude} ../../../../shm-lib/include/shm/cxl_type.h
:language: cpp
```

## 真实用法：YCSB-C 构造 DB 列表

```{literalinclude} ../../../../tests/YCSB-C/ycsbc.cc
:language: cpp
:start-after: auto build_dbs
:end-before: auto build_workload
```

## 建议

- 如果你的对象里有动态成员（vector/string 等），优先使用 `cxl_vector` / `cxl_string` 作为承载，保证扩容内存域一致。
- 如果必须混用标准容器与 `cxl_*`，要显式规定“跨域数据的拷贝边界”（避免在 shm 中保存堆指针）。

## Do / Don't（面向系统集成）

- Do：把长生命周期数据结构（索引节点、mapping table、operation list 等）统一迁移到 `cacheable` 内存域。
- Do：对外暴露接口时，尽量避免直接返回“指向 shm 的裸指针”，优先定义清晰的 ownership。
- Don't：把 `std::string` 的 `c_str()` / `std::vector` 的内部指针保存到 shm 结构中（除非你确认其底层也在 shm 并且生命周期可控）。
