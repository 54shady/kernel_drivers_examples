# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make KERNEL_DIR=/home/zeroway/github/4412dt/new_kernel/linux-4.4 KERNEL_BUID_OUTPUT=/home/zeroway/github/4412dt/new_kernel/linux-4.4/out ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi-

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := buttons.o
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
