---
layout: default
title: Home
nav_order: 1
description: "CXL-SDK - A comprehensive software development kit for CXL/UB shared memory programming"
lang: en
permalink: /en/
nav_exclude: true
---

# CXL-SDK Documentation

Welcome to the CXL-SDK documentation center! This is a comprehensive software development kit designed for CXL (Compute Express Link) and Unified Buffer (UB) memory systems.

{% include language-switcher.html %}

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/promisivia/shm-pcc-sdk.git
cd shm-pcc-sdk

# Build core library
cd shm-lib
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run Examples

```bash
# YCSB-C Benchmark
cd tests/YCSB-C
./build.sh cc
./run_shm_ds.sh -db=bwtree -mode=test
```

## 📚 Documentation Navigation

### User Documentation

- **[User Guide]({{ site.baseurl }}/en/docs/user-guide.html)** - Installation, configuration, and usage guide
- **[Environment Variables]({{ site.baseurl }}/en/docs/environment-variables.html)** - Environment variables configuration

### Developer Documentation

- **[Developer Guide]({{ site.baseurl }}/en/docs/developer-guide.html)** - Code structure, build system, and development guide
- **[Contributing Guide]({{ site.baseurl }}/en/docs/contributing.html)** - How to contribute to the project

### Project Information

- **[Roadmap]({{ site.baseurl }}/en/docs/roadmap.html)** - Project development roadmap
- **[Open Source Checklist]({{ site.baseurl }}/en/docs/open-source-checklist.html)** - Open source preparation checklist
- **[Code Review and Recommendations]({{ site.baseurl }}/en/docs/code-review.html)** - Code review and improvement recommendations

## 🏗️ Core Features

- 🔧 **Multiple Data Structures**: BwTree, Masstree, CLHT, ClevelHash, HOT, RadixART and other high-performance concurrent data structures
- 🚀 **High-Performance Memory Management**: Provides log-structured memory allocator (lsmalloc) and CXL memory management
- 🔒 **Concurrency Control**: Supports multiple concurrency control mechanisms including Optimistic Concurrency Control (OCC) and Software Transactional Memory (STM)
- 📊 **Performance Evaluation Tools**: Integrated YCSB-C benchmark and STAMP test suite

## 🤝 Contributing

We welcome community contributions! See the [Contributing Guide]({{ site.baseurl }}/en/docs/contributing.html) for details.

## 📝 License

This project is licensed under the [MIT License](https://github.com/promisivia/shm-pcc-sdk/blob/main/LICENSE).

## 🔗 Related Links

- [GitHub Repository](https://github.com/promisivia/shm-pcc-sdk)
- [Issue Tracker](https://github.com/promisivia/shm-pcc-sdk/issues)
- [CXL Specification](https://www.computeexpresslink.org/)

---

**Last Updated**: 2024

