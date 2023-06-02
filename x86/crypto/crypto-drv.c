#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/bitops.h>
#include <linux/pci.h>

#define DMA_TEST_DEMO
/* refcode: linux/drivers/net/ethernet/intel/e1000/e1000_main.c */
#define BAR_0 0
char e1000_driver_name[] = "mycrypto";
#define INTEL_E1000_ETHERNET_DEVICE(device_id) {\
	PCI_DEVICE(0x1111, device_id)}
static const struct pci_device_id e1000_pci_tbl[] = {
	INTEL_E1000_ETHERNET_DEVICE(0x2222),
	/* required last entry */
	{0,}
};

typedef enum tagIOField {
	ErrorCode = 0x00,
	State = 0x01,
	Command = 0x02,
	InterruptFlag = 0x03,
	DmaInAddress = 0x04,
	DmaInPagesCount = 0x08,
	DmaInSizeInBytes = 0x0c,
	DmaOutAddress = 0x10,
	DmaOutPagesCount = 0x14,
	DmaOutSizeInBytes = 0x18,
	MsiErrorFlag = 0x1c,
	MsiReadyFlag = 0x1d,
	MsiResetFlag = 0x1e,
	Unused = 0x1f,
} IoField;

#define NO2STR(n) case n: return #n
static const char *iofield2str(IoField io)
{
    switch (io)
	{
		NO2STR(ErrorCode);
		NO2STR(State);
		NO2STR(Command);
		NO2STR(InterruptFlag);
		NO2STR(DmaInAddress);
		NO2STR(DmaInPagesCount);
		NO2STR(DmaInSizeInBytes);
		NO2STR(DmaOutAddress);
		NO2STR(DmaOutPagesCount);
		NO2STR(DmaOutSizeInBytes);
		NO2STR(MsiErrorFlag);
		NO2STR(MsiReadyFlag);
		NO2STR(MsiResetFlag);
        default:
            return "UnknowIoFiled";
    }
}

int bit = 0; /* 0: byte, 1: word, 2: long */
int regIdx = 0;
int val = 0;
int bars;
void __iomem *hw_addr;
void *cpu_in_addr, *cpu_out_addr;
dma_addr_t dma_in_addr, dma_out_addr;

/* cat /sys/devices/pci0000:00/0000:00:03.0/mycrypto_debug */
static ssize_t mycrypto_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch (bit)
	{
		case 0:
		default:
			printk("%s = 0x%x\n", iofield2str(regIdx), readb(hw_addr + regIdx));
			break;
		case 1:
			printk("%s = 0x%x\n", iofield2str(regIdx), readw(hw_addr + regIdx));
			break;
		case 2:
			printk("%s = 0x%x\n", iofield2str(regIdx), readl(hw_addr + regIdx));
			break;
	}

    return 0;
}

/*
 * Enable interrupt flag to 2 using devmem
 * devmem 0xfebf1003 b 2
 *
 * for MIS#0 (LineBase INTx or signle MSI mode)
 * 1: Error INT
 * Enable Error INT flag
 * echo 0 3 1 > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 *
 * Trigger write ErrorCode
 * echo 0 0 0  > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 *
 * 2: Ready INT
 * Enable Ready INT
 * echo 0 3 2 > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 *
 * Trigger Encrypt
 * echo 0 2 2 > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 *
 * 3: Reset INT
 * Enable Reset INT
 * echo 0 3 3 > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 *
 * Trigger Encrypt
 * echo 0 2 2 > /sys/devices/pci0000\:00/0000\:00\:03.0/mycrypto_debug
 */
static ssize_t mycrypto_debug_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d %d %d", &bit, &regIdx, &val);
	printk("Write [0x%x]%s =  %d\n",  hw_addr + regIdx, iofield2str(regIdx), val);

	switch (bit)
	{
		case 0:
		default:
			/* byte write */
			writeb(val & 0xf, hw_addr + regIdx);
			break;
		case 1:
			/* word write */
			writew(val & 0xff, hw_addr + regIdx);
			break;
		case 2:
			/* long write */
			writel(val, hw_addr + regIdx);
			break;
	}

    return count;
}

static DEVICE_ATTR(mycrypto_debug, S_IRUGO | S_IWUSR, mycrypto_debug_show, mycrypto_debug_store);

static struct attribute *mycrypto_attrs[] = {
    &dev_attr_mycrypto_debug.attr,
    NULL,
};

static const struct attribute_group mycrypto_attr_group = {
    .attrs = mycrypto_attrs,
};

typedef enum tagCryptoDeviceMSI
{
	CryptoDevice_MsiZero = 0x00,
	CryptoDevice_MsiError = 0x01,
	CryptoDevice_MsiReady = 0x02,
	CryptoDevice_MsiReset = 0x03,
	CryptoDevice_MsiMax = 0x04,
} CryptoDeviceMSI;

typedef enum {
	CryptoDevice_NoError,
	CryptoDevice_DmaError,
	CryptoDevice_DeviceHasBennReseted,
	CryptoDevice_WriteIoError,
	CryptoDevice_InternalError
} CryptoDeviceErrorCode;

static const char *errno2errstr(CryptoDeviceErrorCode error)
{
    switch (error)
    {
		NO2STR(CryptoDevice_NoError);
		NO2STR(CryptoDevice_DmaError);
		NO2STR(CryptoDevice_DeviceHasBennReseted);
		NO2STR(CryptoDevice_WriteIoError);
		NO2STR(CryptoDevice_InternalError);
        default:
            return "UnknowError";
    }
}

static irqreturn_t crypto_irq_handler(int irq, void *data)
{
	int ret;

	ret = readb(hw_addr + InterruptFlag);
	/* read interrupt flag */
	printk("Interrupt(%d) is %d -> %s\n", irq, ret, ret ? "Enable" : "Disable");
	/* INTx */
	if (ret == CryptoDevice_MsiError)
	{
		printk("ErrorINT handled\n");
		printk("ErrorCode: %s\n", errno2errstr(readb(hw_addr + ErrorCode)));
	}
	if (ret == CryptoDevice_MsiReady)
		printk("ReadyINT handled\n");
	if (ret == CryptoDevice_MsiReset)
		printk("ResetINT handled\n");

	/* MSI#1,2,3 */
	//if (readb(hw_addr + MsiErrorFlag))
	//	printk("MsiErrorFlag is set\n");
	//if (readb(hw_addr + MsiReadyFlag))
	//	printk("MsiReadyFlag is set\n");
	//if (readb(hw_addr + MsiResetFlag))
	//	printk("MsiResetFlag is set\n");

	/* disable irq */
	disable_irq_nosync(irq);

	/* clear int */
	writeb(0, hw_addr + InterruptFlag);
	writeb(0, hw_addr + MsiErrorFlag);
	writeb(0, hw_addr + MsiReadyFlag);
	writeb(0, hw_addr + MsiResetFlag);

	/* enable irq */
	enable_irq(irq);

	return IRQ_HANDLED;
}

static int e1000_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err;

	bars = pci_select_bars(pdev, IORESOURCE_MEM);
	printk("bars = %d, %s, %d\n", bars, __FUNCTION__, __LINE__);
	err = pci_enable_device_mem(pdev);
	if (err)
	{
		printk("%s, %d\n", "Error*****", __LINE__);
		return err;
	}

	err = pci_request_selected_regions(pdev, bars, e1000_driver_name);
	if (err)
	{
		printk("%s, %d\n", "error", __LINE__);
		return err;
	}

	pci_set_master(pdev);
	err = pci_save_state(pdev);
	if (err)
	{
		printk("%s, %d\n", "error", __LINE__);
		return err;
	}

	hw_addr = pci_ioremap_bar(pdev, BAR_0);
	if (!hw_addr)
	{
		printk("%s, %d\n", "error", __LINE__);
		return err;
	}
	printk("hw_addr = 0x%x\n", hw_addr);
	printk("ErrorCode = 0x%x\n", readb(hw_addr + ErrorCode));
	printk("State = 0x%x\n", readb(hw_addr + State));
	printk("Command = 0x%x\n", readb(hw_addr + Command));
	printk("InterruptFlag = 0x%x\n", readb(hw_addr + InterruptFlag));

	printk("irq = %d\n", pdev->irq);
	err = request_irq(pdev->irq, crypto_irq_handler, IRQF_ONESHOT, "crypto-irq", NULL);
	//err = request_irq(pdev->irq, crypto_irq_handler, IRQF_PROBE_SHARED, "crypto-irq", NULL);
	if (err) {
		printk("Unable to allocate interrupt Error: %d\n", err);
	}

	/* sysfs for debug */
    err = sysfs_create_group(&pdev->dev.kobj, &mycrypto_attr_group);
    if (err) {
        printk("failed to create sysfs device attributes\n");
        return -1;
    }

#ifdef TEST_VIRT_TO_PHY
	struct aaaa {
		int a;
		char name[10];
	};

	struct aaaa *virt4a;
	unsigned long phy4a;

	virt4a = kmalloc(sizeof(struct aaaa), GFP_ATOMIC);
	if (!virt4a)
		return -1;
	virt4a->a = 911;
	strcpy(virt4a->name, "virt4a");
	phy4a = virt_to_phys(virt4a);
	printk("phy4a = 0x%lx, virt4a = 0x%lx\n", phy4a, virt4a);
	writel(phy4a, hw_addr + DmaInAddress);
#endif

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err)
	{
		printk("%s, %d\n", "error", __LINE__);
		return err;
	}

	/* driver using cpu_in_addr, pass the dma_in_addr/dma_out_addr to device */
	cpu_in_addr = dma_alloc_coherent(&pdev->dev, 4096, &dma_in_addr, GFP_KERNEL);
	if (!cpu_in_addr)
	{
		printk("Failed allocation memory for DMA\n");
		return -ENOMEM;
	}

#ifdef TEST_VIRT_TO_PHY
	//memcpy(cpu_in_addr, virt4a, 4096);

	struct aaaa a = {
		.a = 912,
		.name = "test-dma"
	};
	memcpy(cpu_in_addr, &a, sizeof(struct aaaa));
#endif

#ifdef DMA_TEST_DEMO
	struct aaaa {
		int a;
		char name[10];
	};
	/* setup data */
	struct aaaa a = {
		.a = 911,
		.name = "DmaIn"
	};

	/* 将需要传输的数据放入dma buffer */
	memcpy(cpu_in_addr, &a, sizeof(struct aaaa));
#endif

	printk("DmaInAddress = 0x%llx, cpu_in = 0x%x\n", dma_in_addr, cpu_in_addr);
	writel(dma_in_addr, hw_addr + DmaInAddress);
	writel(4096, hw_addr + DmaInSizeInBytes);
	writel(1, hw_addr + DmaInPagesCount);

	cpu_out_addr = dma_alloc_coherent(&pdev->dev, 4096, &dma_out_addr, GFP_KERNEL);
	if (!cpu_out_addr)
	{
		printk("Failed allocation memory for DMA\n");
		return -ENOMEM;
	}
	printk("DmaOutAddress = 0x%llx, cpu_out = 0x%x\n", dma_out_addr, cpu_out_addr);
	writel(dma_out_addr, hw_addr + DmaOutAddress);
	writel(4096, hw_addr + DmaOutSizeInBytes);
	writel(1, hw_addr + DmaOutPagesCount);

#ifdef DMA_TEST_DEMO
	/* 从dma buffer中取出数据 */
	struct aaaa *pa = cpu_out_addr;
	printk("Dma data: %d %s\n", pa->a, pa->name);
#endif

	return 0;
}

static void e1000_remove(struct pci_dev *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	pci_release_selected_regions(pdev, bars);
	iounmap(hw_addr);

	sysfs_remove_group(&pdev->dev.kobj, &mycrypto_attr_group);

	free_irq(pdev->irq, NULL);

	dma_free_coherent(&pdev->dev, 4096, cpu_in_addr, dma_in_addr);
	dma_free_coherent(&pdev->dev, 4096, cpu_out_addr, dma_out_addr);
}

static void e1000_shutdown(struct pci_dev *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static struct pci_driver e1000_driver = {
	.name     = e1000_driver_name,
	.id_table = e1000_pci_tbl,
	.probe    = e1000_probe,
	.remove   = e1000_remove,
	.shutdown = e1000_shutdown,
};

static int __init e1000_init_module(void)
{
	int ret;

	ret = pci_register_driver(&e1000_driver);
	return ret;
}
module_init(e1000_init_module);

static void __exit e1000_exit_module(void)
{
	pci_unregister_driver(&e1000_driver);
}
module_exit(e1000_exit_module);
MODULE_LICENSE("GPL");
