# SHM-PCC-SDK for CXL

## Project Overview

SHM-PCC-SDK is a comprehensive software development kit designed for CXL (Compute Express Link) memory systems. It provides a shared memory programming model and performance counter collection capabilities, enabling efficient development and evaluation of CXL-based applications.

## Project Structure

```
shm-pcc-sdk/
├── apps/                   # Application Directory
│   └── stamp/              # STAMP Benchmark Suite
├── ds/                     # Data Structures
├── lib/                    # Core Libraries
├── malloc/                 # Memory Allocation
│   ├── lsmalloc/           # log-structured memory allocator
├── stm/                    # Software Transactional Memory
│   └── tl2/                # TL2 STM Implementation
├── tests/                  # Test Suite
│   ├── basic/              # Basic Tests
│   └── real-workloads/     # Real-world Tests
│   └── YCSB-C/             # YCSB-C
```

## Run Project

### Use YCSB-C to test data structures

```shell
cd tests/YCSB-C
./build.sh cc # or ./build.sh nocc
./run_shm_ds.sh
```

### Use STAMP to test STM

```shell
cd apps/stamp
./build.sh
# Run
./run.sh -a <app> -t <threads>
```

### Test Log-structured Memory Allocator

```shell
cd malloc/lsmalloc
./build.sh
./run.sh
```