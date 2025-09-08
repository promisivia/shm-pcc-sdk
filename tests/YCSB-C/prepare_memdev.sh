numactl --cpunodebind=0 --membind=0 dd if=/dev/zero of=/dev/shm/cxl bs=1G count=64
