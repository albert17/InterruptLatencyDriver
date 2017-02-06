/**
 * Author: A. Alvarez
 * Date: 2017/01/30
 * Description: Implement irq and timer handler, using the pins to measure the
 * interrupt latency. 
 */

#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#include "irq.h"
#include "latency.h"

/* Beaglebone for each pin's offsets and values  */
static u32 pins8_offset[46] = {0,0,0x818,0x81C,0x808,0x80C,0,0,0,0,0x834,0x830,0,0x828,0x83C,0x838,0x82C,0x88C,0,0x884,0x880,0x814,0x810,0x804,0x800,0x87C,0x8E0,0x8E8,0x8E4,0x8EC,0,0,0,0,0,0,0,0,0x8B8,0x8BC,0x8B4,0x8B0,0x8A8,0x8AC,0x8A0,0x8A4};
static u32 pins8_value[46] = {0,0,38,39,34,35,0,0,0,0,45,44,0,26,47,46,27,65,0,63,62,37,36,33,32,61,86,88,87,89,0,0,0,0,0,0,0,0,76,77,74,75,72,73,70,71};
static u32 pins9_offset[46] = {0,0,0,0,0,0,0,0,0,0,0,0x878,0,0,0x840,0,0,0,0,0,0,0,0x844,0,0x9AC,0,0x9A4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x964,0,0,0,0};
static u32 pins9_value[46] = {0,0,0,0,0,0,0,0,0,0,0,60,0,0,48,0,0,0,0,0,0,0,49,0,117,0,115,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0};

int setup_pinmux(struct latency_dev *ldev) { 
   u16 irq_pin = ldev->lb.irq_pin;
   u16 gpio_pin = ldev->lb.gpio_pin;
   u32 irq_offset = 0, gpio_offset = 0;
   void* addr;
   
   // Check irq_pin range
   if((irq_pin % 100) >= 0 && (irq_pin % 100) <= 46) {
       // pin is in P9
       if (irq_pin >= 900) {
           irq_offset = pins9_offset[(irq_pin % 100)-1];
           irq_pin = pins9_value[(irq_pin % 100)-1];
       }
       // pin is in P8
       else if(irq_pin >= 800) {
           irq_offset = pins8_offset[(irq_pin % 100)-1];
           irq_pin = pins8_value[(irq_pin % 100)-1];
       }
       // the does not exist
       else {
           return -1;
       }
   }

   // Check gpio_pin range
   if((gpio_pin % 100) >= 0 && (gpio_pin % 100) <= 46) {
        // pin is in P9
       if (gpio_pin >= 900) {
           gpio_offset = pins9_offset[(gpio_pin % 100)-1];
           gpio_pin = pins9_value[(gpio_pin % 100)-1];
       }
       // pin is in P8
       else if(gpio_pin >= 800) {
           gpio_offset = pins8_offset[(gpio_pin % 100)-1];
           gpio_pin = pins8_value[(gpio_pin % 100)-1];
       }
       // pin does not exits
       else {
           return -1;
       }
   }

   // Check pins are GPIO pins
   if((irq_offset == 0) || (gpio_pin == 0)) {
       return -1;
   }
   
    // Write irq_pin setting
    addr = ioremap(AM33XX_CONTROL_BASE + irq_offset, 4);

    if (NULL == addr) { return -EBUSY; }

    iowrite32(INPUT, addr);
    iounmap(addr);

    // Write gpio_pin setting 
    addr = ioremap(AM33XX_CONTROL_BASE + gpio_offset, 4);

    if (NULL == addr) {return -EBUSY; }

    iowrite32(OUTPUT, addr);
    iounmap(addr);

    // Set Beaglebone pins number
    ldev->lb.irq_pin = irq_pin;
    ldev->lb.gpio_pin = gpio_pin;
    return 0;
}

int configure_gpio_irq(struct latency_dev *ldev) {
   int ret = 0;
    // Write pins
   ret = setup_pinmux(ldev);
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to apply pinmux settings.\n");
      goto err_return;
   }
   // Configure gpio pin
   ret = gpio_request_one(ldev->lb.gpio_pin, GPIOF_OUT_INIT_HIGH,
      D_NAME " gpio");
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to request GPIO pin %d.\n",
         ldev->lb.gpio_pin);
      goto err_return;
   }
   // Confgiure irq pin
   ret = gpio_request_one(ldev->lb.irq_pin, GPIOF_IN, D_NAME " irq");
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to request IRQ pin %d.\n",
         ldev->lb.irq_pin);
      goto err_free_gpio_return;
   }
   // Set irq in pin
   ret = gpio_to_irq(ldev->lb.irq_pin);
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to get IRQ for pin %d.\n",
         ldev->lb.irq_pin);
      goto err_free_irq_return;
   } else {
      ldev->irq = (u16)ret;
      ret = 0;
   }
   // Set irq context
   ret = request_any_context_irq(
      ldev->irq,
      irq_handler,
      IRQF_TRIGGER_FALLING | IRQF_DISABLED,
      D_NAME,
      (void*)ldev
   );
   if (ret < 0) {
      printk(KERN_ALERT D_NAME " : failed to enable IRQ %d for pin %d.\n",
         ldev->irq, ldev->lb.irq_pin);
      goto err_free_irq_return;
   } 
   else {
      ldev->irq_enabled = 1;
   }
   
   // Configure and set timer
   init_timer(&ldev->timer);
   ldev->timer.expires = jiffies + ldev->lb.period;
   ldev->timer.data = (unsigned long)ldev;
   ldev->timer.function = timer_handler;
   add_timer(&ldev->timer);
   
   return 0;

err_free_irq_return:
   printk(KERN_ALERT D_NAME " : irq_pin fail\n");
   gpio_free(ldev->lb.irq_pin);
err_free_gpio_return:
   printk(KERN_ALERT D_NAME " : gpio_pin_fail\n");
   gpio_free(ldev->lb.gpio_pin);
err_return:
   return ret;
}

void release_gpio_irq(struct latency_dev *ldev) {
   // Disable irq interrupt
   disable_irq(ldev->irq);
   ldev->irq_enabled = 0;
   // Delete timer
   del_timer_sync(&ldev->timer);
   // Free irq interrupt
   free_irq(ldev->irq, (void*)ldev);
   // Free pins
   gpio_free(ldev->lb.irq_pin);
   gpio_free(ldev->lb.gpio_pin);
}

irqreturn_t irq_handler(int irq, void* dev_id) {
   struct latency_dev* ldev = (struct latency_dev*)dev_id;
   getnstimeofday(&ldev->irq_time);
   ldev->irq_fired = 1;
   gpio_set_value(ldev->lb.gpio_pin, 1);
   return IRQ_HANDLED;
}


void timer_handler(unsigned long ptr) {
   struct latency_dev* ldev = (struct latency_dev*)ptr;
   struct timespec delta;
   long long error;
   if(ldev->counter == ldev->lb.test){
        ldev->state = OFF;
        wake_up_interruptible(&ldev->readers_queue);
   }
   else{
    if (ldev->irq_fired) {
        ldev->counter++;
        delta = timespec_sub(ldev->irq_time, ldev->gpio_time); 
        
        // Calculate avg
        ldev->res.avg = ldev->res.avg ?
                (unsigned long)(((unsigned long long)delta.tv_nsec +
                    (unsigned long long)ldev->res.avg) >> 1) :
                    delta.tv_nsec;

            // Calculate error
            error = (ldev->res.avg - delta.tv_nsec);
            ldev->res.var =
                (unsigned long)(((unsigned long long)ldev->res.var +
                    (unsigned long long)error*error) >> 1);
            
            // Set min
            if((unsigned long) delta.tv_nsec < ldev->res.min) {
                ldev->res.min = delta.tv_nsec;
            }

            // Set max
            if((unsigned long) delta.tv_nsec > ldev->res.max) {
                ldev->res.max = delta.tv_nsec;   
            }
        }

        // Reconfigure timer
        ldev->irq_fired = 0; 
        mod_timer(&ldev->timer, jiffies + ldev->lb.period);
        getnstimeofday(&ldev->gpio_time);
        gpio_set_value(ldev->lb.gpio_pin, 0);
    }
}