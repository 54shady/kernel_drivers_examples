# Run x86 program on arm platform using box64

[Linux Userspace x86_64 Emulator with a twist](https://github.com/ptitSeb/box64)

on x86 compile a program

	gcc -o hello hello.c

run x86 program on arm, which will end with Exec format error

	arm:~# ./hello
	-bash: ./hello: cannot execute binary file: Exec format error

	arm:~# file hello
	hello: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=dc5d2267acfcd02843c5d2d20a4e2c6f84613004, for GNU/Linux 3.2.0, not stripped

## Using box64

compile and install box64 on rk3588 arm board

	git clone https://github.com/ptitSeb/box64
	cd box64/
	mkdir build; cd build; cmake .. -D ARM_DYNAREC=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -DBAD_SIGNAL=ON
	make -j4
	make install

start binfmt and using box64 to run again

	systemctl start systemd-binfmt

run x86 program on arm

	arm:~# ./hello   //默认会使用box64
	//arm:~# box64 ./hello

	Dynarec for ARM64, with extension: ASIMD AES CRC32 PMULL ATOMICS SHA1 SHA2 PageSize:4096 Running on Cortex-A55 with 8 Cores
	Will use Hardware counter measured at 24.0 MHz emulating 3.0 GHz
	Params database has 81 entries
	Box64 with Dynarec v0.3.1 ebf698fd built on Jul 13 2024 03:27:11
	BOX64: Detected 48bits at least of address space
	Counted 34 Env var
	BOX64 LIB PATH: ./:lib/:lib64/:x86_64/:bin64/:libs64/:/lib/x86_64-linux-gnu/:/usr/lib/x86_64-linux-gnu/
	BOX64 BIN PATH: ./:bin/:/usr/local/sbin/:/usr/local/bin/:/usr/sbin/:/usr/bin/:/sbin/:/bin/
	Looking for ./hello
	Rename process to "hello"
	Using native(wrapped) libc.so.6
	Using native(wrapped) ld-linux-x86-64.so.2
	Using native(wrapped) libpthread.so.0
	Using native(wrapped) libdl.so.2
	Using native(wrapped) libutil.so.1
	Using native(wrapped) libresolv.so.2
	Using native(wrapped) librt.so.1
	Using native(wrapped) libbsd.so.0
	main, 5
