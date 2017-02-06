/**
 * Author: A. Alvarez
 * Date: 2017/01/30
 * Description: Defines pins configuration, and irq and timer handlers.
 * interrupt latency. 
 */

#ifndef IRQ_H
#define IRQ_H

#include <linux/interrupt.h>

#include "latency.h"

// ARM control base value
#define AM33XX_CONTROL_BASE 0x44e10000
// Pin as output value
#define OUTPUT 0x7 | (2 << 3)
// Pin as input value
#define INPUT 0x7 | (2 << 3) | (1 << 5)

// latency_dev structure
struct latency_dev {
   wait_queue_head_t readers_queue;
   struct timespec gpio_time, irq_time;
   u16 irq;
   u8 irq_fired, irq_enabled;
   unsigned int counter;
   struct timer_list timer;
   enum io state;
   struct latency_buffer lb;
   struct latency_result res;
   struct cdev cdev;
   struct device dev;
};

/**
 * Configures the pins incated in ldev structure
 * @param ldev structure with all the contents
 * @return 0 in success, negative in fail.
 */
int setup_pinmux(struct latency_dev *ldev);

/**
 * Configure the gpio and the irq.
 * @param ldev structure with all the contents
 * @return 0 in success, negative in fail.
 */
int configure_gpio_irq(struct latency_dev *ldev);

/**
 * Release the gpio and the irq.
 * @param ldev structure with all the contents
 */
void release_gpio_irq(struct latency_dev *ldev);

/**
 * Handles the irq setting a flag in the structure.
 * @param irq irq interrupt number
 * @param dev_id ldev structure with all the contents
 * @return irq handled code
 */
irqreturn_t irq_handler(int irq, void* dev_id);

/**
 * Handles timer interrrupt, calculating the times and 
 * reconfiguring the timer.
 * @param
 */
void timer_handler(unsigned long ptr);

#endif /* IRQ_H */