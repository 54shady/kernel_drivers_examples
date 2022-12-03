#ifndef _SKELETON_H_
#define _SKELETON_H_

/*
 * Describe a pin
 * gpio_pin : gpio pin number
 * name : gpio name
 */
struct pin_desc {
	int gpio_pin;
	int gpio_active_flag;
	char name[20];
};

/* skeleton gpios, pdata or chip */
struct skleton_chip {
	int pin_name_index;
	int value;

	/* a struct point array which contain of gpio_nr struct point */
	struct pin_desc **pd;
};

/* equal to dts pin name which is low case in pcb file */
const char *dts_name[] = {
	"pin_a_name",
	"pin_b_name",
	"pin_c_name",
	"pin_d_name",
	"pin_e_name",
	"pin_f_name",
};

#endif
