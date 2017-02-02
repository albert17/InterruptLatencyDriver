#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>

#include "../driver/latency.h"
    
int main(int argc, char *argv[]) {
    int fd, res;
    // Open device
    printf("[+] Openning device\n");
    fd = open("/dev/measure", O_RDWR);
    if (fd < 0) {
        printf("[-] ERROR: The device cannot be openned\n");
        exit(1);
    }
    printf("[+] Openned device: fd == %d\n", fd);

    struct latency_buffer *lb;
    lb = (struct latency_buffer*) malloc(sizeof(struct latency_buffer));
   // Configure device
    lb->irq_pin = 915;
    lb->gpio_pin = 912;
    lb->period = 10;

    printf("[+] Configuring device\n");
    res = ioctl(fd,ISET,lb);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be configured\n");
        exit(1);
    }
    printf("[+] Configured device\n");
    
    // Starting device
    printf("[+] Starting device\n");
    res = ioctl(fd,ION);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be started\n");
        exit(1);
    }
    printf("[+] Started device\n");
    
    // Wait 1 seconds
    sleep(1);

    // Stoping device
    printf("[+] Stopping device\n");
    res = ioctl(fd,IOFF);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be stopped\n");
        exit(1);
    }
    printf("[+] Stopped device\n");
    
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
    printf("[+] Closing device\n");
    close(fd);
    printf("[+] Closed device\n");
}