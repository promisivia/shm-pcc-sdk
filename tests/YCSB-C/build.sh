# #!/bin/sh
set -e

for variant in "$@"; do
    build_dir="build_$variant"
    mkdir -p $build_dir || { echo "Failed to create directory $build_dir"; exit 1; }
    cmake -B $build_dir -DVARIANT=$variant || { echo "CMake configuration failed for $variant"; exit 1; }
    make -j -C $build_dir || { echo "Make failed for $variant"; exit 1; }
    ln -sf $build_dir/ycsbc_$variant ycsbc || { echo "Failed to create symlink for $variant"; exit 1; }
    ln -sf $build_dir build || { echo "Failed to create symlink for build_$variant"; exit 1; }
done
