#!/bin/bash

# BwTree Linearizability Test Demo Script
# 演示完整的测试流程

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_header() {
    echo -e "${PURPLE}========================================${NC}"
    echo -e "${PURPLE}$1${NC}"
    echo -e "${PURPLE}========================================${NC}"
}

print_step() {
    echo -e "${CYAN}📋 $1${NC}"
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

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# 检查是否在正确的目录
check_directory() {
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "This script must be run from the tests/basic directory!"
        print_info "Please run: cd tests/basic && ./demo.sh"
        exit 1
    fi
}

# 演示步骤1: 快速功能测试
demo_quick_test() {
    print_header "Step 1: Quick BwTree Functionality Test"
    
    print_step "Building quick test..."
    if [ ! -f "./quick_bwtree_test" ]; then
        print_info "Building quick test executable..."
        mkdir -p build
        cd build
        cmake .. > /dev/null 2>&1
        make quick_bwtree_test > /dev/null 2>&1
        cd ..
    fi
    
    if [ -f "./quick_bwtree_test" ]; then
        print_success "Quick test executable ready"
        print_step "Running quick functionality test..."
        ./quick_bwtree_test
        print_success "Quick functionality test completed"
    else
        print_error "Failed to build quick test"
        return 1
    fi
    
    echo ""
}

# 演示步骤2: 构建线性一致性测试
demo_build_test() {
    print_header "Step 2: Build Linearizability Test"
    
    print_step "Building linearizability test..."
    if [ ! -f "./bwtree_linearizability_test" ]; then
        print_info "Building linearizability test executable..."
        mkdir -p build
        cd build
        cmake .. > /dev/null 2>&1
        make bwtree_linearizability_test > /dev/null 2>&1
        cd ..
    fi
    
    if [ -f "./bwtree_linearizability_test" ]; then
        print_success "Linearizability test executable ready"
    else
        print_error "Failed to build linearizability test"
        return 1
    fi
    
    echo ""
}

# 演示步骤3: 小规模测试
demo_small_test() {
    print_header "Step 3: Small Scale Linearizability Test"
    
    print_step "Running small scale test (4 threads, 100 ops each)..."
    print_info "This is a quick test to verify basic functionality"
    
    ./bwtree_linearizability_test 4 100 10000 42
    
    print_success "Small scale test completed"
    echo ""
}

# 演示步骤4: 中等规模测试
demo_medium_test() {
    print_header "Step 4: Medium Scale Linearizability Test"
    
    print_step "Running medium scale test (8 threads, 500 ops each)..."
    print_info "This test provides more thorough coverage"
    
    ./bwtree_linearizability_test 8 500 50000 123
    
    print_success "Medium scale test completed"
    echo ""
}

# 演示步骤5: 使用自动化脚本
demo_automation() {
    print_header "Step 5: Using Automation Scripts"
    
    print_step "Demonstrating build_and_test.sh script..."
    print_info "Showing help information:"
    ./build_and_test.sh help
    
    echo ""
    print_step "Demonstrating clean functionality..."
    ./build_and_test.sh clean
    
    echo ""
    print_step "Demonstrating build functionality..."
    ./build_and_test.sh build
    
    echo ""
    print_step "Demonstrating test functionality..."
    ./build_and_test.sh test 4 200 20000 456
    
    print_success "Automation script demonstration completed"
    echo ""
}

# 演示步骤6: 性能测试建议
demo_performance_tips() {
    print_header "Step 6: Performance Testing Tips"
    
    print_info "For performance testing, consider these parameters:"
    echo ""
    echo "  🚀 Light test (quick verification):"
    echo "     ./bwtree_linearizability_test 4 100 10000 42"
    echo ""
    echo "  🔍 Medium test (thorough coverage):"
    echo "     ./bwtree_linearizability_test 8 500 50000 123"
    echo ""
    echo "  💪 Heavy test (stress testing):"
    echo "     ./bwtree_linearizability_test 16 1000 100000 789"
    echo ""
    echo "  🧪 Extreme test (boundary testing):"
    echo "     ./bwtree_linearizability_test 32 2000 1000000 999"
    echo ""
    
    print_warning "Note: Larger tests require more memory and CPU time"
    print_info "Monitor system resources during heavy tests"
    
    echo ""
}

# 演示步骤7: 故障排除演示
demo_troubleshooting() {
    print_header "Step 7: Troubleshooting Demo"
    
    print_step "Common issues and solutions:"
    echo ""
    echo "  🔧 Build issues:"
    echo "     - Check C++20 support: g++ --version"
    echo "     - Verify dependencies: ldd ./bwtree_linearizability_test"
    echo "     - Clean and rebuild: ./build_and_test.sh clean && ./build_and_test.sh build"
    echo ""
    echo "  🚨 Runtime issues:"
    echo "     - Check system resources: free -h, nproc"
    echo "     - Reduce test parameters for initial testing"
    echo "     - Use fixed seeds for reproducible results"
    echo ""
    echo "  📊 Linearizability violations:"
    echo "     - Analyze failed operation sequences"
    echo "     - Check BwTree implementation"
    echo "     - Consider increasing operation counts"
    echo ""
    
    print_success "Troubleshooting guide presented"
    echo ""
}

# 主演示函数
main_demo() {
    print_header "BwTree Linearizability Test - Complete Demo"
    
    print_info "This demo will walk you through the complete testing process"
    print_info "Press Enter to continue between steps..."
    echo ""
    
    # 检查目录
    check_directory
    
    # 执行演示步骤
    demo_quick_test
    read -p "Press Enter to continue to Step 2..."
    
    demo_build_test
    read -p "Press Enter to continue to Step 3..."
    
    demo_small_test
    read -p "Press Enter to continue to Step 4..."
    
    demo_medium_test
    read -p "Press Enter to continue to Step 5..."
    
    demo_automation
    read -p "Press Enter to continue to Step 6..."
    
    demo_performance_tips
    read -p "Press Enter to continue to Step 7..."
    
    demo_troubleshooting
    
    print_header "Demo Completed Successfully!"
    print_success "You now have a complete understanding of the BwTree linearizability test framework"
    print_info "Check the README files for detailed usage instructions"
    print_info "Happy testing! 🎉"
}

# 运行主演示
main_demo
