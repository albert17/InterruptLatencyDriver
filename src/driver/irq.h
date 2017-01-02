#ifndef IRQ_H
#define IRQ_H

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

static int setup_pinmux(void);
static int configure_gpio_irq(void);
static void release_gpio_irq(void);
static irqreturn_t test_irq_latency_interrupt_handler(int irq, void* dev_id);
static void test_irq_latency_timer_handler(unsigned long ptr);

#endif /* IRQ_H */