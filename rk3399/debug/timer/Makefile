# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make CROSS_COMPILE=<cross_compiler_prefix> KERNEL_DIR=<your_kernel_dir> KERNEL_BUID_OUTPUT=<kernel_buid_output>
#
# make CROSS_COMPILE=/home/zeroway/rk3399/src/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android- KERNEL_DIR=/home/zeroway/rk3399/src/kernel KERNEL_BUID_OUTPUT=/home/zeroway/rk3399/src/out/target/product/rk3399_firefly_box/obj/KERNEL

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := mytimer_test.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_BUID_OUTPUT ?=$(KERNEL_DIR)
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) ARCH=arm64 M=$(PWD) O=$(KERNEL_BUID_OUTPUT) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
