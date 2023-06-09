# qemu中虚拟设备实现

参考书:How to Develop Embedded Software Using The QEMU Machine Emulator.pdf

## 设备和驱动对应的代码(环境参考huc)

crypto.c : virtual device in qemu
crypto-drv.c : driver for crypto

## 代码准备

### 将设备添加到qemu中

qemu code(将crypto.c放入hw/misc/下进行编译)

	git checkout 59e1b8a22e -b qdrv

patch virtio-mini code

	ln -s ${PWD}/0001-Add-virtio-mini-device.patch ${HOME}/src/crypto-qemu/
	cd ${HOME}/src/crypto-qemu
	git apply 0001-Add-virtio-mini-device.patch

patch crypto and hello-pci

	ln -s ${PWD}/hello-pci-dev.c ${HOME}/src/crypto-qemu/hw/misc
	ln -s ${PWD}/crypto.c ${HOME}/src/crypto-qemu/hw/misc
	echo "softmmu_ss.add(files('hello-pci-dev.c'))" >> ${HOME}/src/crypto-qemu/hw/misc/meson.build
	echo "softmmu_ss.add(files('crypto.c'))" >> ${HOME}/src/crypto-qemu/hw/misc/meson.build

Compile qemu and run

	./configure --target-list=x86_64-softmmu \
		--extra-ldflags="`pkg-config --libs openssl`"

	${HOME}/src/crypto-qemu/build/x86_64-softmmu/qemu-system-x86_64 \
		-drive file=/data/huc.qcow2 \
		-smp 2 -m 1024 -enable-kvm \
		-device pci-crypto,aes_cbc_256="abc" \
		-device e1000,netdev=ssh \
		-display none \
		-serial mon:stdio \
		-vnc 0.0.0.0:0 \
		-device pci-hellodev \
		-device virtio-mini,disable-legacy=on \
		-netdev user,id=ssh,hostfwd=tcp::2222-:22

### 编译对应的设备驱动

	docker run --rm -it --privileged \
		-v ${PWD}:/code \
		-v ${HOME}/src/linux:/usr/src/linux bpf2004 make

## 调试

### PCI寄存器的查看和修改(使用setpci)

进系统后查看pci设备寄存器(通用)

	setpci --dumpregs

	00 W VENDOR_ID
	02 W DEVICE_ID
	04 W COMMAND
	06 W STATUS
	08 B REVISION
	09 B CLASS_PROG
	...
	10 L BASE_ADDRESS_0

用lspci -n 找到对应设备的BDF值(在驱动中定义了vendorId 0x2222, deviceId 0x1111)

	00:03.0 00ff: 1111:2222

查看设备对应寄存器的值(比如查看VENDOR_ID, 由dumpregs可知寄存器偏移量为00)

	setpci -s 00:03.0 00.w

再比如查看device id

	setpci -s 00:03.0 02.w

查看设备的bar0的地址

	setpci -s 00:03.0 10.l

或者使用lspci查看(下面的0xfebf1000就是bar0地址, 即mmio的地址)

	lspci -vvvv -xxxx -s 00:03.0
	Region 0: Memory at febf1000 (32-bit, non-prefetchable) [size=4K]

或者通过qemu monitor查看也能查看到该地址

	(qemu) info qtree
	(qemu) info mtree

### 使用[devmem](https://github.com/VCTLabs/devmem2)工具来调试

编译工具

	gcc devmem2.c -o devmem

下面命令会调用到mmio的read函数

	./devmem 0xfebf1000 b
	./devmem 0xfebf1000 w
	./devmem 0xfebf1000 l

写mmio空间的第二个地址0xfebf1000 + 2,命令是1:对应的是reset

	./devmem 0xfebf1002 b 1

Encrypt:命令是2

	./devmem 0xfebf1002 b 2

Decrypt:命令是3

	./devmem 0xfebf1002 b 3

Enable interrupt

	./devmem 0xfebf1003 b 2

Disable interrupt

	./devmem 0xfebf1003 b 0

### 其它信息查看

查看设备对应的io空间

	grep mycrypto /proc/iomem
		febf1000-febf1fff : mycrypto

查看驱动对应的符号表

	grep crypto_drv  /proc/kallsyms
