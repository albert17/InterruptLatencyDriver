# InterruptLatencyDriver
InterruptLatencyDriver is a linux driver which measures the interrupt latency using the gpio.

[![Build Status](https://travis-ci.org/albert17/InterruptLatencyDriver.svg)](https://travis-ci.org/albert17/InterruptLatencyDriver)
## Usage
### Compile
#### Compile driver
Export KDIR variable with the kernel path
```sh
$ export KDIR=/path/to/kernel-source
```

Export CC variable with the compiler path
```sh
$ export CC=/path/to/compiler
```

Compile the module
```sh
$ cd src/driver
$ make
```

#### Compile example
Export LINARO variable with the compiler path
```sh
$ export LINARO=/path/to/compiler
```

Compile the example
```sh
$ cd src/example
$ make
```
### Execute
## Platform
This software has been tested is the following platforms:
* Beaglebone - Linux kernel: 3.8.13-rt
 