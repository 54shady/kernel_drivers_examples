# BPF samples

## Usage

使用对应文件

	ln -s $PWD/demo_kern.c /usr/src/linux/samples/bpf
	ln -s $PWD/demo_user.c /usr/src/linux/samples/bpf

修改makefile(/usr/src/linux/samples/bpf/Makefile)

添加目标(demo)

	tprogs-y += demo

demo依赖demo_user

	demo-objs := demo_user.o $(TRACE_HELPERS)

编译对应的bpf模块

	always-y += demo_kern.o

需要县编译好内核并安装头文件

	cd /usr/src/linux
	make defconfig && make && make headers_install

编译bpf模块

	make M=samples/bpf

或者在bpf目录编译

	cd /usr/src/linux/samples/bpf && make

用file查看下编译输出文件(demo_user.o和demo_kern.o)

	demo_user.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
	demo_kern.o: ELF 64-bit LSB relocatable, eBPF, version 1 (SYSV), with debug_info, not stripped

- demo_kern.c中的文件是被编译成eBPF的二进制文件,是要被eBPF虚拟机执行的代码
- demo_user.c是将eBPF二进制文件加载到内核的代码


## 使用内核中的samples(commit bpf: add sample usages for persistent maps/progs)

先要挂载对应的文件系统

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
