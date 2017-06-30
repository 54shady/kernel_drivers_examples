/* 参考内核里的驱动usbmouse.c */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

/* 自定义数据结构 */
struct usbmouse_key {
	char *usb_buf_vir; /* 虚拟地址 */
	dma_addr_t usb_buf_phys; /* 物理地址 */
	struct urb *uk_urb;
	struct input_dev *uk_input_dev;
	struct usb_device *usbdev;
};

static void usbmouse_as_key_irq(struct urb *urb)
{
	static unsigned char pre_val = 0;

	/* 从urb的context里获得到自定义数据 */
	struct usbmouse_key *u_key = urb->context;

	char *usb_buf_vir; /* 虚拟地址 */
	struct input_dev *uk_input_dev;

	usb_buf_vir = u_key->usb_buf_vir;
	uk_input_dev = u_key->uk_input_dev;

	/*
	 * 我所用的USB鼠标数据含义如下:
	 * usb_buf_vir[1] : bit[0]: 1左键按下 0左键松开
	 *				bit[1]: 2右键按下 0右键松开
	 *				bit[2]: 4中键按下 0中键松开
	 */
	if ((pre_val & (1<<0)) != (usb_buf_vir[1] & (1<<0)))
	{
		/* 左键发生了变化 */
		input_event(uk_input_dev, EV_KEY, KEY_L, (usb_buf_vir[1] & (1<<0)) ? 1 : 0);
		input_sync(uk_input_dev);
	}

	if ((pre_val & (1<<1)) != (usb_buf_vir[1] & (1<<1)))
	{
		/* 右键发生了变化 */
		input_event(uk_input_dev, EV_KEY, KEY_S, (usb_buf_vir[1] & (1<<1)) ? 1 : 0);
		input_sync(uk_input_dev);
	}

	if ((pre_val & (1<<2)) != (usb_buf_vir[1] & (1<<2)))
	{
		/* 中键发生了变化 */
		input_event(uk_input_dev, EV_KEY, KEY_ENTER, (usb_buf_vir[1] & (1<<2)) ? 1 : 0);
		input_sync(uk_input_dev);
	}

	pre_val = usb_buf_vir[1];

	/* 重新提交urb */
	usb_submit_urb(urb, GFP_KERNEL);
}

static int usbmouse_as_key_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	char *usb_buf_vir; /* 虚拟地址 */
	dma_addr_t usb_buf_phys; /* 物理地址 */
	/* 根据usb接口interface获得usb设备 */
	struct usb_device *dev = interface_to_usbdev(intf);

	/* host-side wrapper for one interface setting's parsed descriptors */
	struct usb_host_interface *h_intf;

	struct usb_endpoint_descriptor *endpoint;
	int pipe;
	struct input_dev *uk_input_dev;

	struct usbmouse_key *u_key;
	struct urb *uk_urb;
	static int len;

	/* 自定义数据结构 */
	u_key = kzalloc(sizeof(struct usbmouse_key), GFP_KERNEL);

	/*
	 * 获取除端点0之外的第一个端点描述, endpoint数组保存的是非端点0的所有端点
	 * 一般鼠标除了端点0外还会有一个中断类型的中断端点,所以也就是这里为什么取
	 * endpoint数组里的第一个端点
	 * 这个端点用于上报鼠标的数据
	 */
	h_intf = intf->cur_altsetting;
	endpoint = &h_intf->endpoint[0].desc;

	/* a. 分配一个input device */
	uk_input_dev = devm_input_allocate_device(&dev->dev);

	/* b. 设置 */
	/* b.1 能够产生哪类事件 */
	set_bit(EV_KEY, uk_input_dev->evbit);
	set_bit(EV_REP, uk_input_dev->evbit);

	/* b.2 能够产生哪些事件 */
	set_bit(KEY_L, uk_input_dev->keybit);
	set_bit(KEY_S, uk_input_dev->keybit);
	set_bit(KEY_ENTER, uk_input_dev->keybit);

	/* c. 注册 */
	if (input_register_device(uk_input_dev))
	{
		printk("input register device error\n");
		return -1;
	}

	/* d. 硬件相关操作 */
	/* 数据传输三要素: 源,目的,长度 */

	/* 源 : USB设备的某个端点 */
	/* pipe 包含usb设备地址,端点地址,端点方向 */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);

	/* 目的 */
	usb_buf_vir = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &usb_buf_phys);

	/* 数据长度 */
	len = endpoint->wMaxPacketSize;

	/* 使用三要素来构造urb */
	uk_urb = usb_alloc_urb(0, GFP_KERNEL);

	usb_fill_int_urb(uk_urb, dev, pipe, usb_buf_vir, len, usbmouse_as_key_irq, u_key, endpoint->bInterval);
	uk_urb->transfer_dma = usb_buf_phys;
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* 使用urb */
	usb_submit_urb(uk_urb, GFP_KERNEL);

	/* 将所有数据都关联到自定义数据结构 */
	u_key->usbdev = dev;
	u_key->uk_input_dev = uk_input_dev;
	u_key->uk_urb = uk_urb;
	u_key->usb_buf_vir = usb_buf_vir;
	u_key->usb_buf_phys = usb_buf_phys;

	/* 将自定义数据结构设置到interface的dev->driver_data */
	usb_set_intfdata(intf, u_key);

	printk("bcdUSB = 0x%x\n", dev->descriptor.bcdUSB);
	printk("Vid = 0x%x\n", dev->descriptor.idVendor);
	printk("Pid = 0x%x\n", dev->descriptor.idProduct);

	return 0;
}

static void usbmouse_as_key_disconnect(struct usb_interface *intf)
{
	/* 根据inteface 获取自定义数据结构 */
	struct usbmouse_key *u_key = usb_get_intfdata(intf);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	usb_kill_urb(u_key->uk_urb);
	usb_free_urb(u_key->uk_urb);

	usb_free_coherent(interface_to_usbdev(intf), 8, u_key->usb_buf_vir, u_key->usb_buf_phys);
	input_unregister_device(u_key->uk_input_dev);
	kfree(u_key);
}

static struct usb_device_id usbmouse_as_key_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usbmouse_as_key_id_table);

static struct usb_driver usbmouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usbmouse_as_key_probe,
	.disconnect	= usbmouse_as_key_disconnect,
	.id_table	= usbmouse_as_key_id_table,
};

module_usb_driver(usbmouse_as_key_driver);

MODULE_AUTHOR("M_O_Bz@163.com");
MODULE_DESCRIPTION("USB HID Boot Protocol mouse driver");
MODULE_LICENSE("GPL");
