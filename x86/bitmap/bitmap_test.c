#include <linux/init.h>
#include <linux/module.h>

/* https://www.cnblogs.com/schips/p/10674687.html */

#define BITMAP_LEN 10

#define USING_UNLONG
#ifdef USING_UNLONG
unsigned long mybm[1];
#else
DECLARE_BITMAP(mybm, BITMAP_LEN);
#endif

static int bitmap_test_init(void)
{
	int bit;

	/* architecture specifiy API */
	/*
	 * non atomic version
	 * include/asm-generic/bitops/instrumented-non-atomic.h
	 */
	__set_bit(1, mybm);
	/*
	 * arch___set_bit(nr, addr);
	 * asm volatile(__ASM_SIZE(bts) " %1,%0" : : ADDR, "Ir" (nr) : "memory");
	 */
	if (!test_bit(1, mybm))
		printk(KERN_ALERT" bit 1 tested not set\n");
	else
		printk(KERN_ALERT" bit 1 tested set\n");

	__clear_bit(1, mybm);
	if (!test_bit(1, mybm))
		printk(KERN_ALERT" bit 1 tested not set\n");
	else
		printk(KERN_ALERT" bit 1 tested set\n");

	/*
	 * atomic version
	 * include/asm-generic/bitops/atomic.h
	 * arch/x86/boot/bitops.h
	 */
	set_bit(2, mybm);
	if (!test_bit(2, mybm))
		printk(KERN_ALERT" bit 2 tested not set\n");
	else
		printk(KERN_ALERT" bit 2 tested set\n");

	clear_bit(2, mybm);
	if (!test_bit(2, mybm))
		printk(KERN_ALERT" bit 2 tested not set\n");
	else
		printk(KERN_ALERT" bit 2 tested set\n");

	/* Generic API */
	bitmap_zero(mybm, BITMAP_LEN);
	if (!test_bit(3, mybm))
		printk(KERN_ALERT" bit 3 tested not set\n");
	else
		printk(KERN_ALERT" bit 3 tested set\n");

	bitmap_fill(mybm, BITMAP_LEN);
	if (!test_bit(3, mybm))
		printk(KERN_ALERT" bit 3 tested not set\n");
	else
		printk(KERN_ALERT" bit 3 tested set\n");

	/* clear all before test */
	bitmap_zero(mybm, BITMAP_LEN);
	set_bit(4, mybm);
	set_bit(8, mybm);
	set_bit(9, mybm);
	/* iterator all the setted bit */
	for_each_set_bit(bit, mybm, BITMAP_LEN)
		printk(KERN_ALERT" %d\n", bit);
	printk(KERN_ALERT"===========\n");

	/* iterator all the no setted bit */
	for_each_clear_bit(bit, mybm, BITMAP_LEN)
		printk(KERN_ALERT" %d\n", bit);
	printk(KERN_ALERT"===========\n");

	printk(KERN_ALERT"BitMap test Done\n");

	return 0;
}

static void bitmap_test_exit(void)
{
	printk(KERN_ALERT"BitMap test Exit\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(bitmap_test_init);
module_exit(bitmap_test_exit);
