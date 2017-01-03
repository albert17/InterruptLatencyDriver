/* Includes */
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/semaphore.h>
#include<linux/uaccess.h>

#include "irq.h"

static u32 pins8_offset [47] = {};
static u32 pins8_val [47] = {};
static u32 pins9_offset [47] = {};
static u32 pins9_val [47] = {};

static int setup_pinmux(struct latency_dev *latency_devp) { 
   u16 irq_pin = latencty_devp->irq_pin;
   u16 gpio_pin = latencty_devp->gpio_pin;
   u16 irq_offset, gpio_offset;

   // Check irq_pin range
   if((irq_pin % 100) >= 0 && (irq_pin % 100) <= 46) {
       if (irq_pin >= 900) {
           irq_offset = pin9_offset[irq_pin % 100];
           irq_pin = pin9_value[irq_pin % 100];
       }
       else if(irq_pin >= 800) {
           irq_offset = pin8_offset[irq_pin % 100];
           irq_pin = pin8_value[irq_pin % 100];
       }
       else {
           return -1;
       }
   }

   // Check gpio_pin range
   if((gpio_pin % 100) >= 0 && (gpio_pin % 100) <= 46) {
       if (gpio_pin >= 900) {
           gpio_offset = pin9_offset[gpio_pin % 100];
           gpio_pin = pin9_value[gpio_pin % 100];
       }
       else if(gpio_pin >= 800) {
           gpio_offset = pin8_offset[gpio_pin % 100];
           gpio_pin = pin8_value[gpio_pin % 100];
       }
       else {
           return -1;
       }
   }

   // Check de pin is a GPIO
   if(irq_offset == 0 | gpio_pin == 0) {
       return -1;
   }
   
    // Write irq_pin setting
    void* addr = ioremap(irq_offset, 4);

    if (NULL == addr) { return -EBUSY; }

    iowrite32(INPUT, addr); // write settings for each pin
    iounmap(addr);

    // Write irq_pin setting 
    void* addr = ioremap(gpio_offset, 4);

    if (NULL == addr) {return -EBUSY; }

    iowrite32(OUTPUT, addr); // write settings for each pin
    iounmap(addr);

    // Set Beaglebone pins number
    latencty_devp->irq_pin = irq_pin;
    latencty_devp->gpio_pin = gpio_pin;
    return 0;
}

int configure_gpio_irq(struct latency_dev *latency_devp) {

   ret = setup_pinmux();
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
      test_irq_latency_interrupt_handler,
      IRQF_TRIGGER_FALLING | IRQF_DISABLED,
      D_NAME,
      (void*)&data
   );
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to enable IRQ %d for pin %d.\n",
         latency_devp->irq, latency_devp->irq_pin);
      goto err_free_irq_return;
   } else
      latency_devp->irq_enabled = 1;
	
   init_timer(&latency_devp->timer);
   latency_devp->timer.expires = jiffies + TEST_INTERVAL;
   latency_devp->timer.data = (unsigned long)&data;
   latency_devp->timer.function = test_irq_latency_timer_handler;
   add_timer(&latency_devp->timer);

   printk(KERN_INFO D_NAME
      " : beginning GPIO IRQ latency test (%u passes in %d seconds).\n",
      NUM_TESTS, (NUM_TESTS * TEST_INTERVAL) / HZ);
	
   return 0;

err_free_irq_return:
   gpio_free(latency_devp->irq_pin);
err_free_gpio_return:
   gpio_free(latency_devp->gpio_pin);
err_return:
   return ret;
}

void release_gpio_irq(struct latency_dev *latency_devp) {
   del_timer_sync(&latency_devp->timer);
	
   free_irq(latency_devp->irq, (void*)&data);
	
   gpio_free(latency_devp->irq_pin);
   gpio_free(latency_devp->gpio_pin);
	
   printk(KERN_INFO D_NAME " : unloaded IRQ latency test.\n");
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
            printk(KERN_INFO D_NAME
               " : GPIO IRQ triggered after > 1 sec, something is fishy.\n");
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
      printk(KERN_INFO D_NAME
         " : finished %u passes. average GPIO IRQ latency is %lu nsecs.\n",
            NUM_TESTS, data->avg_nsecs);
	   		
      goto stopTesting;
   } else {
      if (data->missed_irqs > MISSED_IRQ_MAX) {
         printk(KERN_INFO D_NAME " : too many interrupts missed. "
            "check your jumper cable and reload this module!\n");

      goto stopTesting;
   }
	   
      mod_timer(&data->timer, jiffies + TEST_INTERVAL);
	   
      getnstimeofday(&data->gpio_time);
      gpio_set_value(data->gpio_pin, 0);
   }
	
   return;
	
stopTesting:
   if (latency_devp->irq_enabled) {
      disable_irq(latency_devp->irq);
      latency_devp->irq_enabled = 0;
   }
}
