# Makefile
# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Usage
# make CC=<your_compiler_path> KERNELDIR=<your_kernel_dir>
#
# make CC=/home/zeroway/3288/src/3288_4.4/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc KERNELDIR=/home/zeroway/3288/src/3288_4.4/kernel

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
DEBFLAGS = -O2
endif

obj-m := i2c_regmap.o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
CC ?= gcc
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
