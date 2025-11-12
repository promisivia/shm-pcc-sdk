#!/bin/bash
set -x

# 默认配置
IMAGE="shm-pcc-sdk-ycsb:latest"
CONTAINER="ycsb-build-$(date +%s)"

# 帮助信息
show_help() {
    echo "用法: $0 [选项] [变体...]"
    echo "选项: -h(帮助) -i(镜像) -c(容器名) -d(后台) -r(自动删除)"
    echo "变体: NOCC, CC 等，不指定则启动交互式容器"
    echo "示例: $0 NOCC CC 或 $0 -d -r NOCC"
}

# 解析参数
DETACH=false
RM=false
BUILD_ARGS=""
BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help) show_help; exit 0 ;;
        -i) IMAGE="$2"; shift 2 ;;
        -c) CONTAINER="$2"; shift 2 ;;
        -d) DETACH=true; shift ;;
        -r) RM=true; shift ;;
        -b) BUILD=true; shift ;;
        -*) echo "未知选项 $1"; show_help; exit 1 ;;
        *) BUILD_ARGS="$BUILD_ARGS $1"; shift ;;
    esac
done

# 构建镜像（如果不存在）或者指定要构建 arg=build
if ! docker images | grep -q "$(echo $IMAGE | cut -d: -f1)" || [[ "$BUILD" == true ]]; then
    echo "构建镜像 $IMAGE..."
    docker build --network=host --build-arg MEMKIND_FROM_SOURCE=1 -t $IMAGE -f Dockerfile .
fi

# 准备运行命令
ROOT_DIR=$(pwd)/../..
docker run --rm -it \
    -v "$(pwd)/../..":/workspace \
    -w /workspace/tests/YCSB-C \
    $IMAGE \
    bash -c "chmod +x ./build.sh && ./build.sh $BUILD_ARGS"
echo "构建完成！检查 $(pwd) 目录下的构建结果"

# 显示状态
echo "容器状态:"
docker ps -a | grep $CONTAINER || echo "容器已删除"
echo "完成！"