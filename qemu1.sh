#!/bin/bash
# sudo setenforce 0
# sudo mkdir -p /mnt/hostshare
# sudo mount -t 9p -o trans=virtio,version=9p2000.L hostshare /mnt/hostshare

# wfn, ipads123

qemu-system-x86_64 \
	-cpu host -m 16G -smp 8 --enable-kvm -boot d \
	-hda /disk/$USER/share-fs/fedora-img1.qcow2 \
	-device ivshmem-plain,memdev=ivshmem \
	-object memory-backend-file,size=4G,share=on,mem-path=/dev/shm/ivshmem-$USER,id=ivshmem \
	-netdev passt,id=net0 \
	-device virtio-net-pci,netdev=net0 \
	-fsdev local,id=hostshare,path=$(pwd),security_model=passthrough \
	-device virtio-9p-pci,fsdev=hostshare,mount_tag=hostshare \
	-nographic -serial mon:stdio \
	# -kernel ./linux-6.11.8/vmlinux -append "console=ttyS0 root=/dev/vda1" \
	# -s -S \
