# Apps（YCSB-C / STAMP / Demos）

本仓库包含两类“可运行入口”：

- `tests/YCSB-C/`：最完整的集成样例与基准（推荐先从这里开始）
- `apps/**` 与 `demos/`：按场景推进的应用/第三方集成与小型 demo

## 1. YCSB-C（推荐主入口）

### 构建

YCSB-C 的构建脚本支持用不同 variant 生成不同可执行文件，并把 `./ycsbc` 软链到最近一次构建产物：

```{literalinclude} ../../../../tests/YCSB-C/build.sh
:language: bash
```

### 运行

运行脚本封装了常用参数（`-db`、线程数、机器数、INI 配置等），适合作为“可复现基准入口”：

```{literalinclude} ../../../../tests/YCSB-C/run_shm_ds.sh
:language: bash
:start-after: run_ycsbc() {
:end-before: run_real() {
```

更多细节（workload/spec、trace、不同模式）建议直接阅读 `tests/YCSB-C/run_shm_ds.sh` 与 `tests/YCSB-C/ycsbc.cc`。

## 2. STAMP

`apps/stamp/` 是 STAMP 基准套件的集成目录，通常包含独立的构建与运行脚本：

- `apps/stamp/build.sh`
- `apps/stamp/run.sh`
- `apps/stamp/README.md`

建议把它当作“如何把更大体量的应用迁移到仓库构建体系/依赖体系里”的参考。

## 3. 其它应用与 demos

- `apps/**`：包含若干系统/项目的集成（例如图计算、消息中间件、存储系统等），每个子目录的 README 是最权威入口
- `demos/`：更小粒度的 demo，适合快速验证 allocator/容器/消息队列等组件能否跑通

```{note}
如果你准备把自己的服务接入共享内存/CXL，建议先用 YCSB-C 把“分配器初始化 + 数据结构选择 + 并发/通信路径”跑通，再迁移到你的应用目录。
```

