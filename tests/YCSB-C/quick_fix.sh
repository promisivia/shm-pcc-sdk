#!/bin/bash

echo -e "\033[32m快速修复脚本 - 解决sudo和cityhash问题\033[0m"

# 检查是否在Docker容器中
if [ -f /.dockerenv ]; then
    echo -e "\033[34m检测到Docker环境\033[0m"
    
    # 安装sudo（如果不存在）
    if ! command -v sudo &> /dev/null; then
        echo -e "\033[33m安装sudo...\033[0m"
        dnf install -y sudo
    fi
    
    # 配置sudo权限
    echo "developer ALL=(ALL) NOPASSWD:ALL" | sudo tee -a /etc/sudoers
    
    # 安装cityhash
    if ! dnf list installed cityhash-devel &> /dev/null; then
        echo -e "\033[33m安装cityhash...\033[0m"
        cd /tmp
        git clone https://github.com/google/cityhash.git
        cd cityhash
        ./configure --prefix=/usr
        make -j$(nproc)
        sudo make install
        sudo ldconfig
        cd /workspace/tests/YCSB-C
    fi
    
    # 设置库路径
    export LD_LIBRARY_PATH="/usr/lib:/usr/local/lib:$LD_LIBRARY_PATH"
    
    echo -e "\033[32m修复完成！现在可以运行测试了\033[0m"
    echo -e "\033[34m运行: ./huawei_test.sh\033[0m"
else
    echo -e "\033[31m此脚本需要在Docker容器中运行\033[0m"
    echo -e "\033[34m请先运行: ./docker_run_build.sh\033[0m"
fi
