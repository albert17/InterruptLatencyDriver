/* Defines */
#define LAST 0
#define AVG 1

/* Driver functions declaration*/
int latency_init(void);
void latency_exit(void);
static int latencty_open(struct inode *inode, struct file *fp);
static int latency_close(struct inode *inode, struct file *fp);
static int latency_ioctl(struct file *fp, u16 irq_pin, u16 gpio_pin, int period);
static int latency_read(struct file *fp, int type);

