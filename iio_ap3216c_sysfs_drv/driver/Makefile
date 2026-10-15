KERNEL_PATH=/home/apprentice/Intelligent_Security_system/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga

ifeq ($(KERNELRELEASE),)

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules
	cp *.ko /nfs/rootfs/drv
clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
else
	obj-m	+= ap3216c_drv.o
	ap3216c_drv-objs := ap3216c_main.o ap3216c.o
endif
