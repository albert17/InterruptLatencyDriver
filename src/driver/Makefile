# export the environment variables CC and KDIR properly set

obj-m := latency.o
latency-objs := irq.o

default:
	make ARCH=arm CROSS_COMPILE=${CC} -C ${KDIR} M=$(PWD) modules

clean:
	rm -rf *.o *~ core .*.cmd *.ko *.mod.c .*d .*tmp* *odule*