#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

static LIST_HEAD(person_list);
struct person
{
	char name[20];
	struct list_head list;
};

/* add a person to the person list */
void add_person(const char *person_name)
{
	struct person *tmp;
	tmp = kmalloc(sizeof(struct person), GFP_KERNEL);
	if (NULL == tmp)
	{
		printk("error\n");
		return -1;
	}

	strcpy(tmp->name, person_name);
	list_add(&tmp->list, &person_list);
}

const char *players[] = {
	"jordan",
	"eminem",
	"shady"
};

void check_person_list(void)
{
	struct person *ps;
	list_for_each_entry(ps, &person_list, list) {
		printk("%s, %d, %s\n", __FUNCTION__, __LINE__, ps->name);
	}
}

static int kernel_list_init(void)
{
	int i;

	/* add player to the list */
	for (i = 0; i < sizeof(players) / sizeof(players[0]); i++)
		add_person(players[i]);

	check_person_list();
	return 0;
}

static void kernel_list_exit (void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(kernel_list_init);
module_exit(kernel_list_exit);
MODULE_LICENSE("GPL");
