# RK3288 kernel driver example

## 目录说明

- app 是测试驱动的应用程序
- debug 是驱动模板
- script 是测试脚本

## 各模块入口

[Kernel debug technic](./debug/debug_technic.md)

[HELLO](./debug/hello)

[I2C](./debug/i2c)

[SPI](./debug/spi)

[USB](./debug/usb)

[Regulator (ACT8846, RK818)](./debug/regulator)

[Platform_Driver_Test](./debug/platform_driver_test)

[PWM Backlight](./debug/pwm)

[Regmap](./debug/regmap)

[Audio (ES8323, ES8316)](./debug/codec)

[Timer](./debug/timer)

[Timer And Workqueue](./debug/timer_workq)

[Workqueue](./debug/workqueue)

[Notify](./debug/notify_chain)

[Script](./script)

[PowerDomain](./debug/misc)

[GDB on android](./debug/android_gdb.md)

[Generic Netlink](./debug/netlink)

## 使用方法(单独编译模块)或者放到内核目录中编译

- make CC=your_compiler_path KERNELDIR=your_kernel_dir

## 举例(以hello模块为例)

### 编译内核时没有指定O选项

- cd debug/hello
- make CC=/3288androidsrc/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc KERNELDIR=/3288androidsrc/kernel

### 编译内核时有指定O选项

- cd debug/hello
- make CROSS_COMPILE=/3288androidsrc/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- KERNEL_DIR=/3288androidsrc/kernel KERNEL_BUID_OUTPUT=/3288androidsrc/out/target/product/rk3288/obj/KERNEL
