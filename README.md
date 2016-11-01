# kernel_drivers_examples

kernel 3.10.49 drivers test examples

# 目录说明

- app 是测试驱动的应用程序
- debug 是驱动模板
- script 是测试脚本

# Usage

- make CC=your_compiler_path KERNELDIR=your_kernel_dir

## example(build the hello module)

- cd debug/hello

- make CC=/home/zeroway/3288/src/3288_4.4/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc KERNELDIR=/home/zeroway/3288/src/3288_4.4/kernel
