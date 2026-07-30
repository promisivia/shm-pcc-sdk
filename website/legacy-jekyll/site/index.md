---
layout: default
title: Home
nav_order: 1
description: "SHM-PCC-SDK - A comprehensive software development kit for CXL/UB shared memory programming"
lang: zh
permalink: /
nav_exclude: false
---

# SHM-PCC-SDK Documentation

欢迎来到 SHM-PCC-SDK 文档中心！这是一个为 CXL (Compute Express Link) 和统一缓冲区 (UB) 内存系统设计的综合软件开发工具包。

{% include language-switcher.html %}

## 🚀 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/promisivia/shm-pcc-sdk.git
cd shm-pcc-sdk

# 构建核心库
cd shm-lib
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行示例

```bash
# YCSB-C 基准测试
cd tests/YCSB-C
./build.sh cc
./run_shm_ds.sh -db=bwtree -mode=test
```

## 📚 文档导航

### 用户文档

- **[用户指南]({{ site.baseurl }}/zh/docs/user-guide.html)** - 安装、配置和使用指南
- **[环境变量]({{ site.baseurl }}/zh/docs/environment-variables.html)** - 环境变量配置说明

### 开发者文档

- **[开发者指南]({{ site.baseurl }}/zh/docs/developer-guide.html)** - 代码结构、构建系统和开发指南
- **[贡献指南]({{ site.baseurl }}/zh/docs/contributing.html)** - 如何为项目做贡献

### 项目信息

- **[路线图]({{ site.baseurl }}/zh/docs/roadmap.html)** - 项目发展路线图
- **[开源检查清单]({{ site.baseurl }}/zh/docs/open-source-checklist.html)** - 开源准备检查清单
- **[代码审查与建议]({{ site.baseurl }}/zh/docs/code-review.html)** - 代码审查和改进建议

## 🏗️ 核心特性

- 🔧 **多种数据结构**: BwTree, Masstree, CLHT, ClevelHash, HOT, RadixART 等高性能并发数据结构
- 🚀 **高性能内存管理**: 提供日志结构内存分配器 (lsmalloc) 和 CXL 内存管理
- 🔒 **并发控制**: 支持多种并发控制机制，包括乐观并发控制 (OCC) 和软件事务内存 (STM)
- 📊 **性能评估工具**: 集成的 YCSB-C 基准测试和 STAMP 测试套件

## 🤝 贡献

我们欢迎社区贡献！请查看 [贡献指南]({{ site.baseurl }}/zh/docs/contributing.html) 了解详情。

## 📝 许可证

本项目采用 [MIT License](https://github.com/promisivia/shm-pcc-sdk/blob/main/LICENSE)。

## 🔗 相关链接

- [GitHub 仓库](https://github.com/promisivia/shm-pcc-sdk)
- [问题追踪](https://github.com/promisivia/shm-pcc-sdk/issues)
- [CXL 规范](https://www.computeexpresslink.org/)

---

**最后更新**: 2024
