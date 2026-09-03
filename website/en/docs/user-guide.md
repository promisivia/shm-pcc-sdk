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

[Add configuration details here]

## Usage Examples

[Add usage examples here]

## Performance Tuning

[Add performance tuning tips here]

## Troubleshooting

[Add troubleshooting information here]

## FAQ

[Add frequently asked questions here]


