#include "irq_latency.h"


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

static int latencty_open(struct inode *inode, struct file *fp);

static int latency_close(struct inode *inode, struct file *fp);

static int latency_ioctl(struct file *fp, int period);

static int latency_read(struct file *fp);




MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_DESCRIPTION("Measure interrupt latency");

module_init(latency_init);
module_exit(latency_exit);