# kernel_drivers_examples

kernel 3.10.49 drivers test examples

# 目录说明

- app 是测试驱动的应用程序
- debug 是驱动模板
- script 是测试脚本

# 使用方法(单独编译模块)或者放到内核目录中编译

- make CC=your_compiler_path KERNELDIR=your_kernel_dir

## 举例(以hello模块为例)

- cd debug/hello
- make CC=/home/zeroway/3288/src/3288_4.4/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc KERNELDIR=/home/zeroway/3288/src/3288_4.4/kernel

## 各模块入口

[hello](./debug/hello/README.md)

[regulator](./debug/regulator/README.md)

[platform_driver_test](./debug/platform_driver_test/README.md)

[pwm](./debug/pwm/README.md)

[regmap](./debug/regmap/README.md)

[script](./script/README.md)
