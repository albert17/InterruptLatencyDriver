/* Includes */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include "irq.h"
#include "latency.h"

MODULE_AUTHOR("ALBERTO ALVAREZ (alberto.alvarez.aldea@gmail.com)");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Measure interrupt latency");
MODULE_VERSION("1.0");

static u32 pins8_offset[47] = {0,0,0x818,0x81C,0x808,0x80C,0,0,0,0,0x834,0x830,0,0x828,0x83C,0x838,0x82C,0x88C,0,0x884,0x880,0x814,0x810,0x804,0x800,0x87C,0x8E0,0x8E8,0x8E4,0x8EC,0,0,0,0,0,0,0,0,0x8B8,0x8BC,0x8B4,0x8B0,0x8A8,0x8AC,0x8A0,0x8A4};
static u32 pins8_value[47] = {0,0,38,39,34,35,0,0,0,0,45,44,0,26,47,46,27,65,0,63,62,37,36,33,32,61,86,88,87,89,0,0,0,0,0,0,0,0,76,77,74,75,72,73,70,71};
static u32 pins9_offset[47] = {0,0,0,0,0,0,0,0,0,0,0,0x878,0,0,0x840,0,0,0,0,0,0,0,0x844,0,0x9AC,0,0x9A4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x964,0,0,0,0};
static u32 pins9_value[47] = {0,0,0,0,0,0,0,0,0,0,0,60,0,0,48,0,0,0,0,0,0,0,49,0,117,0,115,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0};

int setup_pinmux(struct latency_dev *latency_devp) { 
   u16 irq_pin = latency_devp->irq_pin;
   u16 gpio_pin = latency_devp->gpio_pin;
   u32 irq_offset = 0, gpio_offset = 0;
   void* addr;

   // Check irq_pin range
   if((irq_pin % 100) >= 0 && (irq_pin % 100) <= 46) {
       if (irq_pin >= 900) {
           irq_offset = pins9_offset[irq_pin % 100];
           irq_pin = pins9_value[irq_pin % 100];
       }
       else if(irq_pin >= 800) {
           irq_offset = pins8_offset[irq_pin % 100];
           irq_pin = pins8_value[irq_pin % 100];
       }
       else {
           return -1;
       }
   }

   // Check gpio_pin range
   if((gpio_pin % 100) >= 0 && (gpio_pin % 100) <= 46) {
       if (gpio_pin >= 900) {
           gpio_offset = pins9_offset[gpio_pin % 100];
           gpio_pin = pins9_value[gpio_pin % 100];
       }
       else if(gpio_pin >= 800) {
           gpio_offset = pins8_offset[gpio_pin % 100];
           gpio_pin = pins8_value[gpio_pin % 100];
       }
       else {
           return -1;
       }
   }

   // Check de pin is a GPIO
   if((irq_offset == 0) || (gpio_pin == 0)) {
       return -1;
   }
   
    // Write irq_pin setting
    addr = ioremap(irq_offset, 4);

    if (NULL == addr) { return -EBUSY; }

    iowrite32(INPUT, addr); // write settings for each pin
    iounmap(addr);

    // Write irq_pin setting 
    addr = ioremap(gpio_offset, 4);

    if (NULL == addr) {return -EBUSY; }

    iowrite32(OUTPUT, addr); // write settings for each pin
    iounmap(addr);

    // Set Beaglebone pins number
    latency_devp->irq_pin = irq_pin;
    latency_devp->gpio_pin = gpio_pin;
    return 0;
}

int configure_gpio_irq(struct latency_dev *latency_devp) {

   int ret = 0;
   ret = setup_pinmux(latency_devp);
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to apply pinmux settings.\n");
      goto err_return;
   }
	
   ret = gpio_request_one(latency_devp->gpio_pin, GPIOF_OUT_INIT_HIGH,
      D_NAME " gpio");
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to request GPIO pin %d.\n",
         latency_devp->gpio_pin);
      goto err_return;
   }
	
   ret = gpio_request_one(latency_devp->irq_pin, GPIOF_IN, D_NAME " irq");
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to request IRQ pin %d.\n",
         latency_devp->irq_pin);
      goto err_free_gpio_return;
   }
	
   ret = gpio_to_irq(latency_devp->irq_pin);
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to get IRQ for pin %d.\n",
         latency_devp->irq_pin);
      goto err_free_irq_return;
   } else {
      latency_devp->irq = (u16)ret;
      ret = 0;
   }
	
   ret = request_any_context_irq(
      latency_devp->irq,
      irq_handler,
      IRQF_TRIGGER_FALLING | IRQF_DISABLED,
      D_NAME,
      (void*)latency_devp
   );
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to enable IRQ %d for pin %d.\n",
         latency_devp->irq, latency_devp->irq_pin);
      goto err_free_irq_return;
   } else
      latency_devp->irq_enabled = 1;
	
   init_timer(&latency_devp->timer);
   latency_devp->timer.expires = jiffies + latency_devp->period;
   latency_devp->timer.data = (unsigned long)&latency_devp;
   latency_devp->timer.function = timer_handler;
   add_timer(&latency_devp->timer);
	
   return 0;

err_free_irq_return:
   gpio_free(latency_devp->irq_pin);
err_free_gpio_return:
   gpio_free(latency_devp->gpio_pin);
err_return:
   return ret;
}

void release_gpio_irq(struct latency_dev *latency_devp) {
   disable_irq(latency_devp->irq);
   latency_devp->irq_enabled = 0;
   
   del_timer_sync(&latency_devp->timer);
	
   free_irq(latency_devp->irq, (void*)latency_devp);
	
   gpio_free(latency_devp->irq_pin);
   gpio_free(latency_devp->gpio_pin);
}

irqreturn_t irq_handler(int irq, void* dev_id) {
   struct latency_dev* latency_devp = (struct latency_dev*)dev_id;
   
   getnstimeofday(&latency_devp->irq_time);
   latency_devp->irq_fired = 1;
   
   gpio_set_value(latency_devp->gpio_pin, 1);
   
   return IRQ_HANDLED;
}


void timer_handler(unsigned long ptr) {
   struct latency_dev* latency_devp = (struct latency_dev*)ptr;

   if (latency_devp->irq_fired) {
      struct timespec delta = timespec_sub(latency_devp->irq_time, latency_devp->gpio_time);    
        latency_devp->avg_nsecs = latency_devp->avg_nsecs ?
            (unsigned long)(((unsigned long long)delta.tv_nsec +
                (unsigned long long)latency_devp->avg_nsecs) >> 1) :
                delta.tv_nsec;
        latency_devp->last_nsecs = (unsigned long) delta.tv_nsec;
    }

      latency_devp->irq_fired = 0; 
      mod_timer(&latency_devp->timer, jiffies + latency_devp->period);
	   
      getnstimeofday(&latency_devp->gpio_time);
      gpio_set_value(latency_devp->gpio_pin, 0);
}
