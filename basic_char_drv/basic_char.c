#include <linux/of.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");

/*设备类全局变量*/
static struct class *basic_char_class;

struct dev_data{
        dev_t dev_id;
        struct cdev drv_cdev;
};

static int dev_open(struct inode *inode, struct file *file){
	printk("hello,world!!!\n");
	return 0;
}
static int dev_close(struct inode *inode, struct file *file){
	printk("bye,bye\n");
	return 0;
}

static const struct file_operations dev_ops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_close,
};

int my_probe(struct platform_device *device){
	int ret = 0;
	/*获取设备树节点信息*/
	struct device_node* node = (device -> dev).of_node;
	struct dev_data *dev_pri = kzalloc(sizeof(struct dev_data),GFP_KERNEL);
	if(!dev_pri){
		ret = -ENOMEM;
		goto err_malloc;
	}
	/*使用设备树节点名作为设备号的名称以及设备节点的名称*/
	/*1.申请设备号*/
	if (alloc_chrdev_region(&(dev_pri -> dev_id), 0, 1, node -> name)) {
		ret = -EFAULT;
		goto err_chrdev_region;
	}
	/*2.申请cdev*/
	cdev_init(&(dev_pri -> drv_cdev), &dev_ops);
	if ((ret = cdev_add(&(dev_pri -> drv_cdev), dev_pri -> dev_id, 1))) {
		goto err_cdev;
	}
	/*3.申请设备节点*/
	struct device *dev = device_create(basic_char_class, &device -> dev, dev_pri -> dev_id, NULL, "%s", node -> name);
	if(IS_ERR(dev)){
		ret = PTR_ERR(dev);
		goto err_device;
	}
	/*4.将私有数据存储在platform_device中*/
	dev_set_drvdata(&(device -> dev), (void*)dev_pri);
	return 0;
err_device:
	cdev_del(&(dev_pri->drv_cdev));
err_cdev:
	unregister_chrdev_region(dev_pri -> dev_id, 1);
err_chrdev_region:
	kfree(dev_pri);
err_malloc:
	return ret;
}
int my_remove(struct platform_device *device){
	struct dev_data *dev_pri = (struct dev_data*)dev_get_drvdata(&(device -> dev));
	if(dev_pri == NULL){
		printk("remove error\n");
		return 0;
	}
	/*1.注销设备节点
	第一件事情是获取设备号;
	*/
	device_destroy(basic_char_class,dev_pri -> dev_id);
	/*2.注销cdev
	获取cdev
	*/
	cdev_del(&(dev_pri->drv_cdev));
	/*3.退出设备号*/
	unregister_chrdev_region(dev_pri -> dev_id, 1);
	/*4.释放设备信息结构体内存*/
	kfree(dev_pri);
	return 0;
}

/*注册总线设备*/
static const struct of_device_id my_of_match[] = {
	{ .compatible = "my_test" },
	{}
};

static struct platform_driver my_driver = {
	.driver = {
		.name	= "my_test",
		.owner	= THIS_MODULE,
		.of_match_table	= my_of_match,
	},
	.probe		= my_probe,
	.remove		= my_remove,
};

static int __init basic_char_init(void){
	/*1.注册设备类*/
	int ret = 0;
	basic_char_class = class_create(THIS_MODULE, "basic_char");
	if (IS_ERR(basic_char_class)) {
		ret = PTR_ERR(basic_char_class);
		goto err_class;
	}
	/*2.注册platform总线*/
	ret = platform_driver_register(&my_driver);
	if(ret){
		goto err_platform;
	}
	return 0;
err_platform:
	class_destroy(basic_char_class);
err_class:
	printk("fail\n");
	return ret;
}
static void __exit basic_char_exit(void){
	/*1.注销platform总线*/
	platform_driver_unregister(&my_driver);
	/*2.注销设备类*/
	class_destroy(basic_char_class);
}

module_init(basic_char_init);
module_exit(basic_char_exit);
