#KERN_DIR := /lib/modules/$(shell uname -r)/build
KERN_DIR := /work/jz2440_code/linux-2.6.22.6

CUR_DIR := $(shell pwd)
CC := arm-linux-gcc

obj-m += globalmem.o
all:globalmem.c
	make -C $(KERN_DIR) M=$(CUR_DIR) modules
	sudo cp globalmem.ko /work/nfs/nfs_root/globalmem/
#install:
#	insmod globalmem.ko
#unstall:
#	rmmod globalmem.ko

test:
	$(CC) -o test_globalmem global_test.c
	sudo cp test_globalmem /work/nfs/nfs_root/globalmem/
clean:
	make -C $(KERN_DIR) M=$(CUR_DIR) clean
	-rm test_globalmem
.PHONY:clean
