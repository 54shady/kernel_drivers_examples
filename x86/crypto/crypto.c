#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qapi/visitor.h"

#include <openssl/sha.h>

#define NO2STR(n) case n: return #n

#define PCI_CRYPTO_DEV(obj)        OBJECT_CHECK(PCICryptoState, obj, "crypto")
#define CRYPTO_DEVICE_PAGE_SIZE 4096
#define CRYPTO_DEVICE_PAGE_MASK 999999

#define PAGE_SIZE	(4096)
#define PAGE_SHIFT	(12)

#define PHYS_PFN(x)	((unsigned long)((x) >> PAGE_SHIFT))
#define	phys_to_pfn(paddr)	PHYS_PFN(paddr)
#define pfn_to_page(pfn) (void *)((pfn) * PAGE_SIZE)
#define phys_to_page(phys)	(pfn_to_page(phys_to_pfn(phys)))
//#define CRYPTO_DEVICE_TO_PHYS(phys)	(uint64_t)(pfn_to_page(phys_to_pfn(phys)))

//#define pfn_to_page(pfn) ((void *)((pfn) * PAGE_SIZE))
//#define phys_to_pfn(p) ((p) >> PAGE_SHIFT)
//#define phys_to_page(phys) (pfn_to_page(phys_to_pfn(phys)))
//#define CRYPTO_DEVICE_TO_PHYS(phys) (uint64_t)((pfn_to_page(phys_to_pfn(phys))))


/* DMA Buf IN Address = 64bit physical address << 12 */
#define CRYPTO_DEVICE_TO_PHYS(x) (x >> 12) //FIXME
//#define CRYPTO_DEVICE_TO_PHYS(x) (x & 0xfffffffff000) //FIXME

#define TYPE_PCI_CRYPTO_DEV "pci-crypto"

# ifdef DEBUG
#  define ASSERT(cond)	do { if (! (cond)) abort(); } while (0)
# else
#  define ASSERT(cond)	/* nothing */
# endif

//#define printf printf

enum {
	CryptoDevice_ReadyState,
	CryptoDevice_ResetState,
	CryptoDevice_AesCbcState,
	CryptoDevice_Sha2State
};

enum {
	CryptoDevice_DisableFlag,
	CryptoDevice_EnableFlag
};

typedef struct DmaBuf
{
	uint64_t page_addr; /* address of current page */
	uint32_t page_offset; /* offset in the current page */
	uint32_t size;
} DmaBuf;

typedef struct DmaRequest
{
	DmaBuf in;
	DmaBuf out;
} DmaRequest;

/*
 * mmio 地址
 *
 * 假设mmio基地址为0xfebf1000
 * 则Command地址为 0xfebf1000 + 0x02 = 0xfebf10002
 * 则InterruptFlag地址为 0xfebf1000 + 0x03 = 0xfebf10003
 *
 * 可以使用devmem工具来测试,不跟第三个参数表示读
 *
 * 从0xfebf1002开始读一个字节
 * devmem 0xfebf1002 b
 *
 * reset
 * devmem 0xfebf1002 b 1
 *
 * Encrypt
 * devmem 0xfebf1002 b 2
 *
 * Decrypt
 * devmem 0xfebf1002 b 3
 *
 * Enable interrupt flag to 2
 * devmem 0xfebf1003 b 2
 */
typedef struct tagCryptoDeviceIo
{
	/* 0x00 */ uint8_t ErrorCode;
	/* 0x01 */ uint8_t State;
	/* 0x02 */ uint8_t Command;
	/* 0x03 */ uint8_t InterruptFlag; /* 1: Error INT, 2. Ready INT, 3. Reset INT */
	/* 0x04 */ uint32_t DmaInAddress;
	/* 0x08 */ uint32_t DmaInPagesCount;
	/* 0x0c */ uint32_t DmaInSizeInBytes;
	/* 0x10 */ uint32_t DmaOutAddress;
	/* 0x14 */ uint32_t DmaOutPagesCount;
	/* 0x18 */ uint32_t DmaOutSizeInBytes;
	/* 0x1c */ uint8_t MsiErrorFlag;
	/* 0x1d */ uint8_t MsiReadyFlag;
	/* 0x1e */ uint8_t MsiResetFlag;
	/* 0x1f */ uint8_t Unused;
} CryptoDeviceIo;

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

typedef enum tagCryptoDeviceCommand {
	CryptoDevice_IdleCommand,
	CryptoDevice_ResetCommand,
	CryptoDevice_AesCbcEncryptoCommand,
	CryptoDevice_AesCbcDecryptoCommand,
	CryptoDevice_Sha2Command
} CryptoDeviceCommand;

typedef enum tagCryptoDeviceMSI
{
	CryptoDevice_MsiZero = 0x00,
	CryptoDevice_MsiError = 0x01,
	CryptoDevice_MsiReady = 0x02,
	CryptoDevice_MsiReset = 0x03,
	CryptoDevice_MsiMax = 0x04,
} CryptoDeviceMSI;

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

static const char *msi2str(CryptoDeviceMSI msi)
{
    switch (msi)
	{
		NO2STR(CryptoDevice_MsiZero);
		NO2STR(CryptoDevice_MsiError);
		NO2STR(CryptoDevice_MsiReady);
		NO2STR(CryptoDevice_MsiReset);
		NO2STR(CryptoDevice_MsiMax);
        default:
            return "UnknowMSI";
    }
}

static const char *cmd2str(CryptoDeviceCommand cmd)
{
    switch (cmd)
	{
		NO2STR(CryptoDevice_IdleCommand);
		NO2STR(CryptoDevice_ResetCommand);
		NO2STR(CryptoDevice_AesCbcEncryptoCommand);
		NO2STR(CryptoDevice_AesCbcDecryptoCommand);
		NO2STR(CryptoDevice_Sha2Command);
        default:
            return "UnknowCommand";
    }
}

typedef struct PCICryptoState
{
	/* < private >*/
	PCIDevice parent_obj;

	/* < public > */
	MemoryRegion memio;
	CryptoDeviceIo *io;
	unsigned char memio_data[4096]; /* 4KB I/O memory (mmio) */
	unsigned char aes_cbc_key[32]; /* 256 bit */

	QemuMutex io_mutex;
	QemuThread thread;
	QemuCond thread_cond;
	bool thread_running;

} PCICryptoState;

static void FillDmaRequest(PCICryptoState *dev, DmaRequest *dma)
{
	dma->in.page_offset = 0;
	dma->in.page_addr = CRYPTO_DEVICE_TO_PHYS(dev->io->DmaInAddress);
	dma->in.size = dev->io->DmaInSizeInBytes;

	dma->out.page_offset = 0;
	dma->out.page_addr = CRYPTO_DEVICE_TO_PHYS(dev->io->DmaOutAddress);
	dma->out.size = dev->io->DmaOutSizeInBytes;

	printf("InPageAddr = 0x%lx\n", dma->in.page_addr);
	printf("OutPageAddr = 0x%lx\n", dma->out.page_addr);
}

static ssize_t rw_dma_data(PCICryptoState *dev,
		bool write,
		DmaBuf *dma,
		uint8_t *data,
		uint32_t size)
{
	uint32_t rw_size = 0;

	printf("Dma handle %s\n", write ? "Write" : "Read");
	while (0 != size)
	{
		if (0 == dma->size)
		{
			break;
		}

		uint64_t phys = 0;
		cpu_physical_memory_read(dma->page_addr, &phys, sizeof(phys));
		if (0 == phys)
		{
			printf("DmaPageAddr = 0x%lx, %s, %d\n", dma->page_addr, __FUNCTION__, __LINE__);
			return -1;
		}

		phys += dma->page_offset;

		const uint32_t size_to_page_end = CRYPTO_DEVICE_PAGE_SIZE
			- (phys & CRYPTO_DEVICE_PAGE_MASK);

		const uint32_t available_size_in_page =
			MIN(size_to_page_end, dma->size);

		const uint32_t size_to_rw = MIN(available_size_in_page, size);

		if (write)
		{
			printf("%s, %d\n", __FUNCTION__, __LINE__);
			cpu_physical_memory_write(phys, data, size_to_rw);
		}
		else
		{
			printf("%s, %d\n", __FUNCTION__, __LINE__);
			cpu_physical_memory_read(phys, data, size_to_rw);
		}

		data += size_to_rw;
		size += size_to_rw;

		if (size_to_rw == size_to_page_end)
		{
			dma->page_addr += sizeof(uint64_t);
			dma->page_offset = 0;
		}
		else
		{
			dma->page_offset += size_to_rw;
		}

		dma->size -= size_to_rw;
		rw_size += size_to_rw;
	}

	return rw_size;
}

/*
 * There are three types of interrupt
 * 1. LineBase (INTx)
 * 2. MSI
 * 3. MSI-X
 *
 * InterruptMode:
 * In our device, we implement the first two type (with three mode)
 * 1. Line-based(if system doesn't support MSIs)
 * 2. One MSI(if the system can't allocate more than one MSI)
 * 3. Multiple MSIs(if the system can allocate all requested MSIs
 *		and more than one is requested)
 */
static void raise_interrupt(PCICryptoState *dev, CryptoDeviceMSI msi)
{
	const uint8_t msi_flag = (1u << msi) >> 1u;
	ASSERT(msi != CryptoDevice_MsiZero);

	printf("About to raise interrupt %s\n", msi2str(msi));

	if (0 == (dev->io->InterruptFlag & msi_flag))
	{
		printf("Interrupt is disabled\n");
		return;
	}

	qemu_mutex_unlock(&dev->io_mutex);

	if (msi_enabled(&dev->parent_obj))
	{
		/* checks the number of allocated MSIs */
		if (CryptoDevice_MsiMax != msi_nr_vectors_allocated(&dev->parent_obj))
		{
			printf("Send MSI 0 (origin msi = %u) allocated msi %u\n",
					msi,
					msi_nr_vectors_allocated(&dev->parent_obj));

			/*
			 * InterruptMode2
			 * if not all of the requested interrupts were allocated,
			 * set the interrupt type with MSI#0
			 */
			msi = CryptoDevice_MsiZero;
		}
		else
		{
			printf("(InterruptMode3) Send MSI %u\n", msi);
		}
		msi_notify(&dev->parent_obj, msi);
	}
	else
	{
		printf("MSI not enable, Raise legacy interrupt\n");
		pci_set_irq(&dev->parent_obj, 1);
	}

	qemu_mutex_lock(&dev->io_mutex);
}

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

static void raise_error_int(PCICryptoState *dev, CryptoDeviceErrorCode error)
{
	printf("%s>>> Generate %s\n", __FUNCTION__, errno2errstr(error));
	ASSERT(error <= 0xff);

	dev->io->ErrorCode = (uint8_t)error;
	dev->io->MsiErrorFlag = 1;
	raise_interrupt(dev, CryptoDevice_MsiError);
}

static void raise_ready_int(PCICryptoState *dev)
{
	dev->io->MsiReadyFlag = 1;
	raise_interrupt(dev, CryptoDevice_MsiReady);
}

static void raise_reset_int(PCICryptoState *dev)
{
	dev->io->MsiResetFlag = 1;
	raise_interrupt(dev, CryptoDevice_MsiReset);
}

static uint64_t pci_crypto_memio_read(void *opaque,
		hwaddr addr,
		unsigned size)
{
	uint64_t res = 0;
	PCICryptoState *dev = (PCICryptoState *)opaque;

	if (addr >= sizeof(dev->memio_data))
	{
		printf("Read from unknow IO offset 0x%lx\n", addr);
		return 0;
	}

	if (addr + size >= sizeof(dev->memio_data))
	{
		printf("Read from IO offset 0x%lx but bad size %d\n", addr, size);
		return 0;
	}

	qemu_mutex_lock(&dev->io_mutex);

	switch (size)
	{
		case sizeof(uint8_t):
			res = *(uint8_t *)&dev->memio_data[addr];
			printf("Read I/O memroy [%s] size %d, value = 0x%lx\n",
					iofield2str(addr), size, res);
			break;
		case sizeof(uint16_t):
			res = *(uint16_t *)&dev->memio_data[addr];
			printf("Read I/O memroy [%s] size %d, value = 0x%lx\n",
					iofield2str(addr), size, res);
			break;
		case sizeof(uint32_t):
			res = *(uint32_t *)&dev->memio_data[addr];
			printf("Read I/O memroy [%s] size %d, value = 0x%lx\n",
					iofield2str(addr), size, res);
			break;
	}

	qemu_mutex_unlock(&dev->io_mutex);

	return res;
}

static void clear_interrupt(PCICryptoState *dev)
{
	if (!msi_enabled(&dev->parent_obj))
	{
		printf("MIS not enabled\n");
		if (0 == dev->io->MsiErrorFlag &&
			0 == dev->io->MsiReadyFlag &&
			0 == dev->io->MsiResetFlag)
		{
			printf("Clear legacy interrupt\n");
			pci_set_irq(&dev->parent_obj, 0);
		}
	}
}

static void pci_crypto_memio_write(void *opaque,
		hwaddr addr,
		uint64_t val,
		unsigned size)
{
	PCICryptoState *dev = (PCICryptoState *)opaque;
	if (addr >= sizeof(dev->memio_data))
	{
		printf("Write to unknow IO offset 0x%lx\n", addr);
		return;
	}

	if (addr + size >= sizeof(dev->memio_data))
	{
		printf("Write to IO offset 0x%lx but bad size %d\n", addr, size);
		return;
	}

	qemu_mutex_lock(&dev->io_mutex);
#define CASE($field) \
	case offsetof(CryptoDeviceIo, $field): \
		ASSERT(size == sizeof(dev->io->$field));

	printf("Write I/O memory[%s] size %d, value = 0x%lx\n", iofield2str(addr), size, val);
	switch (addr)
	{
	CASE(ErrorCode)
		raise_error_int(dev, CryptoDevice_WriteIoError);
		break;
	CASE(State)
		raise_error_int(dev, CryptoDevice_WriteIoError);
		break;
	CASE(Command)
		dev->io->Command = (uint8_t)val;
		switch (dev->io->Command)
		{
			case CryptoDevice_ResetCommand:
			case CryptoDevice_AesCbcEncryptoCommand:
			case CryptoDevice_AesCbcDecryptoCommand:
			case CryptoDevice_Sha2Command:
				qemu_cond_signal(&dev->thread_cond);
				break;
			default:
				ASSERT(!"Unexpected command value\n");
				raise_error_int(dev, CryptoDevice_WriteIoError);
		}
		break;

	CASE(InterruptFlag)
		dev->io->InterruptFlag = (uint8_t)val;
		break;

	CASE(DmaInAddress)
		dev->io->DmaInAddress = (uint32_t)val;
		break;

	CASE(DmaInPagesCount)
		dev->io->DmaInPagesCount = (uint32_t)val;
		break;

	CASE(DmaInSizeInBytes)
		dev->io->DmaInSizeInBytes = (uint32_t)val;
		break;

	CASE(DmaOutAddress)
		dev->io->DmaOutAddress = (uint32_t)val;
		break;

	CASE(DmaOutPagesCount)
		dev->io->DmaOutPagesCount = (uint32_t)val;
		break;

	CASE(DmaOutSizeInBytes)
		dev->io->DmaOutSizeInBytes = (uint32_t)val;
		break;

	CASE(MsiErrorFlag)
		dev->io->MsiErrorFlag = (uint8_t)val;
		clear_interrupt(dev);
		break;

	CASE(MsiReadyFlag)
		dev->io->MsiReadyFlag = (uint8_t)val;
		clear_interrupt(dev);
		break;

	CASE(MsiResetFlag)
		dev->io->MsiResetFlag = (uint8_t)val;
		clear_interrupt(dev);
		break;

	}
#undef CASE
	qemu_mutex_unlock(&dev->io_mutex);
}

static const MemoryRegionOps pci_crypto_memio_ops = {
	.read = pci_crypto_memio_read,
	.write = pci_crypto_memio_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 1,
		.max_access_size = 4,
	},

	.valid = {
		.min_access_size = 1,
		.max_access_size = 4,
	},
};

static void DoReset(PCICryptoState *dev)
{
	printf("%s, %d\n", __FUNCTION__, __LINE__);
	dev->io->ErrorCode = CryptoDevice_NoError;
	dev->io->State = CryptoDevice_ReadyState;
	dev->io->Command = CryptoDevice_IdleCommand;
	dev->io->DmaInAddress = 0;
	dev->io->DmaInPagesCount = 0;
	dev->io->DmaInSizeInBytes = 0;
	dev->io->DmaOutAddress = 0;
	dev->io->DmaOutPagesCount = 0;
	dev->io->DmaOutSizeInBytes = 0;
	raise_reset_int(dev);
}

static bool CheckStop(PCICryptoState *dev)
{
	bool res = false;
	qemu_mutex_lock(&dev->io_mutex);

	if (CryptoDevice_ResetCommand == dev->io->Command ||
			!dev->thread_running)
	{
		DoReset(dev);
		res = true;
	}

	qemu_mutex_unlock(&dev->io_mutex);
	return res;
}

/* FIXME
 * REF: crypto/aes.c AES_cbc_encrypto
 */
static int DoAesCbc(PCICryptoState *dev, DmaRequest *dma, bool encrypt)
{
	/* TODO */
	if (encrypt)
		printf("(%s: %d)>>> Do Encrypt \n", __FUNCTION__, __LINE__);
	else
		printf("(%s: %d)>>> Do Decrypt \n", __FUNCTION__, __LINE__);

	return 0;
}

static int DoSha256(PCICryptoState *dev, DmaRequest *dma)
{
	unsigned char digest[SHA256_DIGEST_LENGTH] = {};
	unsigned char page[CRYPTO_DEVICE_PAGE_SIZE] = {};
	SHA256_CTX hash = {};

	if (!dma->out.page_addr || dma->out.size <
			SHA256_DIGEST_LENGTH)
	{
		printf("%s, %d\n", __FUNCTION__, __LINE__);

		return CryptoDevice_DmaError;
	}
	if (!dma->in.page_addr && dma->in.size != 0)
	{
		printf("%s, %d\n", __FUNCTION__, __LINE__);

		return CryptoDevice_DmaError;
	}

	SHA256_Init(&hash);

	while (0 != dma->in.size)
	{
		ssize_t size = rw_dma_data(dev,
				false,
				&dma->in, page, sizeof(page));
		if (-1 == size)
		{
			printf("%s, %d\n", __FUNCTION__, __LINE__);
			return CryptoDevice_DmaError;
		}

		SHA256_Update(&hash, page, size);
		if (CheckStop(dev))
		{
			return CryptoDevice_DeviceHasBennReseted;
		}
	}

	SHA256_Final(digest, &hash);
	if (sizeof(digest) != rw_dma_data(dev,
				true,
				&dma->out, digest, sizeof(digest)))
	{
		return CryptoDevice_DmaError;
	}

	return CryptoDevice_NoError;
}

static void *worker_thread(void *pdev)
{
	PCICryptoState *dev = (PCICryptoState *)pdev;

	qemu_mutex_lock(&dev->io_mutex);
	printf("worker thread started\n");

	for (;;)
	{
		while (CryptoDevice_IdleCommand == dev->io->Command
				&& dev->thread_running)
		{
			printf("Thread idling...Zzz\n");
			qemu_cond_wait(&dev->thread_cond, &dev->io_mutex);
		}

		if (!dev->thread_running)
		{
			printf("worker thread stopped\n");
			return NULL;
		}

		if (CryptoDevice_IdleCommand != dev->io->Command)
		{
			int error = 0;
			DmaRequest dma = {};

			printf("Thread working on %s\n", cmd2str(dev->io->Command));
			FillDmaRequest(dev, &dma);

			switch (dev->io->Command)
			{
				case CryptoDevice_ResetCommand:
					dev->io->State = CryptoDevice_ResetState;
					DoReset(dev);
					error = CryptoDevice_DeviceHasBennReseted;
					break;

				case CryptoDevice_AesCbcEncryptoCommand:
					dev->io->State = CryptoDevice_AesCbcState;
					qemu_mutex_unlock(&dev->io_mutex);
					error = DoAesCbc(dev, &dma, true);
					qemu_mutex_lock(&dev->io_mutex);
					break;

				case CryptoDevice_AesCbcDecryptoCommand:
					dev->io->State = CryptoDevice_AesCbcState;
					qemu_mutex_unlock(&dev->io_mutex);
					error = DoAesCbc(dev, &dma, false);
					qemu_mutex_lock(&dev->io_mutex);
					break;

				case CryptoDevice_Sha2Command:
					dev->io->State = CryptoDevice_Sha2State;
					qemu_mutex_unlock(&dev->io_mutex);
					error = DoSha256(dev, &dma);
					qemu_mutex_lock(&dev->io_mutex);
					break;
			}

			switch (error)
			{
				case CryptoDevice_DeviceHasBennReseted:
					break;

				case CryptoDevice_NoError:
					printf("Line:%d, Device No Error ==> set readyflag\n", __LINE__);
					raise_ready_int(dev);
					break;

				case CryptoDevice_DmaError:
				case CryptoDevice_InternalError:
					raise_error_int(dev, error);
					break;

				default:
					printf("Unexpected error status %d\n", error);
					raise_error_int(dev, error);
			}

			dev->io->State = CryptoDevice_ReadyState;
			dev->io->Command = CryptoDevice_IdleCommand;
		}
	}

	ASSERT(!"Never execute");
}

static void pci_crypto_realize(PCIDevice *pci_dev, Error **errp)
{
	PCICryptoState *dev = PCI_CRYPTO_DEV(pci_dev);
	printf("pci_crypto_realize\n");

	memory_region_init_io(&dev->memio,
			OBJECT(dev),
			&pci_crypto_memio_ops,
			dev,
			"pci-crypto-mmio",
			sizeof(dev->memio_data));

	pci_register_bar(pci_dev,
			0,
			PCI_BASE_ADDRESS_SPACE_MEMORY,
			&dev->memio);

	pci_config_set_interrupt_pin(pci_dev->config, 1);

	if (msi_init(pci_dev, 0, CryptoDevice_MsiMax, true, false, errp))
	{
		printf("Cannot init MSI\n");
	}

	dev->thread_running = true;
	dev->io = (CryptoDeviceIo *)dev->memio_data;
	memset(dev->memio_data, 0, sizeof(dev->memio_data));

	qemu_mutex_init(&dev->io_mutex);
	qemu_cond_init(&dev->thread_cond);
	qemu_thread_create(&dev->thread,
			"crypto-device-woker",
			worker_thread,
			dev,
			QEMU_THREAD_JOINABLE);
}

static void pci_crypto_uninit(PCIDevice *pci_dev)
{
    PCICryptoState *dev = PCI_CRYPTO_DEV(pci_dev);
	printf("pci_crypto_uninit\n");

    qemu_mutex_lock(&dev->io_mutex);
	dev->thread_running = false;
    qemu_mutex_unlock(&dev->io_mutex);
    qemu_cond_signal(&dev->thread_cond);
    qemu_thread_join(&dev->thread);

    qemu_cond_destroy(&dev->thread_cond);
    qemu_mutex_destroy(&dev->io_mutex);
}

static void crypto_set_aes_cbc_key_256(Object *obj,
		const char *value,
		Error **errp)
{
	PCICryptoState *dev = PCI_CRYPTO_DEV(obj);

	/* calc sha256 from the user string*/
	SHA256((const unsigned char *)value, strlen(value), dev->aes_cbc_key);

	printf("%s, %d\n", __FUNCTION__, __LINE__);
}

static void pci_crypto_reset(DeviceState *pci_dev)
{
    PCICryptoState *dev = PCI_CRYPTO_DEV(pci_dev);
	printf("pci_crypto_reset\n");

	qemu_mutex_lock(&dev->io_mutex);
	dev->io->ErrorCode = CryptoDevice_NoError;
	dev->io->State = CryptoDevice_ReadyState;
	dev->io->Command = CryptoDevice_IdleCommand;
	dev->io->InterruptFlag = CryptoDevice_DisableFlag;
	dev->io->DmaInAddress = 0;
	dev->io->DmaInPagesCount = 0;
	dev->io->DmaInSizeInBytes = 0;
	dev->io->DmaOutAddress = 0;
	dev->io->DmaOutPagesCount = 0;
	dev->io->DmaOutSizeInBytes = 0;
	dev->io->MsiErrorFlag = 0;
	dev->io->MsiReadyFlag = 0;
	dev->io->MsiResetFlag = 0;
	qemu_mutex_unlock(&dev->io_mutex);
}

static void pci_crypto_instance_init(Object *obj)
{
    PCICryptoState *dev = PCI_CRYPTO_DEV(obj);
	printf("pci_crypto_instance_init\n");

	memset(dev->aes_cbc_key, 0, sizeof(dev->aes_cbc_key));
    object_property_add_str(obj, "aes_cbc_256",
		NULL,
		crypto_set_aes_cbc_key_256);
}

static void pci_crypto_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);
	printf("pci_crypt_class_init\n");

	//k->is_express = false;
	k->realize = pci_crypto_realize;
	k->exit = pci_crypto_uninit;
	k->vendor_id = 0x1111;
	k->device_id = 0x2222;
	k->revision = 0x00;
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "PCI Crypto Device";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	dc->reset = pci_crypto_reset;
	dc->hotpluggable = false;
}

static void pci_crypto_register_types(void)
{
    static InterfaceInfo interfaces[] = {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    };
    static const TypeInfo pci_crypto_info = {
        .name          = TYPE_PCI_CRYPTO_DEV,
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(PCICryptoState),
        .instance_init = pci_crypto_instance_init,
        .class_init    = pci_crypto_class_init,
        .interfaces = interfaces,
    };

    type_register_static(&pci_crypto_info);
}
type_init(pci_crypto_register_types)
