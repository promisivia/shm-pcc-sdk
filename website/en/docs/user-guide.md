---
layout: default
title: User Guide
nav_order: 1
parent: User Documentation
description: Detailed usage instructions for CXL-SDK, including installation, configuration, running examples, and troubleshooting
lang: en
permalink: /en/docs/user-guide.html
---

{% include language-switcher.html %}

# User Guide

This document provides detailed usage instructions for CXL-SDK, including installation, configuration, running examples, and troubleshooting.

## Table of Contents

- [Installation Guide](#installation-guide)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

## Installation Guide

### System Requirements

#### Minimum Requirements

- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: GCC 7+ or Clang 10+ (C++17 support)
- **CMake**: Version 3.10 or higher
- **Memory**: 4GB RAM minimum, 8GB+ recommended
- **Storage**: 10GB free space

#### Recommended Requirements

- **CPU**: Multi-core processor (4+ cores)
- **Memory**: 16GB+ RAM
- **Storage**: SSD with 20GB+ free space

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libnuma-dev \
    libmemkind-dev \
    libtbb-dev \
    libssh-dev \
    git
```

**CentOS/RHEL:**
```bash
sudo yum install -y \
    gcc-c++ \
    cmake \
    numactl-devel \
    tbb-devel \
    libssh-devel \
    git
```

### Build the Project

```bash
# Clone the repository
git clone https://github.com/promisivia/shm-pcc-sdk.git
cd shm-pcc-sdk

# Build core library
cd shm-lib
mkdir build && cd build
cmake ..
make -j$(nproc)

# Return to project root
cd ../..
```

## Quick Start

### Run YCSB-C Benchmark

```bash
cd tests/YCSB-C

# Build (with concurrency control)
./build.sh cc

# Run test
./run_shm_ds.sh -db=bwtree -mode=test
```

## Configuration

YCSB-C reads shared-memory settings from
[`tests/YCSB-C/config.ini`](../../../tests/YCSB-C/config.ini). Set the backing
device or file, mapping address, region size, and allocator before running a
workload. Every process sharing the region must use compatible values.

## Usage Examples

The [`demos/`](../../../demos/README.md) guide contains minimal single-process
and multi-process examples for BwTree and ClevelHash. YCSB workloads and database
adapters are under [`tests/YCSB-C/`](../../../tests/YCSB-C/README.md).

## Performance Tuning

Use a release build for measurements, pin workers consistently, record NUMA and
CXL topology, warm up the workload, and repeat each run. Report both the complete
configuration and variance; do not compare results gathered with different
allocator, persistence, or concurrency-control settings.

## Troubleshooting

Start with the smallest demo and a file-backed region. Confirm that the backing
path is writable and large enough, all processes agree on the mapping address,
and required NUMA, memkind, TBB, and libssh development packages are installed.
The YCSB launcher may require privileges to change ASLR settings.

## FAQ

**Can I develop without CXL hardware?** Yes. Use a writable file under
`/dev/shm` as the backing region for functional development.

**Is the API stable?** Not yet. CXL-SDK is active research software; pin a commit
when reproducing experiments.


