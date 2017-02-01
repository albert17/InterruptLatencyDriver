#ifndef LATENCY_H
#define LATENCY_H

/* Defines */
#define D_NAME "latency"
#define LATENCY_IOC_MAGIC '$'

/* Structures */
enum io {
    SET,
    ON,
    OFF,
};

struct latency_buffer{
     u16 irq_pin;
     u16 gpio_pin;
     int period;
};

#define ISET _IOW(LATENCY_IOC_MAGIC, SET, struct latency_buffer)
#define ION _IO(LATENCY_IOC_MAGIC, ON, struct latency_buffer)
#define IOFF _IO(LATENCY_IOC_MAGIC, OFF, struct latency_buffer)

#endif