# export the environment variables CC and KDIR properly set

obj-m := latency.o
latency-objs := irq.o
#ccflags-y := -I$(src)/

default:
	make ARCH=arm CROSS_COMPILE=${CC} -C ${KDIR} M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .*d .*tmp