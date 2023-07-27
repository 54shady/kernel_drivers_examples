#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include <linux/virtio_config.h>
#include "virtio_mini.h"

static int virtio_mini_open(struct inode *inode, struct file *file)
{
    struct virtio_mini_device *vmini = PDE_DATA(inode);

    file->private_data = vmini;

    return 0;
}

/* cat /proc/virtio-mini-0 */
static ssize_t virtio_mini_read(struct file *fil, char *buf, size_t count, loff_t *offp)
{
    struct virtio_mini_device *vmini = fil->private_data;
	char *rcv_buf;
    struct scatterlist sg;
	unsigned long res;
    struct virtqueue *vq = vmini->queues[VIRTIO_MINI_VQ_RX].vq;

    if (vmini->buffers < 1)
	{
        printk(KERN_INFO "all buffers read!");
        return 0;
    }

    rcv_buf = kzalloc(vmini->buf_lens[vmini->buffers - 1], GFP_KERNEL);
    if (!rcv_buf)
	{
        return ENOMEM;
    }

    sg_init_one(&sg, rcv_buf, vmini->buf_lens[vmini->buffers - 1]);
    virtqueue_add_inbuf(vq, &sg, 1, rcv_buf, GFP_KERNEL);
    virtqueue_kick(vq);

    wait_for_completion(&vmini->data_ready);
    res = copy_to_user(buf, vmini->read_data, vmini->buf_lens[vmini->buffers]);
    if (res != 0)
	{
        printk(KERN_INFO "Could not read %lu bytes!", res);
        /* update length to actual number of bytes read */
        vmini->buf_lens[vmini->buffers] = vmini->buf_lens[vmini->buffers] - res;
    }

    kfree(rcv_buf);
    return vmini->buf_lens[vmini->buffers];
}

/* echo hello > /proc/virtio-mini-0 */
static ssize_t virtio_mini_write(struct file* fil, const char *buf, size_t count, loff_t *offp)
{
    struct virtio_mini_device *vmini = fil->private_data;
	void *to_send;
	unsigned long res;
    struct scatterlist sg;
    struct virtqueue *vq = vmini->queues[VIRTIO_MINI_VQ_TX].vq;

    if (vmini->buffers >= VIRTIO_MINI_BUFFERS)
	{
        printk(KERN_INFO "all buffers used!");
        return ENOSPC;
    }

	/* 分配空间用于保存用户要发送的数据 */
    to_send = kmalloc(count, GFP_KERNEL);
    if (!to_send)
	{
        return 1;
    }

	/* 将用户的数据保存到to_send */
    res = copy_from_user(to_send, buf, count);
    if (res != 0) {
        printk(KERN_INFO "Could not write %lu bytes!", res);
        /* update count to actual number of bytes written */
        count = count - res;
    }

	/*
	 * virtqueue中数据是存VirtQueueElement中的in_sg或out_sg散列中的
	 * 驱动中用对应的api来打包,这里virtqueue_add_outbuf
	 * 所以填充的out_sg
	 *
	 * 在设备中通过virtqueue_pop来获取到对应的VirtQueueElement后取出散列中的数据
	 * 1. 取出element
	 * vqe = virtqueue_pop(vq, sizeof(VirtQueueElement));
	 * 2. 取出element里的数据
	 * iov_to_buf(vqe->out_sg, vqe->out_num, 0, rcv_bufs, vqe->out_sg->iov_len);
	 */
    sg_init_one(&sg, to_send, count);
    vmini->buf_lens[vmini->buffers++] = count;
    virtqueue_add_outbuf(vq, &sg, 1, to_send, GFP_KERNEL);
	virtqueue_kick(vq);

    return count;
}

/* host has acknowledged the message; consume buffer */
void virtio_mini_tx_notify_cb(struct virtqueue *vq)
{
    int len;
    void *buf = virtqueue_get_buf(vq, &len);

    /* free sent data */
    if (buf)
	{
        kfree(buf);
    }

    return;
}

void virtio_mini_rx_notify_cb(struct virtqueue *vq)
{
    int len;
    struct virtio_mini_device *vmini = vq->vdev->priv;

    vmini->read_data = virtqueue_get_buf(vq, &len);
    vmini->buffers--;
    complete(&vmini->data_ready);

    printk(KERN_INFO "Received %i bytes", len);
}

int vmini_find_vqs(struct virtio_mini_device *vmini)
{
	int i;
	int err;
    struct virtqueue *vqs[VIRTIO_MINI_VQ_MAX];

    static const char *names[VIRTIO_MINI_VQ_MAX] = {
		[VIRTIO_MINI_VQ_TX] = "virtio-mini-tx",
		[VIRTIO_MINI_VQ_RX] = "virtio-mini-rx"
	};

    static vq_callback_t *callbacks[VIRTIO_MINI_VQ_MAX] = {
		[VIRTIO_MINI_VQ_TX] = virtio_mini_tx_notify_cb,
		[VIRTIO_MINI_VQ_RX] = virtio_mini_rx_notify_cb
	};

    err = virtio_find_vqs(vmini->vdev, VIRTIO_MINI_VQ_MAX, vqs, callbacks, names, NULL);
    if (err)
	{
        return err;
    }

	for (i = 0; i < VIRTIO_MINI_VQ_MAX; i++)
		vmini->queues[i].vq = vqs[i];

    return 0;
}

int probe_virtio_mini(struct virtio_device *vdev)
{
    struct virtio_mini_device *vmini;
    int err;
    char proc_name[20];

    printk(KERN_INFO "virtio-mini device found\n");

	vmini = kzalloc(sizeof(struct virtio_mini_device), GFP_KERNEL);
    if (vmini == NULL) {
        err = ENOMEM;
        goto err;
    }

    /*
	 * make it possible to access underlying virtio_device
	 * from virtio_mini_device and vice versa
	 */
    vdev->priv = vmini;
    vmini->vdev = vdev;
    err = vmini_find_vqs(vmini);
    if (err) {
        printk(KERN_INFO "Error adding virtqueue\n");
        goto err;
    }
    vmini->buffers = 0;

    init_completion(&vmini->data_ready);

    /*
     * create a proc entry named "/proc/virtio-mini-<bus_idx>"
	 * proc_dir_entry data pointer points to associated virtio_mini_device
	 * allows access to virtqueues from defined file_operations functions
	 */
    snprintf(proc_name, sizeof(proc_name), "%s-%i", VIRTIO_MINI_STRING, vdev->index);
    vmini->pde = proc_create_data(proc_name, 0644, NULL, &pde_fops, vmini);
    if (!vmini->pde) {
        printk(KERN_INFO "Error creating proc entry");
        goto err;
    }

    printk(KERN_INFO "virtio-mini device probe successfully\n");

    return 0;

err:
	kfree(vmini);
	return err;
}

void remove_virtio_mini(struct virtio_device *vdev)
{
    struct virtio_mini_device *vmini = vdev->priv;

    proc_remove(vmini->pde);
    complete(&vmini->data_ready);
    vdev->config->reset(vdev);
    vdev->config->del_vqs(vdev);
    kfree(vdev->priv);

    printk(KERN_INFO "virtio-mini device removed\n");
}

module_virtio_driver(driver_virtio_mini);

MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("virtio example front-end driver");
MODULE_LICENSE("GPL v2");
