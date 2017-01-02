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
struct cdev *lcdev;
int major_number;
int ret;
dev_t dev_num;

static struct data data;


struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = latency_open,
    .ioctl = latency_write,
    .read = latency_read,
    .close = latency_close,
};


/* Driver functions implementation*/
int latency_init(void) {

    // obtain major number dinamically
    ret = alloc_chrdev_region(&dev_num,0,1,DEVICENAME);
    if(ret < 0) {
        printk(KERN_ALERT " irq_latency : failed to allocate major number\n");
        return ret;
    }
    else {
        printk(KERN_INFO " irq_latency : major number allocated succesful\n");
    }
    major_number = MAJOR(dev_num);
    printk(KERN_INFO "irq_latency : major number of device is %d\n",major_number);

     // Create, allocate and initialize cdev structure
    lcdev = cdev_alloc();
    lcdev->ops = &fops;
    lcdev->owner = THIS_MODULE;

    // Add cdev structure to the system
    ret = cdev_add(lcdev,dev_num,1);
    if(ret < 0) {
        printk(KERN_ALERT "irq_latency : device adding to the kerknel failed\n");
        return ret;
    }
    else {
        printk(KERN_INFO "irq_latency : device additin to the kernel succesful\n");
    }
    return 0;
}

void latency_exit(void) {
    // Removes cdev structure
    cdev_del(lcdev);
    printk(KERN_INFO " irq_latency : removed the mcdev from kernel\n");

    // Unregister device
    unregister_chrdev_region(dev_num,1);
    printk(KERN_INFO "irq_latency : unregistered the device numbers\n");
    printk(KERN_ALERT " irq_latency : character driver is exiting\n");
}

static int latencty_open(struct inode *inode, struct file *fp) {
    configure_gpio_irq();
}
static int latency_close(struct inode *inode, struct file *fp) {
    release_gpio_irq();
}

static int latency_ioctl(struct file *fp, u16 irq_pin, u16 gpio_pin, int period){
    data.irq_pin = irq_pin;
    data.gpio_pin = gpio_pin;
    data.period = (HZ/period);
}

static int latency_read(struct file *fp, int type) {
    switch (type)
    {
        case LAST:
            return return data.avg_nsecs;
        case AVG:
            return data.last_nsecs;
        default:
            return -1;
    }
}

MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_DESCRIPTION("Measure interrupt latency");

module_init(latency_init);
module_exit(latency_exit);