#ifndef IRQ_H
#define IRQ_H

#include <linux/interrupt.h>
#include "latency.h"

#define AM33XX_CONTROL_BASE 0x44e10000
#define OUTPUT 0x7 | (2 << 3)
#define INPUT 0x7 | (2 << 3) | (1 << 5)

struct latency_dev {
   struct timespec gpio_time, irq_time;
   u16 irq;
   u16 irq_pin, gpio_pin;
   u8 irq_fired, irq_enabled;
   struct timer_list timer;
   unsigned long avg_nsecs, last_nsecs;
   unsigned int period;
   struct semaphore sem;
   enum io state;
   struct cdev cdev;
};

int setup_pinmux(struct latency_dev *latency_devp);
int configure_gpio_irq(struct latency_dev *latency_devp);
void release_gpio_irq(struct latency_dev *latency_devp);
irqreturn_t irq_handler(int irq, void* dev_id);
void timer_handler(unsigned long ptr);

#endif /* IRQ_H */