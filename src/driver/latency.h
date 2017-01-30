/* Defines */
#define LAST 0
#define AVG 1

/* Driver functions declaration*/
int latency_init(void);
void latency_exit(void);
int latency_open(struct inode *inode, struct file *fp);
int latency_close(struct inode *inode, struct file *fp);
int latency_ioctl(struct file *fp, enum mode mode, u16 irq_pin, u16 gpio_pin, int period);
long latency_read(struct file *fp, int type);

