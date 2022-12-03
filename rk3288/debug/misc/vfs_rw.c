#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/* read write buffer */
static char wbuf[] = "this is a kernel read write test";
static char rbuf[1024];

#define FILE_NAME "/tmp/vfs_rw_test"

/*
 * 在内核中一般不容易生成用户空间的指针
 * 或者不方便独立使用用户空间内存
 * 而vfs_read,vfs_write的参数buffer都是用户空间指针
 * 所以需要调用set_fs目的是为了让内核改变对内存地址检查的处理方式
 * set_fs参数只有两个USER_DS和KERNEL_DS
 * USER_DS代表用户空间,set_fs
 * KERNEL_DS代表内核空间
 */
static int vfs_rw_init(void)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* open file */
	fp = filp_open(FILE_NAME, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp))
	{
		printk("create file error\n");
		return -1;
	}

	/* write file */

	/* get current fs*/
	fs = get_fs();

	/* 即对内核空间地址检查并做变换 */
	set_fs(KERNEL_DS);
	pos = 0;
	printk("=> %s\n", wbuf);
	vfs_write(fp, wbuf, sizeof(wbuf), &pos);

	/* read file */
	pos = 0;
	vfs_read(fp, rbuf, sizeof(wbuf), &pos);
	printk("<= %s\n", rbuf);

	/* close file */
	filp_close(fp, NULL);
	set_fs(fs);

	return 0;
}

static void vfs_rw_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(vfs_rw_init);
module_exit(vfs_rw_exit);
MODULE_LICENSE("GPL");
