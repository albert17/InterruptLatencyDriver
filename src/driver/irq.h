#ifndef IRQ_H
#define IRQ_H

#define DEVICENAME "latency"
#define AM33XX_CONTROL_BASE 0x44e10000
#define OUTPUT 0x7 | (2 << 3)
#define INPUT 0x7 | (2 << 3) | (1 << 5)

/* Structures */
struct latency_dev {
   struct timespec gpio_time, irq_time;
   u16 irq;
   u16 irq_pin, gpio_pin;
   u8 irq_fired, irq_enabled;
   struct timer_list timer;
   u32 test_count;
   unsigned long avg_nsecs, missed_irqs;
   unsigned long last_nsecs;
   unsigned int period;
   struct semaphore sem;
   struct cdev cdev;
};

static int setup_pinmux(struct latency_dev *latency_devp);
static int configure_gpio_irq(struct latency_dev *latency_devp);
static void release_gpio_irq(struct latency_dev *latency_devp);
static irqreturn_t test_irq_latency_interrupt_handler(int irq, void* dev_id);
static void test_irq_latency_timer_handler(unsigned long ptr);

#endif /* IRQ_H */