#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#define CHAR_SKELETON_COUNT	1

static int major;
static struct cdev char_skeleton_cdev;
static struct class *cls;

struct self_define_data
{
	int arg1;
	int arg2;
};

static int char_skeleton_open(struct inode *inode, struct file *file)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t char_skeleton_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	struct self_define_data sdd;

	if (copy_from_user(&sdd, buf, count) != 0)
		return -1;

	printk("char_skeleton_write, arg1= %d, arg2 = %d\n", sdd.arg1, sdd.arg2);

	return count;
}

static struct file_operations char_skeleton_fops = {
	.owner	= THIS_MODULE,
	.open 	= char_skeleton_open,
	.write  = char_skeleton_write,
};

static int char_skeleton_init(void)
{
	dev_t dev_id;
	int retval;

	/*
	 * 如果有主设备号,根据主设备号得到设备ID,并注册
	 * 如果没有主设备号,让系统随机分配一个设备ID,根据设备ID获得主设备号
	 */
	if (major)
	{
		dev_id = MKDEV(major, 0);
		retval = register_chrdev_region(dev_id, CHAR_SKELETON_COUNT, "char_skeleton");
	}
	else
	{
		retval = alloc_chrdev_region(&dev_id, 0, CHAR_SKELETON_COUNT, "char_skeleton");
		major = MAJOR(dev_id);
	}

	if (retval) {
		printk("register or alloc region error\n");
		return -1;
	}

	/* 字符设备的注册 */
	/* 内核在内部使用类型 struct cdev 的结构来代表字符设备 */
	/* 初始化char device */
	cdev_init(&char_skeleton_cdev, &char_skeleton_fops);

	/* 关联char devices 和 dev_id */
	cdev_add(&char_skeleton_cdev, dev_id, CHAR_SKELETON_COUNT);

	/* 创建相关的设备节点 */
	cls = class_create(THIS_MODULE, "char_skeleton");
	if (IS_ERR(cls))
		return -EINVAL;

	device_create(cls, NULL, MKDEV(major, 0), NULL, "char_skeleton");/* /dev/char_skeleton */

	return 0;
}

static void char_skeleton_exit(void)
{
	dev_t dev_id = MKDEV(major, 0);

	device_destroy(cls, dev_id);
	class_destroy(cls);
	cdev_del(&char_skeleton_cdev);
	unregister_chrdev_region(dev_id, CHAR_SKELETON_COUNT);
}

module_init(char_skeleton_init);
module_exit(char_skeleton_exit);
MODULE_LICENSE("GPL");
