# Memory

## `shm/mm.h`

```{literalinclude} ../../../../shm-lib/include/shm/mm.h
:language: cpp
```

## 常用入口

- 初始化：`init_cacheable_allocator(...)` / `init_cxl_cacheable_allocator(...)`
- 分配：`cacheable.malloc(...)` / `cacheable.clalign(...)`
- 释放：`cacheable.free(...)`
