#include <linux/of.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

/* self define data */
struct self_define_data {
	int age;
	char name[20];
};

#define GET_VALUE 10
#define PUT_VALUE 11

static struct self_define_data sdd = {
	.age = 0,
	.name = "anonymous"
};

static long ukio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	pr_debug("%s, %d, cmd = %d\n", __FUNCTION__, __LINE__, cmd);
	switch (cmd)
	{
		case GET_VALUE:
			pr_debug("%s %d age\n", sdd.name, sdd.age);
			copy_to_user((void __user *)argp, (void *)&sdd, sizeof(struct self_define_data));
			break;
		case PUT_VALUE:
			copy_from_user((void *)&sdd, (void __user *)argp, sizeof(struct self_define_data));
			pr_debug("%s, %d\n", sdd.name, sdd.age);
			break;
		default:
			break;
	}

	return 0;
}

static int ukio_release(struct inode *inode, struct file *file)
{
	pr_debug("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int ukio_open(struct inode *inode, struct file *file)
{
	pr_debug("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct file_operations ukio_fops = {
	.owner = THIS_MODULE,
	.open = ukio_open,
	.release = ukio_release,
	.unlocked_ioctl = ukio_ioctl,
	.compat_ioctl = ukio_ioctl
};

static struct miscdevice miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "user_kernel_io",
	.fops = &ukio_fops
};

static int AAAAA_platform_probe(struct platform_device *pdev)
{
	int ret;

    ret = misc_register(&miscdev);
	if (ret < 0)
		pr_debug("misc register error\n");

	return 0;
}

static const struct of_device_id AAAAA_platform_dt_ids[] = {
	{ .compatible = "test_platform", },
	{}
};

static int AAAAA_platform_remove(struct platform_device *pdev)
{
	int ret;

	pr_debug("%s, %d\n", __FUNCTION__, __LINE__);
	misc_deregister(&miscdev);
	return 0;
}

static struct platform_driver AAAAA_platform_driver = {
	.driver		= {
		.name	= "test platform driver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(AAAAA_platform_dt_ids),
	},
	.probe	= AAAAA_platform_probe,
	.remove = AAAAA_platform_remove,
};

static int AAAAA_platform_init(void)
{
	return platform_driver_register(&AAAAA_platform_driver);
}

static void AAAAA_platform_exit(void)
{
	platform_driver_unregister(&AAAAA_platform_driver);
}

module_init(AAAAA_platform_init);
module_exit(AAAAA_platform_exit);
MODULE_LICENSE("GPL");
