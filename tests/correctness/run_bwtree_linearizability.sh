#!/bin/bash

# BwTree Linearizability Test Runner
# 用法: ./run_bwtree_linearizability.sh [threads] [ops_per_thread] [key_space] [seed]

set -e

# 默认参数
THREADS=${1:-8}
OPS_PER_THREAD=${2:-1000}
KEY_SPACE=${3:-100000}
SEED=${4:-42}

echo "🚀 Starting BwTree Linearizability Test"
echo "========================================"
echo "Threads: $THREADS"
echo "Operations per thread: $OPS_PER_THREAD"
echo "Key space size: $KEY_SPACE"
echo "Random seed: $SEED"
echo ""

# 检查可执行文件是否存在
if [ ! -f "./bwtree_linearizability_test" ]; then
    echo "❌ Error: bwtree_linearizability_test executable not found!"
    echo "Please build the test first using:"
    echo "  cd tests/basic && mkdir -p build && cd build"
    echo "  cmake .. && make"
    echo ""
    exit 1
fi

# 创建日志目录
LOG_DIR="./logs"
mkdir -p $LOG_DIR

# 生成时间戳
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOG_DIR/bwtree_linearizability_${THREADS}t_${OPS_PER_THREAD}ops_${KEY_SPACE}ks_${TIMESTAMP}.log"

echo "📝 Log file: $LOG_FILE"
echo ""

# 运行测试
echo "⏳ Running test..."
echo "Command: ./bwtree_linearizability_test $THREADS $OPS_PER_THREAD $KEY_SPACE $SEED"
echo ""

# 运行测试并记录输出
./bwtree_linearizability_test $THREADS $OPS_PER_THREAD $KEY_SPACE $SEED 2>&1 | tee $LOG_FILE

# 检查退出码
EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "========================================"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Test completed successfully!"
    echo "🎉 BwTree linearizability check PASSED"
else
    echo "❌ Test failed with exit code $EXIT_CODE"
    echo "💥 BwTree linearizability check FAILED"
fi

echo "📊 Results saved to: $LOG_FILE"
echo ""

# 显示日志文件的最后几行
echo "📋 Last few lines of the log:"
echo "----------------------------------------"
tail -10 $LOG_FILE

exit $EXIT_CODE
