#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
linux="$root/third_party/linux"
rootfs="$root/vm/rootfs"
jobs=${JOBS:-32}

if [ ! -f "$linux/.config" ]; then
	"$root/scripts/configure-linux.sh"
fi

make -C "$linux" -j"$jobs" bzImage modules
make -C "$linux" M="$root/kernel" modules

cmake -S "$root/third_party/eRPC" -B "$root/build/upstream-erpc-vm" \
	-DTRANSPORT=fake -DPERF=ON -DLOG_LEVEL=warn
cmake --build "$root/build/upstream-erpc-vm" \
	--target hello_server hello_client -j"$jobs"

mkdir -p "$root/build/vm" "$rootfs/bin" "$rootfs/dev" "$rootfs/proc" "$rootfs/sys"
cc -static -O2 -Wall -Wextra -Werror \
	-I"$root/include" -I"$root/include/uapi" \
	-c "$root/lib/lrpc.c" -o "$root/build/vm/lrpc.o"
cc -static -O2 -Wall -Wextra -Werror \
	-c "$root/lib/switch_stack.S" -o "$root/build/vm/switch_stack.o"
cc -static -O2 -Wall -Wextra -Werror \
	-I"$root/include" -I"$root/include/uapi" -I"$root/demo" \
	"$root/demo/publisher.c" "$root/build/vm/lrpc.o" \
	"$root/build/vm/switch_stack.o" -o "$rootfs/lrpc-publisher"
cc -static -O2 -Wall -Wextra -Werror \
	-I"$root/include" -I"$root/include/uapi" \
	"$root/demo/shadow.c" "$root/build/vm/lrpc.o" \
	"$root/build/vm/switch_stack.o" -o "$rootfs/lrpc-shadow"
c++ -static -O2 -Wall -Wextra -Werror -std=c++17 \
	-I"$root/include" -I"$root/include/uapi" -I"$root/demo" \
	"$root/demo/erpc_client.cc" "$root/build/vm/lrpc.o" \
	"$root/build/vm/switch_stack.o" -o "$rootfs/erpc-lrpc-client"

cp /usr/bin/busybox "$rootfs/bin/busybox"
cp "$root/kernel/ub_lrpc.ko" "$rootfs/ub_lrpc.ko"
cp "$root/third_party/eRPC/build/hello_server" "$rootfs/upstream-erpc-server"
cp "$root/third_party/eRPC/build/hello_client" "$rootfs/upstream-erpc-client"
mkdir -p "$rootfs/lib/x86_64-linux-gnu" "$rootfs/lib64" "$rootfs/mnt/huge" "$rootfs/tmp"
cp /lib64/ld-linux-x86-64.so.2 "$rootfs/lib64/ld-linux-x86-64.so.2"
for lib in libnuma.so.1 libstdc++.so.6 libgcc_s.so.1 libc.so.6 libm.so.6; do
	cp "/lib/x86_64-linux-gnu/$lib" "$rootfs/lib/x86_64-linux-gnu/$lib"
done
cp "$root/vm/init" "$rootfs/init"
chmod 755 "$rootfs/init" "$rootfs/lrpc-publisher" "$rootfs/lrpc-shadow" \
	"$rootfs/erpc-lrpc-client"
for applet in sh mount insmod cat sleep poweroff ip mkdir env; do
	ln -sf busybox "$rootfs/bin/$applet"
done

(cd "$rootfs" && find . -print0 | cpio --null -o --format=newc 2>/dev/null) |
	gzip -9 > "$root/vm/initramfs.cpio.gz"
cp "$linux/arch/x86/boot/bzImage" "$root/vm/bzImage"
echo "VM artifacts: $root/vm/bzImage $root/vm/initramfs.cpio.gz"
