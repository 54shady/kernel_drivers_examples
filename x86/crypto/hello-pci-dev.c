#include "qemu/osdep.h"
#include "hw/pci/pci.h"

typedef struct PCIHelloDevState {
    PCIDevice parent_obj;

    /* for PIO */
    MemoryRegion io;

    /* for MMIO */
    MemoryRegion mmio;

    /* irq used */
    qemu_irq irq;

    /* dma buf size */
    unsigned int dma_size;

    /* buffer copied with the dma operation on RAM */
    char *dma_buf;

    /* did we throw an interrupt ? */
    int threw_irq;

    /* id of the device, writable */
    int id;
} PCIHelloDevState;

#define TYPE_PCI_HELLO_DEV "pci-hellodev"
#define PCI_HELLO_DEV(obj)     OBJECT_CHECK(PCIHelloDevState, (obj), TYPE_PCI_HELLO_DEV)
/* sizes must be power of 2 in PCI */
#define HELLO_IO_SIZE 1<<4 /* 16 byte */
#define HELLO_MMIO_SIZE 1<<6 /* 64 byte */

static void hello_iowrite(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    int i;
    PCIHelloDevState *d = (PCIHelloDevState *) opaque;
    PCIDevice *pci_dev = (PCIDevice *) opaque;

    printf("PIO Write : addr=%x, value=%lu, size=%d\n", (unsigned) addr, value, size);

    switch (addr) {
        case 0:
            if (value) {
                /* throw an interrupt */
                printf("irq assert\n");
                d->threw_irq = 1;
                pci_irq_assert(pci_dev);
            } else {
                /*  ack interrupt */
                printf("irq deassert\n");
                pci_irq_deassert(pci_dev);
                d->threw_irq = 0;
            }
            break;
        case 4:
            /* throw a random DMA */
            for ( i = 0; i < d->dma_size; ++i)
                d->dma_buf[i] = rand();

			/*
			 * code below will cause memory corruption
			 *
			 * add below kernel paramter in grub to avoid the memory corruption check
			 * check Documentation/admin-guide/kernel-parameters.rst
			 * or unconfig CONFIG_X86_BOOTPARAM_MEMORY_CORRUPTION_CHECK
			 * memory_corruption_check=0
			 */
            //cpu_physical_memory_write(value, (void *) d->dma_buf, d->dma_size);
            break;
        default:
            printf("Io not used\n");
			break;
    }
}

static uint64_t hello_ioread(void *opaque, hwaddr addr, unsigned size)
{
	int ret = 0;

    PCIHelloDevState *d = (PCIHelloDevState *) opaque;
    printf("PIO Read : addr =%x, size=%d\n", (unsigned) addr, size);

    switch (addr) {
        case 0:
            /* irq status */
            ret = d->threw_irq;
            break;
        default:
            printf("Io not used\n");
            ret = 0x0;
			break;
    }

	return ret;
}

static uint64_t hello_mmioread(void *opaque, hwaddr addr, unsigned size)
{
	int ret = 0;

    PCIHelloDevState *d = (PCIHelloDevState *) opaque;

    printf("MMIO Read: addr =%x, size=%d\n",(unsigned)  addr, size);

    switch (addr) {
        case 0:
            /* also irq status */
            printf("read irq_status\n");
            ret = d->threw_irq;
            break;
        case 4:
            /* Id of the device */
            printf("read id\n");
            ret = d->id;
            break;
        default:
            printf("MMIO not used\n");
			break;
    }

	return ret;
}

static void hello_mmiowrite(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    PCIHelloDevState *d = (PCIHelloDevState *) opaque;

    printf("MMIO write: addr=%x, value=%lx, size=%d\n",(unsigned)  addr, value, size);

    switch (addr) {
        case 4:
            /* change the id */
			printf("write id\n");
            d->id = value;
            break;
        default:
            printf("MMIO not writable or not used\n");
			break;
    }
}

/*
 * Callbacks called when the Memory Region
 * representing the MMIO space is accessed.
 */
static const MemoryRegionOps hello_mmio_ops = {
    .read = hello_mmioread,
    .write = hello_mmiowrite,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/*
 * Callbacks called when the Memory Region
 * representing the PIO space is accessed.
 */
static const MemoryRegionOps hello_io_ops = {
    .read = hello_ioread,
    .write = hello_iowrite,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

/* Callbacks for MMIO and PIO regions are registered here */
static void hello_io_setup(PCIHelloDevState *d)
{
    memory_region_init_io(&d->mmio, OBJECT(d), &hello_mmio_ops, d, "hello_mmio", HELLO_MMIO_SIZE);
    memory_region_init_io(&d->io, OBJECT(d), &hello_io_ops, d, "hello_io", HELLO_IO_SIZE);
}

/* When device is loaded */
static void pci_hellodev_realize(PCIDevice *pci_dev, Error **errp)
{
    uint8_t *pci_conf;

    /* init the internal state of the device */
    PCIHelloDevState *d = PCI_HELLO_DEV(pci_dev);

    d->dma_size = 0x1ffff * sizeof(char);
    d->dma_buf = malloc(d->dma_size);
    d->id = 0x1337;
    d->threw_irq = 0;

    /*
	 * create the memory region representing the MMIO and PIO
     * of the device
     */
    hello_io_setup(d);

    /*
     * See linux device driver (Edition 3) for the definition of a bar
     * in the PCI bus.
     */
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_IO, &d->io);
    pci_register_bar(pci_dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);

    pci_conf = pci_dev->config;

    /*
	 * also in ldd, a pci device has 4 pin for interrupt here we use pin B.
	 * lspci -vv
     */
    pci_conf[PCI_INTERRUPT_PIN] = 0x02;

    /* this device support interrupt */
    //d->irq = pci_allocate_irq(pci_dev);
    printf("Hello World loaded\n");
}

/*
 * When device is unloaded
 * Can be useful for hot(un)plugging
 */
static void pci_hellodev_uninit(PCIDevice *dev)
{
    PCIHelloDevState *d = (PCIHelloDevState *) dev;
    free(d->dma_buf);
    printf("Good bye World unloaded\n");
}

static void qdev_pci_hellodev_reset(DeviceState *dev)
{
    printf("Reset World\n");
}

/*
 * Called when the device is defined
 * PCI configuration is defined here
 * We inherit from PCIDeviceClass
 * Also see ldd for the meaning of the different args
 */
static void pci_hellodev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
    k->realize = pci_hellodev_realize;
    k->exit = pci_hellodev_uninit;
    /* this identify our device */
    k->vendor_id = 0x1337;
    k->device_id = 0x0001;
    k->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    k->revision  = 0x00;
    dc->desc = "PCI Hello World";
    dc->reset = qdev_pci_hellodev_reset;
}

/*
 * Contains all the informations of the device
 * we are creating.
 * class_init will be called when we are defining
 * our device.
 */
static InterfaceInfo interfaces[] = {
	{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
	{ },
};

static const TypeInfo pci_hello_info = {
    .name           = TYPE_PCI_HELLO_DEV,
    .parent         = TYPE_PCI_DEVICE,
    .instance_size  = sizeof(PCIHelloDevState),
    .class_init     = pci_hellodev_class_init,
	.interfaces = interfaces,
};

/*
 * function called before the qemu main
 * it will define our device
 */
static void pci_hello_register_types(void)
{
    type_register_static(&pci_hello_info);
}

/* macro actually defining our device and registering it in qemu*/
type_init(pci_hello_register_types);
