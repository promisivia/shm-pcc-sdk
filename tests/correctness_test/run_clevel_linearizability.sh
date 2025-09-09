#!/bin/bash

# Clevel-Hashing Linearizability Test Runner
# 用法: ./run_clevel_linearizability.sh [threads] [ops_per_thread] [key_space] [seed] [rehash_test]

# 默认参数
THREADS=${1:-4}
OPS_PER_THREAD=${2:-100}
KEY_SPACE=${3:-10000}
SEED=${4:-42}
REHASH_TEST=${5:-false}

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志目录
LOG_DIR="./logs/clevel_linearizability"
mkdir -p $LOG_DIR

echo -e "${BLUE}🔍 Clevel-Hashing Linearizability Test${NC}"
echo "=========================================="
echo -e "${YELLOW}Parameters:${NC}"
echo "  Threads: $THREADS"
echo "  Ops per thread: $OPS_PER_THREAD"
echo "  Key space: $KEY_SPACE"
echo "  Seed: $SEED"
echo "  Rehash test: $REHASH_TEST"
echo ""

# 检查可执行文件是否存在
if [ ! -f "./clevel_linearizability_test" ]; then
    echo -e "${RED}❌ Error: clevel_linearizability_test executable not found!${NC}"
    echo "Please build the project first using:"
    echo "  mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# 运行测试
echo -e "${BLUE}🧪 Running Clevel-Hashing linearizability test...${NC}"
echo ""

# 构建测试命令
TEST_CMD="./clevel_linearizability_test $THREADS $OPS_PER_THREAD $KEY_SPACE $SEED"
if [ "$REHASH_TEST" = "true" ]; then
    TEST_CMD="$TEST_CMD rehash"
fi

# 运行测试并记录输出
echo "Command: $TEST_CMD"
echo ""

# 记录开始时间
START_TIME=$(date +%s)

# 运行测试
if $TEST_CMD 2>&1 | tee "$LOG_DIR/clevel_test_$(date +%Y%m%d_%H%M%S).log"; then
    echo ""
    echo -e "${GREEN}✅ Clevel-Hashing linearizability test completed successfully!${NC}"
    EXIT_CODE=0
else
    echo ""
    echo -e "${RED}❌ Clevel-Hashing linearizability test failed!${NC}"
    EXIT_CODE=1
fi

# 记录结束时间
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo ""
echo -e "${BLUE}📊 Test Summary:${NC}"
echo "  Duration: ${DURATION}s"
echo "  Log file: $LOG_DIR/"
echo ""

# 显示最近的日志文件
echo -e "${YELLOW}Recent log files:${NC}"
ls -la "$LOG_DIR"/*.log 2>/dev/null | tail -3 | while read line; do
    echo "  $line"
done

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
else
    echo -e "${RED}💥 Some tests failed. Check the logs above for details.${NC}"
fi

exit $EXIT_CODE

