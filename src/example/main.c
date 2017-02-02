/**
 * Author: A. Alvarez
 * Date: 2017/01/30
 * Description: User program which uses the "measure" driver. 
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include "../driver/latency.h"
    
int main(int argc, char *argv[]) {
    int fd, res;
    struct latency_buffer *lb = (struct latency_buffer*) malloc(sizeof(struct latency_buffer));
    struct latency_result *result = (struct latency_result*) malloc(sizeof(struct latency_result));
    
    // Open device
    printf("[+] Openning device\n");
    fd = open("/dev/"D_NAME, O_RDWR);
    if (fd < 0) {
        printf("[-] ERROR: The device cannot be openned\n");
        free(lb);free(result);
        exit(1);
    }
    printf("[+] Openned device: fd == %d\n", fd);

   // Configure device
    lb->irq_pin = 915;
    lb->gpio_pin = 912;
    lb->period = 10;

    printf("[+] Configuring device\n");
    res = ioctl(fd,ISET,lb);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be configured\n");
        free(lb);free(result);
        exit(1);
   
    }
    printf("[+] Configured device\n");
    
    // Starting device
    printf("[+] Starting device\n");
    res = ioctl(fd,ION);
    if (res == -1) {
        free(lb);free(result);
        exit(1);
   
    }
    printf("[+] Started device\n");
    
    // Wait 10 seconds
    sleep(10);

    // Stoping device
    printf("[+] Stopping device\n");
    res = ioctl(fd,IOFF);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be stopped\n");
        free(lb);free(result);
        exit(1);
   
    }
    printf("[+] Stopped device\n");
    
    // Get time
    res = read(fd, result);
    if (res == -1) {
        printf("[-] ERROR: Time cannot be obtained\n");
        free(lb);free(result);
        exit(1);
    }
    printf("Result - Time MIN: %lu ns\n", result->min);
    printf("Result - Time MAX: %lu ns\n", result->max);
    printf("Result - Time AVG: %lu ns\n", result->avg);
    printf("Result - Time VAR: %lu ns\n", result->var);
    
    // Close device
    printf("[+] Closing device\n");
    close(fd);
    printf("[+] Closed device\n");
    
    // Free memory
    free(lb);free(result);
}