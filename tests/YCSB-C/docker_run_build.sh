#!/bin/bash
set -x

# Default configuration
IMAGE="shm-pcc-sdk-ycsb:latest"
CONTAINER="ycsb-build-$(date +%s)"

# Help information
show_help() {
    echo "Usage: $0 [options] [variants...]"
    echo "Options: -h(help) -i(image) -c(container) -d(detach) -r(auto-remove)"
    echo "Variants: NOCC, CC, etc. If not specified, start interactive container"
    echo "Example: $0 NOCC CC or $0 -d -r NOCC"
}

# Parse arguments
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
        -*) echo "Unknown option $1"; show_help; exit 1 ;;
        *) BUILD_ARGS="$BUILD_ARGS $1"; shift ;;
    esac
done

# Build image (if not exists) or specify to build arg=build
if ! docker images | grep -q "$(echo $IMAGE | cut -d: -f1)" || [[ "$BUILD" == true ]]; then
    echo "Building image $IMAGE..."
    docker build --network=host --build-arg MEMKIND_FROM_SOURCE=1 -t $IMAGE -f Dockerfile .
fi

# Prepare run command
ROOT_DIR=$(pwd)/../..
docker run --rm -it \
    -v "$(pwd)/../..":/workspace \
    -w /workspace/tests/YCSB-C \
    $IMAGE \
    bash -c "chmod +x ./build.sh && ./build.sh $BUILD_ARGS"
echo "Build complete! Check build results in $(pwd) directory"

# Show status
echo "Container status:"
docker ps -a | grep $CONTAINER || echo "Container removed"
echo "Done!"