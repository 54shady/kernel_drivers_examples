#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

/* data store in list */
struct ldata {
	struct list_head list;
	int value;
};

#define TEST_NODE_NAME "ldata"
#define TEST_DIR_NAME "test"

/* global variables */
static struct mutex lock;
struct proc_dir_entry *p_dir_name;
struct dentry *d_dir_name;
static struct list_head head;

static void add_one_record(void)
{
	struct ldata *data;
	int v;

	mutex_lock(&lock);
	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (data != NULL)
	{
		list_add(&data->list, &head);
		get_random_bytes(&v, 4);
		printk("Insert %d into seq show\n", v);
		data->value = v;
	}
	mutex_unlock(&lock);
}

/* echo 0 > /proc/test/mydata to trigger */
static ssize_t myseq_write(struct file *file, const char __user * buffer,
		size_t count, loff_t *ppos)
{
	add_one_record();
	return count;
}

static void *myseq_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&lock);
	return seq_list_start(&head, *pos);
}

static void *myseq_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &head, pos);
}

static void myseq_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&lock);
}

static int myseq_show(struct seq_file *m, void *p)
{
	struct ldata *data = list_entry(p, struct ldata, list);
	seq_printf(m, "value: %d\n", data->value);
	return 0;
}

static struct seq_operations myseq_ops = {
	.start = myseq_start,
	.next = myseq_next,
	.stop = myseq_stop,
	.show = myseq_show
};

static int myseq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &myseq_ops);
}

/* 都通过open函数来设置seq_operations */
/* for proc filesystem operations */
static const struct proc_ops proc_fops = {
	.proc_open = myseq_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
	.proc_write = myseq_write
};

/* for debug filesystem operations */
static const struct file_operations debug_fops = {
	.owner = THIS_MODULE,
	.open = myseq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = myseq_write
};

static void clean_all(struct list_head *head)
{
	struct ldata *data;

	while (!list_empty(head)) {
		data = list_entry(head->next, struct ldata, list);
		list_del(&data->list);
		kfree(data);
	}
}

static int seq_test_init(void)
{
	int i;
	mutex_init(&lock);
	INIT_LIST_HEAD(&head);

	/* init with 3 records */
	for (i = 0; i < 3; i++)
		add_one_record();

	/* create /proc/test */
	p_dir_name = proc_mkdir(TEST_DIR_NAME, NULL);
	if (p_dir_name)
		proc_create(TEST_NODE_NAME, 0644, p_dir_name, &proc_fops);

	/* create /sys/kernel/debug/test */
	d_dir_name = debugfs_create_dir(TEST_DIR_NAME, NULL);
	debugfs_create_file(TEST_NODE_NAME, 0644, d_dir_name, NULL,
				&debug_fops);

	return 0;
}

static void seq_test_exit(void)
{
	remove_proc_entry(TEST_NODE_NAME, p_dir_name);
	proc_remove(p_dir_name);
	debugfs_remove(d_dir_name);
	clean_all(&head);
}

module_init(seq_test_init);
module_exit(seq_test_exit);
MODULE_LICENSE("GPL");
