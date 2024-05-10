# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make CROSS_COMPILE=<your_compiler_path> KERNELDIR=<your_kernel_dir>
#
# export CROSS_COMPILE=~/src/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
# make KERNELDIR=~/src/kernel/linux-5.10/
#
# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := hello.o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
CROSS_COMPILE ?= gcc
PWD := $(shell pwd)

modules:
	$(MAKE) LLVM=1 LLVM_IAS=1 ARCH=arm64 -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers *.mod

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
