#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static int c_show(struct seq_file *m, void *v)
{
	int i;
	for (i = 0; i < 100; i++)
		seq_printf(m, "Hardware\t: %s, %d\n", "machine_name", i);

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	++*pos;
	return NULL;
}

static void c_stop(struct seq_file *m, void *v)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show
};

static int cpuinfo_open(struct inode *inode, struct file *file)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return seq_open(file, &cpuinfo_op);
}

static const struct file_operations proc_cpuinfo_operations = {
	.open		= cpuinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init proc_cpuinfo_init(void)
{
	proc_create("mycpuinfo", 0, NULL, &proc_cpuinfo_operations);
	return 0;
}

static void proc_cpuinfo_exit (void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	remove_proc_entry("mycpuinfo", NULL);
}

module_init(proc_cpuinfo_init);
module_exit(proc_cpuinfo_exit);
MODULE_LICENSE("GPL");
