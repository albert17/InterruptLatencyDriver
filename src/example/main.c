#include <stdlib.h>
#include <stdio.h>
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
    printf("[+] Configuring device\n");
    res = ioctl(SET,fd, 48, 60, 10);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be configured\n");
        exit(1);
    }
    // Starting device
    res = ioctl(ON,fd, 48, 60, 10);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be started\n");
        exit(1);
    }
    
    // Stoping device
    res = ioctl(OFF,fd, 48, 60, 10);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be stopped\n");
        exit(1);
    }
    
    // Wait 1 seconds
    sleep(1);

    // Get last time
    res = read(fd, LAST);
    if (res == -1) {
        printf("[-] ERROR: LAST time cannot be obtained\n");
    }
    // Get avg time
    res = read(fd, AVG);
    if (res == -1) {
        printf("[-] ERROR: AVG time cannot be obtained\n");
    }
    // Close device
    close(fd);
}