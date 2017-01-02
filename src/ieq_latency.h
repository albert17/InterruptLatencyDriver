#ifdef IRQ_LATENCY_H
#define IEQ_LATENCY_H

/* Includes */
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>
#include<linux/uaccess.h>

/* Defines */
#define DEVICENAME "irq_latency"

/* Functions */
int latency_init(void);
void latency_exit(void);
static int latencty_open(struct inode *inode, struct file *fp);
static int latency_read(struct file *fp);
static int latency_ioctl(struct file *fp, int period);
static int latency_close(struct inode *inode, struct file *fp);

/* Variables */
struct cdev *mcdev;
int major_number;
int ret;
dev_t dev_num;

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = latency_open,
    .write = latency_write,
    .read = latency_read,
    .release = latency_close,
    .llseek = latency_lseek
};

#endif /* IRQ_LATENCY_H */