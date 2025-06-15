#!/bin/bash

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的信息
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# 检查命令是否存在
check_command() {
    if ! command -v $1 &> /dev/null; then
        error "$1 is required but not installed"
    fi
}

# 检查必要的命令
check_command cmake
check_command make

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
STM_DIR="$(dirname "$SCRIPT_DIR")/../stm"
TL2_DIR="$STM_DIR/tl2"

# 默认STM实现
STM_IMPL=${1:-"tl2"}

info "Building STM implementation: $STM_IMPL"

# 编译TL2
info "Building TL2..."
cd "$TL2_DIR" || error "Failed to change directory to $TL2_DIR"

# 创建并进入build目录
mkdir -p build
cd build || error "Failed to change directory to build"

# 配置和编译TL2
cmake -DCMAKE_INSTALL_PREFIX=./stm .. || error "CMake configuration failed"
make || error "Make failed"
make install || error "Make install failed"

info "TL2 build completed successfully"

# 编译STAMP
info "Building STAMP benchmarks..."
cd "$SCRIPT_DIR" || error "Failed to change directory to $SCRIPT_DIR"

# 创建并进入build目录
mkdir -p build
cd build || error "Failed to change directory to build"

# 配置和编译STAMP
cmake -DSTM_IMPL=$STM_IMPL .. || error "CMake configuration failed"
make || error "Make failed"

info "STAMP build completed successfully"
info "Build artifacts are in: $SCRIPT_DIR/build"
info "Run with: ./run.sh <app>"