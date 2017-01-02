#include "irq_latency.h"


int latency_init(void);

void latency_exit(void);

static int latencty_open(struct inode *inode, struct file *fp);

static int latency_read(struct file *fp);

static int latency_ioctl(struct file *fp, int period);

static int latency_close(struct inode *inode, struct file *fp);

MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_DESCRIPTION("Measure interrupt latency");

module_init(latency_init);
module_exit(latency_exit);