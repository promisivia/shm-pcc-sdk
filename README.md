# SHM-PCC-SDK for CXL/UB

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org/)

## 📖 Project Overview

SHM-PCC-SDK is a comprehensive software development kit designed for CXL (Compute Express Link) and Unified Buffer (UB) memory systems. It provides a shared memory programming model and performance counter collection capabilities, enabling efficient development and evaluation of CXL-based applications.

### Core Features

- 🔧 **Multiple Data Structures**: Includes BwTree, Masstree, CLHT, ClevelHash, HOT, RadixART, and many other high-performance concurrent data structures
- 🚀 **High-Performance Memory Management**: Provides log-structured memory allocator (lsmalloc) and CXL memory management
- 🔒 **Concurrency Control**: Supports multiple concurrency control mechanisms including Optimistic Concurrency Control (OCC) and Software Transactional Memory (STM)
- 📊 **Performance Evaluation Tools**: Integrated YCSB-C benchmark and STAMP test suite
- 🛠️ **Applications**: Provides multiple applications (STAMP, etc.)

## 🏗️ Project Structure

```
shm-pcc-sdk/
├── apps/                   # Application Directory
│   ├── stamp/              # STAMP Benchmark Suite
├── ds/                     # Data Structure Implementations
│   ├── BwTree/             # BwTree Implementation
│   ├── Masstree/           # Masstree Implementation
│   ├── CLHT/               # CLHT Hash Table
│   ├── ClevelHash/         # ClevelHash Implementation
│   ├── HOT/                # HOT (Height Optimized Trie)
│   ├── RadixART/           # RadixART Implementation
│   └── ...                 # More Data Structures
├── malloc/                 # Memory Allocators
│   ├── lsmalloc/           # Log-structured Memory Allocator
│   └── cxl-shm/            # CXL Shared Memory Allocator
├── shm-lib/                # Shared Memory Library
│   ├── include/            # Public Headers
│   ├── shm/                # Shared Memory Management
│   ├── msg/                # Message Queues
│   └── utils/              # Utility Functions
├── stm/                    # Software Transactional Memory
│   ├── tl2/                # TL2 STM Implementation
│   ├── swisstm/            # SwissTM Implementation
│   └── tinystm/            # TinySTM Implementation
├── tests/                  # Test Suite
│   ├── YCSB-C/             # YCSB-C Benchmark
│   ├── basic/              # Basic Tests
│   ├── correctness/        # Correctness Tests
│   ├── allocator/          # MemoryAllocator Tests
└── docs/                   # Documentation Directory
    ├── USER_GUIDE.md       # User Guide
    ├── DEVELOPER_GUIDE.md  # Developer Guide
    └── API_REFERENCE.md    # API Reference
```

## 🚀 Quick Start

### System Requirements

- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: GCC 7+ or Clang 10+ (C++17 support)
- **CMake**: Version 3.10 or higher
- **Dependencies**:
  - libnuma-dev
  - libmemkind-dev
  - libtbb-dev
  - libssh-dev

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
git clone <repository-url>
cd shm-pcc-sdk

# Build core library
cd shm-lib
mkdir build && cd build
cmake ..
make -j$(nproc)

# Return to project root
cd ../..
```

### Run Examples

#### 1. YCSB-C Benchmark

```bash
cd tests/YCSB-C

# Build (with concurrency control)
./build.sh cc

# Or build (without concurrency control)
./build.sh nocc

# Run test
./run_shm_ds.sh -db=bwtree -mode=test
```

**Supported Database Types:**
- `bwtree` - BwTree
- `masstree` - Masstree
- `clht` - CLHT Hash Table
- `clevelhash` - ClevelHash
- `hot` - HOT
- `btree_olc` - BTree-OLC
- `radix_art_olc` - RadixART-OLC

#### 2. STAMP Benchmark

```bash
cd apps/stamp
./build.sh
./run.sh -a <app> -t <threads>
```

**Supported Applications:**
- `bayes` - Bayesian Network
- `genome` - Genome Sequencing
- `intruder` - Intrusion Detection
- `kmeans` - K-Means Clustering
- `labyrinth` - Labyrinth Solver
- `ssca2` - SSCA2 Graph Analysis
- `vacation` - Vacation Reservation
- `yada` - Yada R-Naught

#### 3. Log-structured Memory Allocator Test

```bash
cd malloc/lsmalloc
./build.sh
# Run tests (check tests/ directory for test executables)
```

## 📚 Documentation

- [User Guide](docs/USER_GUIDE.md) - Detailed installation, configuration, and usage instructions
- [Developer Guide](docs/DEVELOPER_GUIDE.md) - Code structure, build system, and contribution guidelines

## 🔬 Testing

### Run Test Suite

```bash
# Basic tests
cd tests/basic
./run_tests.sh

# Correctness tests
cd tests/correctness
./build_and_test.sh

# YCSB-C tests
cd tests/YCSB-C
./build.sh cc
./run_shm_ds.sh -db=bwtree -mode=test
```

## 🛠️ Configuration Options

### Build Configuration

YCSB-C supports multiple build configurations:

- `cc` - With concurrency control
- `nocc` - Without concurrency control
- `cc_mq` - Concurrency control + message queue
- `limit_atomic` - Use cxl_std::atomic
- `lat_cc` - Latency test (with concurrency control)
- `lat_nocc` - Latency test (without concurrency control)

### Runtime Configuration

Configure runtime parameters via `config.ini`:

```ini
[server]
threads = 144
db_num = 1

[client]
threads = 48

[workload]
recordcount = 1000000
operationcount = 10000000
```

## 📊 Performance Benchmarks

The project includes multiple performance benchmarks:

- **YCSB-C**: Key-value store performance tests
- **STAMP**: Transactional memory performance tests
- **Custom Benchmarks**: Performance tests for specific data structures

## 🤝 Contributing

We welcome community contributions! Please see [Contributing Guide](docs/CONTRIBUTING.md) for details.

### Contribution Process

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the [MIT License](LICENSE).

## 🙏 Acknowledgments

This project uses the following excellent open-source projects:

- [BwTree](https://github.com/wangziqi2013/BwTree)
- [Masstree](https://github.com/kohler/masstree-beta)
- [CLHT](https://github.com/LPD-EPFL/CLHT)
- [HOT](https://github.com/speedskater/hot)
- [YCSB](https://github.com/brianfrankcooper/YCSB)

## 📧 Contact

- **Issues**: [GitHub Issues](https://github.com/your-org/shm-pcc-sdk/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-org/shm-pcc-sdk/discussions)

## 🔗 Related Links

- [CXL Specification](https://www.computeexpresslink.org/)
- [Project Wiki](https://github.com/your-org/shm-pcc-sdk/wiki)
- [Changelog](CHANGELOG.md)

---

**Note**: This project is under active development. APIs may change. Please test thoroughly before using in production.
