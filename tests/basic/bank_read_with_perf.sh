#!/bin/bash

# 获取脚本所在的目录
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PYTHON_SCRIPT="$SCRIPT_DIR/activate_perf.py"
PERF_PID=""

# 清理函数，用于停止 activate_perf.py
cleanup() {
  echo "正在尝试停止 activate_perf.py (PID: $PERF_PID)..."
  if [ -n "$PERF_PID" ] && ps -p "$PERF_PID" > /dev/null; then
    kill "$PERF_PID"
    # 等待 activate_perf.py 进程终止
    wait "$PERF_PID" 2>/dev/null
    echo "activate_perf.py (PID: $PERF_PID) 已停止。"
  else
    if [ -n "$PERF_PID" ]; then
      echo "activate_perf.py (PID: $PERF_PID) 似乎未在运行或已停止。"
    else
      echo "activate_perf.py PID 未知，可能未能成功启动。"
    fi
  fi
}

# 设置 trap，在脚本退出、中断或终止时调用 cleanup 函数
trap cleanup EXIT SIGINT SIGTERM

# 检查 activate_perf.py 脚本是否存在
if [ ! -f "$PYTHON_SCRIPT" ]; then
  echo "错误: 性能统计脚本 $PYTHON_SCRIPT 未找到!"
  exit 1
fi

echo "正在启动性能统计脚本: $PYTHON_SCRIPT ..."
# 以后台模式运行 python 脚本并获取其 PID
python3 "$PYTHON_SCRIPT" &
PERF_PID=$!

# 检查 python 脚本是否成功启动
if [ -z "$PERF_PID" ] || ! ps -p "$PERF_PID" > /dev/null; then
  echo "错误: 启动 $PYTHON_SCRIPT 失败。"
  # cleanup 函数将由 EXIT trap 调用
  exit 1
fi
echo "性能统计脚本 $PYTHON_SCRIPT 已启动，PID: $PERF_PID"

# OPERATION == PARA_READ
# ./test uncached /dev/uncached_mem 32 2>&1 | tee uncached.log
# ./test uncached-flush dev 32 2>&1 | tee read-uncached-flush.log
# ./test cached dev 32 2>&1 | tee read-cached.log

# OPERATION == PARA_READ
# ./test uncached-flush dev 32 2>&1 | tee write-uncached-flush.log
# ./test cached dev 32 2>&1 | tee write-cached.log

# ./prepare_env.sh
# ./test_basics cached dev 
# ./test_basics uncached /dev/uncached_mem_dev
# ./reset_env.sh

# ./test_basics cached /dev/uncached_mem_dev cas_miss
# ./test_basics cached /dev/uncached_mem_dev store_miss
# ./test_basics cached /dev/uncahced_mem_dev read_miss

echo "正在运行测试: ./test_basics uncached uncached_mem /dev/uncached_mem_dev para_read"
# 运行测试命令（移除末尾的 '&' 以便脚本等待其完成）
./test_basics uncached alloc_numa 0 para_write
# ./test_basics uncached uncached_mem /dev/uncached_mem_dev para_read
TEST_EXIT_CODE=$?
echo "测试完成，退出码: $TEST_EXIT_CODE"

# 测试完成后，脚本将退出，EXIT trap 会调用 cleanup 函数来停止 activate_perf.py
echo "测试已结束，将通过 trap 清理并停止性能统计脚本。"

