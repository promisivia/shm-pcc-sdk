obj-m+=uncached_ram.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
install:
	sudo insmod uncached_ram.ko

# manually: sudo mknod /dev/uncached_ram [dev major] 0
