PATH_KERNEL=/home/apprentice/Intelligent_Security_system/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga

ifeq ($(KERNELRELEASE),)

all:
	make -C $(PATH_KERNEL) M=$(PWD) modules
	cp *.ko /nfs/rootfs/drv
	arm-linux-gnueabihf-gcc led_drv_test.c -o led_drv_test
	cp led_drv_test /nfs/rootfs/drv
clean:
	make -C $(PATH_KERNEL) M=$(PWD) clean
else
	obj-m += led_drv.o
endif
