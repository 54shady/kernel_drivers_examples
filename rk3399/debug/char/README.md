# 字符设备驱动

## 使用spinlock保护临界区资源

### 测试方法

首先执行

	cat /dev/spinlock_test &

在10秒内执行下面命令看结果

	echo lock > /dev/spinlock_test

在10秒内执行下面命令看结果

	echo trylock > /dev/spinlock_test

测试注意

- 用cat读取设备文件是,传入的字节数是32768,而这里的read函数只返回msg_len = 4个字符,因此用了flag来控制读,当返回了"read"后,会立刻返回0,否则cat会不断调用spinlock_read函数,就会出现不断输出"read"的现象

- 用echo写入时会自动添加"\n",所以在判断的时候也要判断"\n"
