#!/bin/bash

# BwTree Linearizability Test - Build and Test Script
# 用法: ./build_and_test.sh [action] [threads] [ops_per_thread] [key_space] [seed]

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认参数
ACTION=${1:-"all"}
THREADS=${2:-8}
OPS_PER_THREAD=${3:-1000}
KEY_SPACE=${4:-100000}
SEED=${5:-42}

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# 显示帮助信息
show_help() {
    echo "BwTree Linearizability Test - Build and Test Script"
    echo ""
    echo "用法: $0 [action] [threads] [ops_per_thread] [key_space] [seed]"
    echo ""
    echo "Actions:"
    echo "  build     - 仅构建测试"
    echo "  test      - 仅运行测试（需要先构建）"
    echo "  all       - 构建并运行测试（默认）"
    echo "  clean     - 清理构建文件"
    echo "  help      - 显示此帮助信息"
    echo ""
    echo "参数:"
    echo "  threads        - 并发线程数 (默认: 8)"
    echo "  ops_per_thread - 每个线程的操作数 (默认: 1000)"
    echo "  key_space      - 键空间大小 (默认: 100000)"
    echo "  seed           - 随机数种子 (默认: 42)"
    echo ""
    echo "示例:"
    echo "  $0                    # 使用默认参数构建并测试"
    echo "  $0 build             # 仅构建"
    echo "  $0 test 16 500 50000 # 使用自定义参数测试"
    echo "  $0 clean             # 清理构建文件"
}

# 构建测试
build_test() {
    print_info "Building BwTree linearizability test..."
    
    # 创建构建目录
    mkdir -p build
    cd build
    
    # 运行CMake
    print_info "Running CMake..."
    cmake .. || {
        print_error "CMake failed!"
        exit 1
    }
    
    # 编译
    print_info "Compiling..."
    make -j$(nproc) || {
        print_error "Compilation failed!"
        exit 1
    }
    
    cd ..
    
    # 检查可执行文件
    if [ -f "./bwtree_linearizability_test" ]; then
        print_success "Build completed successfully!"
        print_info "Executable: ./bwtree_linearizability_test"
    else
        print_error "Build failed - executable not found!"
        exit 1
    fi
}

# 运行测试
run_test() {
    print_info "Running BwTree linearizability test..."
    
    # 检查可执行文件
    if [ ! -f "./bwtree_linearizability_test" ]; then
        print_error "Executable not found! Please build first."
        exit 1
    fi
    
    # 运行测试
    print_info "Test parameters:"
    print_info "  Threads: $THREADS"
    print_info "  Operations per thread: $OPS_PER_THREAD"
    print_info "  Key space: $KEY_SPACE"
    print_info "  Seed: $SEED"
    echo ""
    
    # 使用运行脚本
    ./run_bwtree_linearizability.sh $THREADS $OPS_PER_THREAD $KEY_SPACE $SEED
}

# 清理构建文件
clean_build() {
    print_info "Cleaning build files..."
    
    # 删除构建目录
    if [ -d "build" ]; then
        rm -rf build
        print_success "Build directory removed"
    fi
    
    # 删除可执行文件
    if [ -f "bwtree_linearizability_test" ]; then
        rm -f bwtree_linearizability_test
        print_success "Executable removed"
    fi
    
    # 删除测试可执行文件
    if [ -f "test_basics" ]; then
        rm -f test_basics
        print_success "test_basics executable removed"
    fi
    
    print_success "Cleanup completed"
}

# 主函数
main() {
    case $ACTION in
        "build")
            build_test
            ;;
        "test")
            run_test
            ;;
        "clean")
            clean_build
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        "all")
            build_test
            echo ""
            run_test
            ;;
        *)
            print_error "Unknown action: $ACTION"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

# 检查是否在正确的目录
if [ ! -f "CMakeLists.txt" ]; then
    print_error "This script must be run from the tests/basic directory!"
    print_info "Please run: cd tests/basic && ./build_and_test.sh"
    exit 1
fi

# 运行主函数
main "$@"
