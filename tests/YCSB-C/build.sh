#!/usr/bin/env bash
set -e

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <variant> [variant ...]" >&2
    echo "Example: $0 cc nocc" >&2
    exit 2
fi

for variant in "$@"; do
    # normalize to lowercase (accept NOCC, nocc, etc.)
    lower_variant=$(echo "$variant" | tr '[:upper:]' '[:lower:]')
    build_dir="build_$lower_variant"
    mkdir -p "$build_dir" || { echo "Failed to create directory $build_dir"; exit 1; }
    cmake -S . -B "$build_dir" -DVARIANT="$lower_variant" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON || { echo "CMake configuration failed for $variant ($lower_variant)"; exit 1; }
    cmake --build "$build_dir" --parallel || { echo "Build failed for $variant"; exit 1; }
    ln -sf "$build_dir/ycsbc_$lower_variant" ycsbc || { echo "Failed to create symlink for $variant"; exit 1; }
    ln -sf "$build_dir" build || { echo "Failed to create symlink for build_$lower_variant"; exit 1; }
    ln -sf "$(pwd)/$build_dir/compile_commands.json" ../../compile_commands.json || { echo "Failed to create symlink for compile_commands.json for $variant"; exit 1; }
done
