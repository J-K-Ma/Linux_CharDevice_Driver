KERNEL_PATH=/home/apprentice/Intelligent_Security_system/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga

ifeq ($(KERNELRELEASE),)

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules
	cp *.ko /nfs/rootfs/drv
	arm-linux-gnueabihf-gcc basic_char_test.c -o basic_char_test
	cp basic_char_test /nfs/rootfs/drv
clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
else
	obj-m += basic_char.o
endif
