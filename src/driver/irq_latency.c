/* Includes */
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>
#include<linux/uaccess.h>

/* Defines */
#define DEVICENAME "latency"
#define AM33XX_CONTROL_BASE 0x44e10000

/* Structures */
struct data {
   struct timespec gpio_time, irq_time;
   u16 irq;
   u16 irq_pin, gpio_pin;
   u8 irq_fired, irq_enabled;
   struct timer_list timer;
   u32 test_count;
   unsigned long avg_nsecs, missed_irqs;
   unsigned long last_nsecs;
   unsigned int period;
};

/* Aux functions declaration*/
static int setup_pinmux(void);
static int configure_gpio_irq(void);
static void release_gpio_irq(void);
static irqreturn_t test_irq_latency_interrupt_handler(int irq, void* dev_id);
static void test_irq_latency_timer_handler(unsigned long ptr);

/* Driver functions declaration*/
int latency_init(void);
void latency_exit(void);
static int latencty_open(struct inode *inode, struct file *fp);
static int latency_close(struct inode *inode, struct file *fp);
static int latency_ioctl(struct file *fp, u16 irq_pin, u16 gpio_pin, int period);
static int latency_read(struct file *fp, int type);

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


/* Aux functions implementation*/
static int setup_pinmux(void) {
   int i;
   static u32 pins[] = { // offsets in AM335x ref man Table 9-10, upon pin names
			 // pin names: from table/mode0 in BB ref manual
      AM33XX_CONTROL_BASE + 0x878,   // test pin (60): gpio1_28 (beaglebone p9/12) - 1*32+28 = 60
				     // note that 2 and 3 are coded in three bits, since 0x7 needs 3b
      0x7 | (2 << 3),                //       mode 7 (gpio), PULLUP, OUTPUT: 010111
      AM33XX_CONTROL_BASE + 0x840,   // irq pin (48): gpio1_16 (beaglebone p9/15) - 1*32+16 = 48
      0x7 | (2 << 3) | (1 << 5),     //       mode 7 (gpio), PULLUP, INPUT: 110111
   };

   for (i=0; i<4; i+=2) {	// map the mapped i/o addresses to kernel high memory
      void* addr = ioremap(pins[i], 4);

      if (NULL == addr)
         return -EBUSY;

      iowrite32(pins[i+1], addr); // write settings for each pin
      iounmap(addr);
   }

   return 0;
}

int configure_gpio_irq(void) {

   ret = setup_pinmux();
   if (ret < 0) {
      printk(KERN_ALERT DRV_NAME " irq_latency : failed to apply pinmux settings.\n");
      goto err_return;
   }
	
   ret = gpio_request_one(data.gpio_pin, GPIOF_OUT_INIT_HIGH,
      DRV_NAME " gpio");
   if (ret < 0) {
      printk(KERN_ALERT DRV_NAME " irq_latency : failed to request GPIO pin %d.\n",
         data.gpio_pin);
      goto err_return;
   }
	
   ret = gpio_request_one(data.irq_pin, GPIOF_IN, DRV_NAME " irq");
   if (ret < 0) {
      printk(KERN_ALERT DRV_NAME " irq_latency : failed to request IRQ pin %d.\n",
         data.irq_pin);
      goto err_free_gpio_return;
   }
	
   ret = gpio_to_irq(data.irq_pin);
   if (ret < 0) {
      printk(KERN_ALERT DRV_NAME " irq_latency : failed to get IRQ for pin %d.\n",
         data.irq_pin);
      goto err_free_irq_return;
   } else {
      data.irq = (u16)ret;
      ret = 0;
   }
	
   ret = request_any_context_irq(
      data.irq,
      test_irq_latency_interrupt_handler,
      IRQF_TRIGGER_FALLING | IRQF_DISABLED,
      DRV_NAME,
      (void*)&data
   );
   if (ret < 0) {
      printk(KERN_ALERT DRV_NAME " irq_latency : failed to enable IRQ %d for pin %d.\n",
         data.irq, data.irq_pin);
      goto err_free_irq_return;
   } else
      data.irq_enabled = 1;
	
   init_timer(&data.timer);
   data.timer.expires = jiffies + TEST_INTERVAL;
   data.timer.data = (unsigned long)&data;
   data.timer.function = test_irq_latency_timer_handler;
   add_timer(&data.timer);

   printk(KERN_INFO DRV_NAME
      " irq_latency : beginning GPIO IRQ latency test (%u passes in %d seconds).\n",
      NUM_TESTS, (NUM_TESTS * TEST_INTERVAL) / HZ);
	
   return 0;

err_free_irq_return:
   gpio_free(data.irq_pin);
err_free_gpio_return:
   gpio_free(data.gpio_pin);
err_return:
   return ret;
}

void release_gpio_irq(void) {
   del_timer_sync(&data.timer);
	
   free_irq(data.irq, (void*)&data);
	
   gpio_free(data.irq_pin);
   gpio_free(data.gpio_pin);
	
   printk(KERN_INFO DRV_NAME " irq_latency : unloaded IRQ latency test.\n");
}

static irqreturn_t irq_latency_interrupt_handler(int irq, void* dev_id) {
   struct data* data = (struct data*)dev_id;
   
   getnstimeofday(&data->irq_time);
   data->irq_fired = 1;
   
   gpio_set_value(data->gpio_pin, 1);
   
   return IRQ_HANDLED;
}


static void irq_latency_timer_handler(unsigned long ptr) {
   struct data* data = (struct data*)ptr;
   u8 test_ok = 0;
   
   if (data->irq_fired) {
      struct timespec delta = timespec_sub(data->irq_time, data->gpio_time);
         if (delta.tv_sec > 0) {
            printk(KERN_INFO DRV_NAME
               " irq_latency : GPIO IRQ triggered after > 1 sec, something is fishy.\n");
            data->missed_irqs++;
         } else {
            data->avg_nsecs = data->avg_nsecs ?
               (unsigned long)(((unsigned long long)delta.tv_nsec +
                  (unsigned long long)data->avg_nsecs) >> 1) :
                  delta.tv_nsec;
            data->last_nsecs = (unsigned long) delta.tv_nsec;
            test_ok = 1;
         }

      data->irq_fired = 0;
   } else {
      data->missed_irqs++;
   }

   if (test_ok && ++data->test_count >= NUM_TESTS) {
      printk(KERN_INFO DRV_NAME
         " irq_latency : finished %u passes. average GPIO IRQ latency is %lu nsecs.\n",
            NUM_TESTS, data->avg_nsecs);
	   		
      goto stopTesting;
   } else {
      if (data->missed_irqs > MISSED_IRQ_MAX) {
         printk(KERN_INFO DRV_NAME " irq_latency : too many interrupts missed. "
            "check your jumper cable and reload this module!\n");

      goto stopTesting;
   }
	   
      mod_timer(&data->timer, jiffies + TEST_INTERVAL);
	   
      getnstimeofday(&data->gpio_time);
      gpio_set_value(data->gpio_pin, 0);
   }
	
   return;
	
stopTesting:
   if (data.irq_enabled) {
      disable_irq(data.irq);
      data.irq_enabled = 0;
   }
}

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