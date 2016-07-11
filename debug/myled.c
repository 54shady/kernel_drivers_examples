#include "myled.h"

static long myled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	char msg[1024];

	switch (cmd)
	{
		case IOCTL_INT_VALUE:
			printk("cmd: %d arg = %ld, %s, %d\n", cmd, arg, __FUNCTION__, __LINE__);
			break;
		case IOCTL_STRING_VALUE:
			if (copy_from_user(&msg, argp, sizeof(msg)))
			{
				printk("error fault\n");
				return -EFAULT;
			}

			printk("msg = %s\n", msg);
			break;
		default:
			break;
	}

	return 0;
}

static int myled_release(struct inode *inode, struct file *file)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int myled_open(struct inode *inode, struct file *file)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct file_operations myled_fops = {
	.owner = THIS_MODULE,
	.open = myled_open,
	.release = myled_release,
	.unlocked_ioctl = myled_ioctl
};

static struct miscdevice myled_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "myledmiscname",
	.fops = &myled_fops
};

static int __exit myled_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	misc_deregister(&myled_misc);
	return 0;
}

static int myled_probe(struct platform_device *pdev)
{
	int rc;
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	rc = misc_register(&myled_misc);
	if (rc < 0) {
		pr_err("%s: could not register misc device\n", __func__);
		return -1;
	}

	return 0;
}

static const struct of_device_id myled_dt_ids[] = {
	{ .compatible = "myled", .data = NULL, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, myled_dt_ids);

static struct platform_driver myled_driver = {
	.driver		= {
		.name	= "myled",
		.owner = THIS_MODULE,
		.of_match_table = myled_dt_ids,
	},
	.probe		= myled_probe,
	.remove		= myled_remove,
};

module_platform_driver(myled_driver);

MODULE_DESCRIPTION("led driver");
MODULE_LICENSE("GPL");
