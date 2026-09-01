#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$root/third_party"

if [ ! -d "$root/third_party/eRPC" ]; then
	git clone --filter=blob:none --no-checkout https://github.com/erpc-io/eRPC.git \
		"$root/third_party/eRPC"
	git -C "$root/third_party/eRPC" fetch --depth 1 origin \
		de83dab3eab4a0fb19bfc4881c11d4a6b89ff17d
	git -C "$root/third_party/eRPC" checkout --detach FETCH_HEAD
fi
if git -C "$root/third_party/eRPC" apply --check "$root/patches/erpc-lrpc.patch" 2>/dev/null; then
	git -C "$root/third_party/eRPC" apply "$root/patches/erpc-lrpc.patch"
elif ! git -C "$root/third_party/eRPC" apply --reverse --check \
	"$root/patches/erpc-lrpc.patch" 2>/dev/null; then
	echo "eRPC tree is neither clean nor patched as expected" >&2
	exit 1
fi
if [ ! -d "$root/third_party/linux" ]; then
	tmp=${TMPDIR:-/tmp}/linux-6.6.155.tar.xz
	curl -fsSL https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.155.tar.xz -o "$tmp"
	tar -xJf "$tmp" -C "$root/third_party"
	mv "$root/third_party/linux-6.6.155" "$root/third_party/linux"
fi
"$root/scripts/configure-linux.sh"
