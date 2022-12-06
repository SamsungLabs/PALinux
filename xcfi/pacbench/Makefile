PWD := $(shell pwd)
obj-m := pacbench.o
pacbench-objs := main.o feature.o keychange.o instruction.o

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j $(ncpus) -C $(KDIR) KBUILD_EXTMOD=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod *.cmd .tmp_versions *.symvers *.order *.mod.c *.swp
