# export the environment variables CC and KDIR properly set
obj-m += irq_latency.o
default:
	make ARCH=arm CROSS_COMPILE=${CC} -C ${KDIR} M=`pwd` modules
clean:
	make ARCH=arm CROSS_COMPILE=${CC} -C ${KDIR} M=`pwd` clean