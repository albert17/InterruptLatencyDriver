#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../driver/latency.h"

int main(int argc, char *argv[]) {
    int fd, res;
    // Open device
    printf("[+] Openning device\n");
    fd = open(D_NAME);
    if (fd == -1) {
        printf("[-] ERROR: The device cannot be openned\n");
        exit(1);
    }
    // Configure device
    struct latency_buffer *lb;
    lb->irq_pin = 48;
    lb->gpio_pin = 60;
    lb->period = 10;
    printf("[+] Configuring device\n");
    res = ioctl(fd,ISET,lb);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be configured\n");
        exit(1);
    }
    // Starting device
    printf("[+] Starting device\n");
    res = ioctl(fd,ION);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be started\n");
        exit(1);
    }
    
    // Wait 1 seconds
    sleep(1);

    // Stoping device
    printf("[+] Stopping device\n");
    res = ioctl(fd,IOFF);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be stopped\n");
        exit(1);
    }

    // Get time
    char result[128];
    res = read(fd, &result);
    if (res == -1) {
        printf("[-] ERROR: Time cannot be obtained\n");
    }
    else {
        printf("Result: %s", result);
    }
    // Close device
    close(fd);
}