/* Includes */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/slab.h>	
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include<linux/parport.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>


#include "irq.h"
#include "latency.h"

/* Driver functions declaration*/
int latency_init(void);
void latency_exit(void);
int latency_open(struct inode *inode, struct file *fp);
int latency_close(struct inode *inode, struct file *fp);
long latency_ioctl(struct file *fp, unsigned int cmd, unsigned long arg);
ssize_t latency_read(struct file *fp, char __user *buf, size_t count, loff_t *fpos);

/* Variables */
static struct latency_dev *latency_devp;
static struct class *latency_class  = NULL;
struct latency_buffer *lb;
struct latency_result *result;

static dev_t dev_num;
static atomic_t available = ATOMIC_INIT(1);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = latency_open,
    .unlocked_ioctl = latency_ioctl,
    .read = latency_read,
    .release = latency_close
};


/* Driver functions implementation*/
int latency_init(void) {
    int ret;
    // obtain major number dinamically
    ret = alloc_chrdev_region(&dev_num,0,1,D_NAME);
    if(ret < 0) {
        printk(KERN_ALERT D_NAME " : failed to allocate major number\n");
        return ret;
    }
    else {
        printk(KERN_INFO D_NAME " : major number allocated succesful\n");
    }
    ret = MAJOR(dev_num);
    printk(KERN_INFO D_NAME " : major number of device is %d\n", ret);

    latency_class = class_create(THIS_MODULE, "latency"); 
    // Allocate latency_dev
    latency_devp = kmalloc(sizeof(struct latency_dev), GFP_KERNEL);
    
    // Create, allocate and initialize cdev structure
    cdev_init(&latency_devp->cdev, &fops);
    latency_devp->cdev.ops = &fops;
    latency_devp->cdev.owner = THIS_MODULE;

    // Add cdev structure to the system
    ret = cdev_add(&latency_devp->cdev,dev_num,1);
    if(ret < 0) {
        printk(KERN_ALERT D_NAME " : device adding to the kerknel failed\n");
        return ret;
    }
    // Add /dev
    device_create(latency_class, NULL, dev_num, NULL, D_NAME);

    printk(KERN_INFO D_NAME " : device additin to the kernel succesful\n");
    return 0;
}

void latency_exit(void) {
    // Removes cdev structure
    cdev_del(&latency_devp->cdev);
    printk(KERN_INFO D_NAME " : removed the mcdev from kernel\n");

    // Free memory
    kfree(latency_devp);

    // Remove /dev
    device_destroy(latency_class, dev_num);

    // Remove /sys
    class_destroy(latency_class);    

    // Unregister device
    unregister_chrdev_region(dev_num,1);
    printk(KERN_INFO D_NAME " : unregistered the device numbers\n");
    printk(KERN_ALERT D_NAME " : character driver is exiting\n");
}

int latency_open(struct inode *inode, struct file *fp) {
    struct latency_dev *dev; /* device information */
    if (!atomic_dec_and_test (&available)) {
        atomic_inc(&available);
        printk(KERN_ALERT D_NAME " : device occuped\n");
        return -EBUSY; /* already open */
    }
	/*  Find the device */
	dev = container_of(inode->i_cdev, struct latency_dev, cdev);
    dev->state = OFF;
	fp->private_data = dev;
    printk(KERN_ALERT D_NAME " : device oppened\n");
    return 0; /* success */    
}

int latency_close(struct inode *inode, struct file *fp) {
    struct latency_dev *dev =fp->private_data;
    printk(KERN_ALERT D_NAME " : device closing\n");
    if(dev->state != OFF) {
        release_gpio_irq(dev);
    }
    printk(KERN_ALERT D_NAME " : device released gpio irq\n");
    dev->state = OFF;
    atomic_inc(&available); /* release the device */
    printk(KERN_ALERT D_NAME " : device closed\n");
    return 0;
}

long latency_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
    struct latency_dev *dev =fp->private_data;
    int ret = 0;
    printk(KERN_ALERT D_NAME " : ioctl function called\n");
	if (_IOC_TYPE(cmd) != LATENCY_IOC_MAGIC) {
        printk(KERN_ALERT D_NAME " : ioctl incorrect magic number\n");
        return -ENOTTY;
    }

    switch(cmd) {
        case ISET:
            printk(KERN_ALERT D_NAME " : ioctl set called\n");
            if (dev->state == OFF) {
                printk(KERN_ALERT D_NAME " : ioctl copying from user\n");
                if(copy_from_user(&dev->lb, (struct latency_buffer *)arg, sizeof(struct latency_buffer))) { 
                    printk(KERN_ALERT D_NAME " : ioctl fail copy from user\n");
                    return -EFAULT;
                }
                dev->lb.period = (HZ/dev->lb.period);
                dev->state = SET;
            }
            break;
        case ION:
            printk(KERN_ALERT D_NAME " : ioctl on called\n");
            if (dev->state == SET) {
                dev->res.avg = 0; dev->res.var = 0;
                dev->res.max = 0; dev->res.min = -1;
                printk(KERN_ALERT D_NAME " : ioctl configuring gpio\n");
                ret = configure_gpio_irq(dev);
                if (ret == -1) {
                    printk(KERN_ALERT D_NAME " : failed to configure pins.\n");
                    return ret;
                }
                dev->state = ON;
            }
            break;
        case IOFF:
            printk(KERN_ALERT D_NAME " : ioctl off called\n");
            if (dev->state == ON) {
                release_gpio_irq(dev);
                dev->state = OFF;
            }
            break;
        default:
            printk(KERN_ALERT D_NAME " : ioctl default called\n");
            return -ENOTTY;
    }
    return ret;
}

ssize_t latency_read(struct file *fp, char __user *buf, size_t count, loff_t *fpos) {
    struct latency_dev *dev =fp->private_data;
    printk(KERN_ALERT D_NAME " : read function called\n");
    if(copy_to_user((struct latency_result*)buf, &dev->res, sizeof(struct latency_result))) {
        printk(KERN_ALERT D_NAME " : read fail copy to user\n");
        return -EFAULT;
    }
    return 0;
}

MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Measure interrupt latency");
MODULE_VERSION("1.0");

module_init(latency_init);
module_exit(latency_exit);
