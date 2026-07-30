`ramcloud-cxl` 是在 RAMCloud 基础上做的 CXL 内存扩展版本，目标是在超节点环境里把远端/扩展内存纳入对象存储的数据面。

`./RAMCloud.md` 中记录了RAMCloud的如何支持分配在CXL上。

## TODO

### Share CXL pool

- 支持多个RAMCloud的实例从一个共享的CXL内存池分配内存
    - 原先每个实例跑在一个机器，只能分配本地的内存
    - 现在虽然还是每个实例跑在一个机器上，但多个实例共享一个内存池
- 测试：多个server进程mmap一块CXL内存，一起用