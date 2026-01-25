/*
 * NoticeKorea NKUSBVME USB/VME Interface board driver
 * Refactored for Rocky Linux 9 (Kernel 5.14+)
 * 2026-01-24 Refactoring by Gemini
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

#define DRIVER_VERSION "v2.0.0-RL9"
// 저자 정보 수정: 원작자(H.J. Kim) + 유지보수 총괄(Dr. J.Y. Choi) + 리팩토링(Gemini)
#define DRIVER_AUTHOR "J.Y.Choi, Refactored by Gemini"
#define DRIVER_DESC "NoticeKorea USB/VME interface board driver"

#define NKUSBVME_MINOR	190
#define OBUF_SIZE 0x1000
#define IBUF_SIZE 0x10000
#define NAK_TIMEOUT (HZ)

#define USB_NKUSBVME_VENDOR_ID	0x547
#define USB_NKUSBVME_PRODUCT_ID	0x1095

#define NKUSBVME_ENDPOINT_IN_ID  0x82
#define NKUSBVME_ENDPOINT_OUT_ID 0x06

struct nkusbvme_usb_data {
    struct usb_device *nkusbvme_dev;
    int isopen;
    int present;
    char *obuf, *ibuf;
    wait_queue_head_t wait_q;
    struct mutex io_mutex;
};

static struct nkusbvme_usb_data nkusbvme_instance;

static int open_nkusbvme(struct inode *inode, struct file *file) {
    struct nkusbvme_usb_data *nkusbvme = &nkusbvme_instance;
    mutex_lock(&nkusbvme->io_mutex);
    if (nkusbvme->isopen || !nkusbvme->present) {
        mutex_unlock(&nkusbvme->io_mutex);
        return -EBUSY;
    }
    nkusbvme->isopen = 1;
    init_waitqueue_head(&nkusbvme->wait_q);
    mutex_unlock(&nkusbvme->io_mutex);
    return 0;
}

static int close_nkusbvme(struct inode *inode, struct file *file) {
    struct nkusbvme_usb_data *nkusbvme = &nkusbvme_instance;
    mutex_lock(&nkusbvme->io_mutex);
    nkusbvme->isopen = 0;
    mutex_unlock(&nkusbvme->io_mutex);
    return 0;
}

static ssize_t write_nkusbvme(struct file *file, const char __user *buffer, size_t count, loff_t * ppos) {
    DEFINE_WAIT(wait);
    struct nkusbvme_usb_data *nkusbvme = &nkusbvme_instance;
    unsigned long copy_size;
    unsigned long bytes_written = 0;
    unsigned int partial;
    int result = 0;
    int maxretry;
    int errn = 0;

    mutex_lock(&nkusbvme->io_mutex);

    if (!nkusbvme->present || !nkusbvme->nkusbvme_dev) {
        mutex_unlock(&nkusbvme->io_mutex);
        return -ENODEV;
    }

    do {
        unsigned long thistime;
        char *obuf = nkusbvme->obuf;

        thistime = copy_size = (count >= OBUF_SIZE) ? OBUF_SIZE : count;
        if (copy_from_user(nkusbvme->obuf, buffer, copy_size)) {
            errn = -EFAULT;
            goto error;
        }

        maxretry = 5;
        while (thistime) {
            if (!nkusbvme->nkusbvme_dev) {
                errn = -ENODEV;
                goto error;
            }

            result = usb_bulk_msg(nkusbvme->nkusbvme_dev,
                    usb_sndbulkpipe(nkusbvme->nkusbvme_dev, NKUSBVME_ENDPOINT_OUT_ID),
                    obuf, thistime, &partial, 5000);

            if (result == -ETIMEDOUT) {
                if (!maxretry--) {
                    errn = -ETIME;
                    goto error;
                }
                prepare_to_wait(&nkusbvme->wait_q, &wait, TASK_INTERRUPTIBLE);
                schedule_timeout(NAK_TIMEOUT);
                finish_wait(&nkusbvme->wait_q, &wait);
                continue;
            } else if (!result && partial) {
                obuf += partial;
                thistime -= partial;
            } else {
                break;
            }
        }

        if (result) {
            pr_err("nkusbvme: Write error %d\n", result);
            errn = -EIO;
            goto error;
        }
        bytes_written += copy_size;
        count -= copy_size;
        buffer += copy_size;
    } while (count > 0);

    mutex_unlock(&nkusbvme->io_mutex);
    return bytes_written ? bytes_written : -EIO;

error:
    mutex_unlock(&nkusbvme->io_mutex);
    return errn;
}

static ssize_t read_nkusbvme(struct file *file, char __user *buffer, size_t count, loff_t * ppos) {
    DEFINE_WAIT(wait);
    struct nkusbvme_usb_data *nkusbvme = &nkusbvme_instance;
    ssize_t read_count = 0;
    unsigned int partial;
    int this_read;
    int result;
    int maxretry = 10;
    char *ibuf;

    mutex_lock(&nkusbvme->io_mutex);

    if (!nkusbvme->present || !nkusbvme->nkusbvme_dev) {
        mutex_unlock(&nkusbvme->io_mutex);
        return -ENODEV;
    }
    ibuf = nkusbvme->ibuf;

    while (count > 0) {
        if (!nkusbvme->nkusbvme_dev) {
            mutex_unlock(&nkusbvme->io_mutex);
            return -ENODEV;
        }
        this_read = (count >= IBUF_SIZE) ? IBUF_SIZE : count;

        result = usb_bulk_msg(nkusbvme->nkusbvme_dev,
                usb_rcvbulkpipe(nkusbvme->nkusbvme_dev, NKUSBVME_ENDPOINT_IN_ID),
                ibuf, this_read, &partial, 8000);

        if (partial) {
            // Success
        } else if (result == -ETIMEDOUT || result == 15) {
            if (!maxretry--) {
                mutex_unlock(&nkusbvme->io_mutex);
                return -ETIME;
            }
            prepare_to_wait(&nkusbvme->wait_q, &wait, TASK_INTERRUPTIBLE);
            schedule_timeout(NAK_TIMEOUT);
            finish_wait(&nkusbvme->wait_q, &wait);
            continue;
        } else if (result != -EREMOTEIO) {
            mutex_unlock(&nkusbvme->io_mutex);
            pr_err("nkusbvme: Read error %d\n", result);
            return -EIO;
        } else {
            mutex_unlock(&nkusbvme->io_mutex);
            return 0;
        }

        if (partial) {
            if (copy_to_user(buffer, ibuf, partial)) {
                mutex_unlock(&nkusbvme->io_mutex);
                return -EFAULT;
            }
            count -= partial;
            read_count += partial;
            buffer += partial;
        }
    }
    mutex_unlock(&nkusbvme->io_mutex);
    return read_count;
}

static long unlocked_ioctl_nkusbvme(struct file *file, unsigned int cmd, unsigned long arg) {
    return 0;
}

static struct file_operations usb_nkusbvme_fops = {
    .owner = THIS_MODULE,
    .read = read_nkusbvme,
    .write = write_nkusbvme,
    .unlocked_ioctl = unlocked_ioctl_nkusbvme,
    .open = open_nkusbvme,
    .release = close_nkusbvme,
};

static struct usb_class_driver usb_nkusbvme_class = {
    .name = "nkusbvme%d",
    .fops = &usb_nkusbvme_fops,
    .minor_base = NKUSBVME_MINOR,
};

static int probe_nkusbvme(struct usb_interface *intf, const struct usb_device_id *id) {
    struct usb_device *dev = interface_to_usbdev(intf);
    struct nkusbvme_usb_data *nkusbvme = &nkusbvme_instance;
    int retval;

    retval = usb_register_dev(intf, &usb_nkusbvme_class);
    if (retval) return -ENOMEM;

    nkusbvme->nkusbvme_dev = dev;
    nkusbvme->obuf = (char *) kmalloc(OBUF_SIZE, GFP_KERNEL);
    if (!nkusbvme->obuf) return -ENOMEM;
    nkusbvme->ibuf = (char *) kmalloc(IBUF_SIZE, GFP_KERNEL);
    if (!nkusbvme->ibuf) {
        kfree(nkusbvme->obuf);
        return -ENOMEM;
    }
    mutex_init(&(nkusbvme->io_mutex));
    usb_set_intfdata(intf, nkusbvme);
    nkusbvme->present = 1;
    return 0;
}

static void disconnect_nkusbvme(struct usb_interface *intf) {
    struct nkusbvme_usb_data *nkusbvme = usb_get_intfdata(intf);
    usb_set_intfdata(intf, NULL);
    if (nkusbvme) {
        usb_deregister_dev(intf, &usb_nkusbvme_class);
        mutex_lock(&nkusbvme->io_mutex);
        nkusbvme->present = 0;
        nkusbvme->isopen = 0;
        kfree(nkusbvme->ibuf);
        kfree(nkusbvme->obuf);
        mutex_unlock(&nkusbvme->io_mutex);
    }
}

static struct usb_device_id nkusbvme_table [] = {
    { USB_DEVICE(USB_NKUSBVME_VENDOR_ID, USB_NKUSBVME_PRODUCT_ID)},
    {}
};
MODULE_DEVICE_TABLE(usb, nkusbvme_table);

static struct usb_driver nkusbvme_driver = {
    .name = "nkusbvme",
    .probe = probe_nkusbvme,
    .disconnect = disconnect_nkusbvme,
    .id_table = nkusbvme_table,
};

module_usb_driver(nkusbvme_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");