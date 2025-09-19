#!/bin/bash

# 修复库路径和权限问题的脚本
echo -e "\033[32m正在修复库路径和权限问题...\033[0m"

# 设置库路径
export LD_LIBRARY_PATH="/usr/lib:/usr/local/lib:$LD_LIBRARY_PATH"

# 更新动态链接库缓存
sudo ldconfig

# 检查cityhash库是否存在
if [ -f "/usr/lib/libcityhash.so.0" ] || [ -f "/usr/local/lib/libcityhash.so.0" ]; then
    echo -e "\033[32m✓ cityhash库已找到\033[0m"
else
    echo -e "\033[31m✗ cityhash库未找到，正在安装...\033[0m"
    # 如果库不存在，尝试重新安装
    cd /tmp
    if [ ! -d "cityhash" ]; then
        git clone https://github.com/google/cityhash.git
    fi
    cd cityhash
    ./configure --prefix=/usr
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    cd /
fi

# 检查ycsbc可执行文件
if [ -f "./ycsbc" ]; then
    echo -e "\033[32m✓ ycsbc可执行文件存在\033[0m"
    # 设置可执行权限
    chmod +x ./ycsbc
else
    echo -e "\033[33m⚠ ycsbc可执行文件不存在，请先构建\033[0m"
fi

# 检查依赖库
echo -e "\033[34m检查依赖库...\033[0m"
ldd ./ycsbc 2>/dev/null || echo "ycsbc未构建或无法检查依赖"

echo -e "\033[32m修复完成！\033[0m"
