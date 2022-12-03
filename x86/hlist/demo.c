#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/* refercence with kernel module zd1201 */
/* https://zhuanlan.zhihu.com/p/82375193 */
struct AAAAA {
	/* hash table implement in list address */
	struct hlist_head nodelist;
};

struct AAAAA_node {
	struct hlist_node fnode;
	char name[20];
};

struct AAAAA *module5a;
static int hlist_test_init(void)
{
	struct AAAAA_node *node1;
	struct AAAAA_node *node2;
	struct AAAAA_node *pos;

	/* another hlist_node to use as temporary storage */
	struct hlist_node *tmp_node;

	module5a = kmalloc(sizeof(struct AAAAA), GFP_ATOMIC);
	if (!module5a)
		return -1;

	node1 = kmalloc(sizeof(*node1), GFP_ATOMIC);
	if (!node1)
		return -1;
	strcpy(node1->name, "Node1");

	node2 = kmalloc(sizeof(*node2), GFP_ATOMIC);
	if (!node2)
		return -1;
	strcpy(node2->name, "Node2");

	/* init hash table */
	INIT_HLIST_HEAD(&module5a->nodelist);

	/* add node to hash table */
	hlist_add_head(&node1->fnode, &module5a->nodelist);
	hlist_add_head(&node2->fnode, &module5a->nodelist);

	hlist_for_each_entry_safe(pos, tmp_node, &module5a->nodelist, fnode) {
		printk(KERN_ALERT"Iter node : %s\n", pos->name);
	}

	return 0;
}

static void hlist_test_exit(void)
{
	struct AAAAA_node *pos;
	struct hlist_node *tmp_node;

	hlist_for_each_entry_safe(pos, tmp_node, &module5a->nodelist, fnode) {
		printk(KERN_ALERT"Delete node : %s\n", pos->name);
		hlist_del_init(&pos->fnode);
		kfree(pos);
	}
	kfree(module5a);
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(hlist_test_init);
module_exit(hlist_test_exit);
