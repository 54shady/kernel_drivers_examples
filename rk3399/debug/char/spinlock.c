#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#define DEVICE_NAME "spinlock_test"

static char *data = "read\n";
static char flag = 1;
static DEFINE_SPINLOCK(lock);

void do_some_working(int ms)
{
	mdelay(ms);
}

/*
 * 用cat读取设备文件是,传入的字节数是32768
 * 而这里的read函数只返回msg_len = 4个字符
 * 因此用了flag来控制读,当返回了"read"后
 * 会立刻返回0,否则cat会不断调用spinlock_read函数
 * 就会出现不断输出"read"的现象
 */
static ssize_t spinlock_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int msg_len = strlen(data);

	if (copy_to_user(buf, (void *)data, msg_len))
		return -EINVAL;

	if (flag)
	{
		flag = 0;
		if (spin_trylock(&lock))
		{
			/* do 10s the working */
			printk("reading...occupy the lock\n");
			do_some_working(10000);
			spin_unlock(&lock);
		}
		else
		{
			return -EBUSY;
		}
		return msg_len;
	}
	else
	{
		flag = 1;
		return 0;
	}
}

/*
 * 测试:
 * echo lock > /dev/spinlock_test
 * echo trylock > /dev/spinlock_test
 * 用echo写入文件时会自动添加"\n"
 * 所以在判断的时候也要判断"\n"
 */
static ssize_t spinlock_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char data[10];

	memset(data, 0 , 10);

	if (copy_from_user(data, buf, count))
		return -EINVAL;

	/* if write data is "lock" */
	if (strcmp("lock\n", data) == 0)
	{
		spin_lock(&lock);

		/* do 10s the working */
		printk("writing...occupy the lock\n");
		do_some_working(10000);

		spin_unlock(&lock);
	}
	else if (strcmp("trylock\n", data) == 0)
	{
		if (spin_trylock(&lock))
		{
			/* do 10s the working */
			do_some_working(10000);

			spin_unlock(&lock);
		}
		else
		{
			printk("spin lock unavaliable\n");
			return -EBUSY;
		}
	}

	return count;
}

static struct file_operations spinlock_fops = {
	.owner = THIS_MODULE,
	.read = spinlock_read,
	.write = spinlock_write
};

static struct miscdevice spinlock_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &spinlock_fops
};

static int spinlock_init(void)
{
	int ret;

	ret = misc_register(&spinlock_misc);
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return ret;
}

static void spinlock_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	misc_deregister(&spinlock_misc);
}

module_init(spinlock_init);
module_exit(spinlock_exit);
MODULE_LICENSE("GPL");
