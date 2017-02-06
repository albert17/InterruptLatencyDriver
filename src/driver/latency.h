#ifndef LATENCY_H
#define LATENCY_H

#include <linux/ioctl.h>
#include <linux/types.h>

// Device name
#define D_NAME "measure"
// IOCTL magic number
#define LATENCY_IOC_MAGIC (0xDA)

// List of available conmmands
enum io {
    SET,
    ON,
    OFF,
};

// pins and period information
struct latency_buffer{
     uint16_t irq_pin;
     uint16_t gpio_pin;
     int period;
     unsigned int test;
};

// Times information
struct latency_result{
     unsigned long min;
     unsigned long max;
     unsigned long avg;
     unsigned long var;
};


//IOCTL commands
#define ISET _IOW(LATENCY_IOC_MAGIC, SET, struct latency_buffer)
#define ION _IO(LATENCY_IOC_MAGIC, ON)

#endif