# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make CROSS_COMPILE=<cross_compiler_prefix> KERNEL_DIR=<your_kernel_dir> KERNEL_BUID_OUTPUT=<kernel_buid_output>
#
# make CROSS_COMPILE=/home/zeroway/3288/51/src/3288_5.1_v2/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- KERNEL_DIR=/home/zeroway/3288/51/src/3288_5.1_v2/kernel KERNEL_BUID_OUTPUT=/home/zeroway/3288/51/src/3288_5.1_v2/out/target/product/rk3288/obj/KERNEL

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := demo1_kernel.o demo2_kernel.o
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_BUID_OUTPUT ?=$(KERNEL_DIR)
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) O=$(KERNEL_BUID_OUTPUT) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
