#include <stdlib.h>
#include <stdio.h>

#define DEVICENAME "latency"
#define LAST 0
#define AVG 1

int main(int argc, char *argv[]) {
    int fd, res;
    // Open device
    printf("[+] Openning device\n");
    fd = open(DEVICENAME);
    if (fd == -1) {
        printf("[-] ERROR: The device cannot be openned\n")
        exit(1);
    }
    // Configure device
    printf("[+] Configuring device\n");
    res = ioctl(fd, 48, 60, 10);
    if (res == -1) {
        printf("[-] ERROR: The device cannot be configured\n")
        exit(1);
    }
    // Wait 1 seconds
    sleep(1);
    // Get last time
    res = read(fd, LAST);
    if (res == -1) {
        printf("[-] ERROR: LAST time cannot be obtained\n")
    }
    // Get avg time
    res = read(fd, AVG);
    if (res == -1) {
        printf("[-] ERROR: AVG time cannot be obtained\n")
    }
    // Close device
    close(fd);
}