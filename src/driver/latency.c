/* Includes */
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>
#include<linux/uaccess.h>

#include "irq.h"
#include "latency.h"

/* Variables */
static struct latency_dev lat_devp;
static dev_t dev_num;
static atomic_t available = ATOMIC_INIT(1);

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = latency_open,
    .ioctl = latency_write,
    .read = latency_read,
    .release = latency_close,
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

    // Allocate latency_dev
    latency_devp = kmalloc(sizeof(struct latency_dev), GFP_KERNEL);
    
    // Create, allocate and initialize cdev structure
    latency_devp->cdev = cdev_alloc();
    latency_devp->cdev->ops = &fops;
    latency_devp->cdev->owner = THIS_MODULE;

    // Add cdev structure to the system
    ret = cdev_add(latency_devp->lcdev,dev_num,1);
    if(ret < 0) {
        printk(KERN_ALERT D_NAME " : device adding to the kerknel failed\n");
        return ret;
    }
    else {
        printk(KERN_INFO D_NAME " : device additin to the kernel succesful\n");
    }
    return 0;
}

void latency_exit(void) {
    // Removes cdev structure
    cdev_del(latency_devp->cdev);
    printk(KERN_INFO D_NAME " : removed the mcdev from kernel\n");

    // Free memory
    kfree(latency_devp);

    // Unregister device
    unregister_chrdev_region(dev_num,1);
    printk(KERN_INFO D_NAME " : unregistered the device numbers\n");
    printk(KERN_ALERT D_NAME " : character driver is exiting\n");
}

static int latencty_open(struct inode *inode, struct file *fp) {
    struct latency_dev *dev; /* device information */
    if (! atomic_dec_and_test (&available)) {
        atomic_inc(&available);
        return -EBUSY; /* already open */
    }
	/*  Find the device */
	dev = container_of(inode->i_cdev, struct latencty_dev, cdev);
    dev->state = OFF;
	filp->private_data = dev;

    return 0; /* success */    
}

static int latency_close(struct inode *inode, struct file *fp) {
    struct latency_dev *dev =fp->private_data;
    int ret;
    ret = release_gpio_irq(dev);
    if (ret == -1) {
        printk(KERN_ALERT D_NAME " : failed to release pins.\n");
        return ret;
    }
    atomic_inc(&available); /* release the device */
    return 0;
}

static int latency_ioctl(struct file *fp, enum Mode mode, u16 irq_pin, u16 gpio_pin, int period){
    struct latency_dev *dev =fp->private_data;
    int ret;
    switch (mode)
    {
        case SET:
            if (dev->state == OFF) {
                dev->irq_pin = irq_pin;
                dev->gpio_pin = gpio_pin;
                dev->period = (HZ/period);
                dev->state = SET;
            break;
        case ON:
            if(dev->satate == SET) {
                ret = configure_gpio_irq(dev);
                if (ret == -1) {
                    printk(KERN_ALERT D_NAME " : failed to configure pins.\n");
                    return ret;
                }
                dev->state = ON;
            }
            break;
        case OFF:
            if(dev->state == ON) {
                ret = release_gpio_irq(dev);
                if (ret == -1) {
                    printk(KERN_ALERT D_NAME " : failed to release pins.\n");
                    return ret;
                }
                dev->state = OFF;
            }
        default:
            break;
    }
}

static int latency_read(struct file *fp, int type) {
    struct latency_dev *dev =fp->private_data;
    switch (type)
    {
        case LAST:
            return return dev->avg_nsecs;
        case AVG:
            return dev->last_nsecs;
        default:
            return -1;
    }
}

MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_DESCRIPTION("Measure interrupt latency");

module_init(latency_init);
module_exit(latency_exit);