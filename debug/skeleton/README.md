# 如何使用

替换VENDOR和MODULE为相应的名字

	%s/VENDOR/your_verdor_name/g
	%s/MODULE/your_module_name/g

编译说明

	CROSS_COMPILE指定交叉编译工具链的前缀

	KERNEL_DIR执行内核代码路径

	编译内核时没有使用O选项指定编译生成文件的输入路径
	KERNEL_BUID_OUTPUT 就和内核路径一致

	编译内核时有使用O选项指定编译生成文件的输入路径
	KERNEL_BUID_OUTPUT 就为相应的路径
