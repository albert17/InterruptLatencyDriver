#ifndef LATENCY_H
#define LATENCY_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Defines */
#define D_NAME "measure"
#define LATENCY_IOC_MAGIC (0xDA)

/* Structures */
enum io {
    SET,
    ON,
    OFF,
};

struct latency_buffer{
     uint16_t irq_pin;
     uint16_t gpio_pin;
     int period;
};

#define ISET _IOW(LATENCY_IOC_MAGIC, SET, struct latency_buffer)
#define ION _IO(LATENCY_IOC_MAGIC, ON)
#define IOFF _IO(LATENCY_IOC_MAGIC, OFF)

#endif