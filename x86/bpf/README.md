# BPF samples

## Using Ubuntu 22.04 as compilation setup

[Ubuntu get source code](https://wiki.ubuntu.com/Kernel/SourceCode)
[Ubuntu Build Your Own Kernel](https://wiki.ubuntu.com/Kernel/BuildYourOwnKernel)

安装必要的软件依赖

	apt install -y dpkg-dev
	apt-get build-dep linux linux-image-$(uname -r)

在Docker中可以通过指定具体的版本来安装内核代码(比如5.15.0-57-generic)或者预先下载好代码

	apt-get build-dep linux linux-image-5.15.0-57-generic

安装编译依赖

	apt-get install libncurses-dev gawk flex bison openssl libssl-dev dkms \
		libelf-dev libudev-dev libpci-dev libiberty-dev autoconf llvm libcap2-dev \
		rsync bc pahole git clang binutils-dev dpkg-dev

ubuntu解决安装包冲突问题

	apt-get install aptitude
	aptitude install <package-name>

使用[docker](Dockerfile)来构建编译环境

	docker build . -t bpf

下载代码(等价于下面的git命令)到/usr/src/linux目录下

	apt-get -y source linux-image-unsigned-$(uname -r)
		==> git clone git://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/jammy

	cd /path/to/download/ubuntu/source
	ln -s $PWD /usr/src/linux
	cd /usr/src/linux

直接将预先下载好的代码映射到docker中

	docker -v /path/to/linux:/usr/src/linux bpf /bin/bash

修改内核配置

	chmod +x scripts/*
	make clean
	make defconfig
	./scripts/config -e CONFIG_FTRACE
	./scripts/config -e CONFIG_DEBUG_INFO
	./scripts/config -e CONFIG_DEBUG_INFO_DWARF5
	./scripts/config -e CONFIG_BPF_SYSCALL
	./scripts/config -e CONFIG_DEBUG_INFO_BTF
	./scripts/config -d CONFIG_DEBUG_INFO_REDUCED
	yes "" | make oldconfig

安装头文件到当前编译目录

	make headers_install

编译内核

	make -j$(nproc) \
	    KERNEL_DIR=/usr/src/linux \
	    BPF_TOOLS_PATH=bpf_tools \
	    BPF_EXAMPLES_PATH=bpf_examples \
		CC=/usr/bin/gcc

编译内核自带的用例

	make M=samples/bpf

或者在bpf目录编译

	cd /usr/src/linux/samples/bpf && make

### 运行内核自带的用列

在运行环境中挂载对应的文件系统

	mount -t bpf bpf /sys/fs/bpf

create map and insert elm

	./fds_example -F /sys/fs/bpf/m -P -m -k 1 -v 42
	bpf: map fd:3 (Success)
	bpf: pin ret:(0,Success)
	bpf: fd:3 u->(1:42) ret:(0,Success)

get elm

	./fds_example -F /sys/fs/bpf/m -G -m -k 1
	bpf: get fd:3 (Success)
	bpf: fd:3 l->(1):42 ret:(0,Success)

update elm

	./fds_example -F /sys/fs/bpf/m -G -m -k 1 -v 24
	bpf: get fd:3 (Success)
	bpf: fd:3 u->(1:24) ret:(0,Success)

get elm

	./fds_example -F /sys/fs/bpf/m -G -m -k 1
	bpf: get fd:3 (Success)
	bpf: fd:3 l->(1):24 ret:(0,Success)

## 使用本demo(using docker to compile)

将当前代码挂载到容器中

	docker -v $PWD:/code -v /path/to/linux:/usr/src/linux bpf /bin/bash

在当前目录操作使用对应文件

	ln -s $PWD/demo_kern.c /usr/src/linux/samples/bpf
	ln -s $PWD/demo_user.c /usr/src/linux/samples/bpf
	mv /usr/src/linux/samples/bpf/Makefile /usr/src/linux/samples/bpf/Makefile-org
	ln -s $PWD/Makefile /usr/src/linux/samples/bpf

在当前目录编译时指定kernel代码路径

    make KERNEL_DIR=/usr/src/linux clean
    make KERNEL_DIR=/usr/src/linux

用file查看下编译输出文件(demo_user.o和demo_kern.o)

	demo_user.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
	demo_kern.o: ELF 64-bit LSB relocatable, eBPF, version 1 (SYSV), with debug_info, not stripped

- demo_kern.c中的文件是被编译成eBPF的二进制文件,是要被eBPF虚拟机执行的代码
- demo_user.c是将eBPF二进制文件加载到内核的代码
