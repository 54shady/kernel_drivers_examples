# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make CROSS_COMPILE=<cross_compiler_prefix> KERNEL_DIR=<your_kernel_dir> KERNEL_BUID_OUTPUT=<kernel_buid_output>
#

PWD := $(shell pwd)
BUILD_ENV = $(PWD)/../../buildenv
cross_compiler_prefix = $(shell awk -F "CROSS_COMPILE=" '/CROSS_COMPILE=/{print $$2}' $(BUILD_ENV))
kernel_output = $(shell awk -F "KERNEL_BUILD_OUTPUT=" '/KERNEL_BUILD_OUTPUT=/{print $$2}' $(BUILD_ENV))
kernel_dir = $(shell awk -F "KERNEL_DIR=" '/KERNEL_DIR=/{print $$2}' $(BUILD_ENV))

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := uk_io.o
obj-m += vfs_rw.o
obj-m += wei.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_BUID_OUTPUT ?=$(KERNEL_DIR)
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
PWD := $(shell pwd)

modules:
	$(MAKE) CROSS_COMPILE=$(cross_compiler_prefix) -C $(kernel_dir) O=$(kernel_output) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
