# kernel_drivers_examples

Codes test on the kernel version below

	3.10.49
	3.10.79

# 目录说明

- app 是测试驱动的应用程序
- debug 是驱动模板
- script 是测试脚本

# 各模块入口

[HELLO](./debug/hello/README.md)

[I2C](./debug/i2c/README.md)

[SPI](./debug/spi/README.md)

[Regulator (ACT8846, RK818)](./debug/regulator/README.md)

[Platform_Driver_Test](./debug/platform_driver_test/README.md)

[PWM Backlight](./debug/pwm/README.md)

[Regmap](./debug/regmap/README.md)

[Audio (ES8323, ES8316)](./debug/codec/README.md)

[Timer](./debug/timer/README.md)

[Timer And Workqueue](./debug/timer_workq/README.md)

[Workqueue](./debug/workqueue/README.md)

[Notify](./debug/notify_chain/README.md)

[Script](./script/README.md)

# 使用方法(单独编译模块)或者放到内核目录中编译

- make CC=your_compiler_path KERNELDIR=your_kernel_dir

## 举例(以hello模块为例)

### 编译内核时没有指定O选项

- cd debug/hello
- make CC=/home/zeroway/3288/src/3288_4.4/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc KERNELDIR=/home/zeroway/3288/src/3288_4.4/kernel

### 编译内核时有指定O选项

- cd debug/hello
- make CROSS_COMPILE=/home/zeroway/3288/51/src/3288_5.1_v2/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- KERNEL_DIR=/home/zeroway/3288/51/src/3288_5.1_v2/kernel KERNEL_BUID_OUTPUT=/home/zeroway/3288/51/src/3288_5.1_v2/out/target/product/rk3288/obj/KERNEL

