#include <linux/module.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#define PCI_TIC_VENDOR 0x1337
#define PCI_TIC_DEVICE 0x0001
#define PCI_TIC_SUBVENDOR 0x1af4
#define PCI_TIC_SUBDEVICE 0x1100

#define MAX_TIC_MAPS 1
#define MAX_TIC_PORT_REGIONS 1
static struct pci_driver hello_pci;
struct hello_pci_mem {
    const char *name;
    void __iomem *start;
    unsigned long size;
};

struct hello_pci_io {
    const char *name;
    unsigned long start;
    unsigned long size;
};

struct hello_pci_info {
    struct hello_pci_mem mem[MAX_TIC_MAPS];
    struct hello_pci_io port[MAX_TIC_PORT_REGIONS];
    u8 irq;
};

static irqreturn_t hello_pci_handler(int irq, void *dev_info)
{
    struct hello_pci_info *info = (struct hello_pci_info *) dev_info;

    pr_alert("IRQ %d handled\n", irq);

    /* is it our device throwing an interrupt ? */
    if (inl(info->port[0].start)) {
        /* deassert it */
        outl(0, info->port[0].start );
        return IRQ_HANDLED;
    } else {
        /* not mine, we have done nothing */
        return IRQ_NONE;
    }
}

static int hello_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    struct hello_pci_info *info;

    info = kzalloc(sizeof(struct hello_pci_info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;

    if (pci_enable_device(dev))
        goto out_free;

    pr_alert("hello_pci enabled device\n");

    if (pci_request_regions(dev, "hello_pci"))
        goto out_disable;

    pr_alert("hello_pci requested regions\n");

    /* BAR 0 has IO */
    info->port[0].name = "hello_pci-io";
    info->port[0].start = pci_resource_start(dev, 0);
    info->port[0].size = pci_resource_len(dev, 0);

    /* BAR 1 has MMIO */
    info->mem[0].name = "hello_pci-mmio";
    info->mem[0].start = pci_ioremap_bar(dev, 1);
    info->mem[0].size = pci_resource_len(dev, 1);
    if (!info->mem[0].start)
        goto out_unrequest;

    pr_alert("remaped addr for kernel uses\n");

    /* get device irq number */
    if (pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &info->irq))
        goto out_iounmap;

    /* request irq */
    if (devm_request_irq(&dev->dev, info->irq, hello_pci_handler, IRQF_SHARED, hello_pci.name, (void *) info))
        goto out_iounmap;

    /*
	 * get a mmio reg value and change it
	 * access mmio via ioread/iowrite
	 */
    pr_alert("device id=%x\n", ioread32(info->mem[0].start + 4));
    iowrite32(0x4567, info->mem[0].start + 4);
    pr_alert("modified device id=%x\n", ioread32(info->mem[0].start + 4));

    /*
	 * access pio via in/out
	 *
	 * assert an irq
	 */
    outb(1, info->port[0].start);
    /* try dma without iommu */
    outl(1, info->port[0].start + 4);

    pci_set_drvdata(dev, info);

    return 0;

out_iounmap:
    pr_alert("tic:probe_out:iounmap");
    iounmap(info->mem[0].start);
out_unrequest:
    pr_alert("tic:probe_out_unrequest\n");
    pci_release_regions(dev);
out_disable:
    pr_alert("tic:probe_out_disable\n");
    pci_disable_device(dev);
out_free:
    pr_alert("tic:probe_out_free\n");
    kfree(info);
    return -ENODEV;
}

static void hello_pci_remove(struct pci_dev *dev)
{
    /* we get back the driver data we store in
     * the pci_dev struct */
    struct hello_pci_info *info = pci_get_drvdata(dev);

    /* let's clean a little */
    pci_release_regions(dev);
    pci_disable_device(dev);
    iounmap(info->mem[0].start);

    kfree(info);
}

/* vendor and device (+ subdevice and subvendor)
 * identifies a device we support
 */
static struct pci_device_id hello_pci_ids[] = {
    { PCI_DEVICE(PCI_TIC_VENDOR, PCI_TIC_DEVICE) },
    { 0, },
};

/* id_table describe the device this driver support
 * probe is called when a device we support exist and
 * when we are chosen to drive it.
 * remove is called when the driver is unloaded or
 * when the device disappears
 */
static struct pci_driver hello_pci = {
    .name = "hello_pci",
    .id_table = hello_pci_ids,
    .probe = hello_pci_probe,
    .remove = hello_pci_remove,
};

/* register driver in kernel pci framework */
module_pci_driver(hello_pci);
MODULE_DEVICE_TABLE(pci, hello_pci_ids);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hello pci driver demo");
MODULE_AUTHOR("zeroway");
