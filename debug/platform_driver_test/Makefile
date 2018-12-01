# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

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

obj-m := test_pf.o
obj-m += show_store_pf.o
obj-m += skeleton_ss.o
obj-m += spinlock_test.o
obj-m += mutex_test.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_BUILD_OUTPUT ?=$(KERNEL_DIR)
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

modules:
	$(MAKE) CROSS_COMPILE=$(cross_compiler_prefix) -C $(kernel_dir) O=$(kernel_output) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
