#include <linux/init.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");

#define LED_ON    _IOW('l', 1, int)
#define LED_OFF   _IOW('l', 0, int)
#define LED_BLINK _IOW('l', 3, int)

static struct class * cls = NULL;

struct led_msg {
	int gpio;
	/*
         0:关闭反转模式;
	 1:开启反转模式;
	*/
	int flag;
	/*
	 用于记录LED灯翻转状态
	*/
	int stat;
	dev_t dev_id;
	struct cdev drv_cdev;
	struct timer_list led_timer;
};

void tim_handler(unsigned long timer){
	struct timer_list* t = (struct timer_list*)timer;
	/*1.获取私有数据*/
	struct led_msg *led_pri = container_of(t,struct led_msg,led_timer);
	/*2.翻转电平*/
	if(led_pri -> stat){
		gpio_set_value(led_pri -> gpio, 1);
	}else{
		gpio_set_value(led_pri -> gpio, 0);
	}
	/*3.修改LED状态*/
	led_pri -> stat ^= (1<<0);
	/*4.判断是否继续闪烁*/
	if(led_pri -> flag){
		mod_timer(t,jiffies+HZ/10);
	}
}

int led_open(struct inode *inode, struct file *file){
	int ret = 0;
	struct led_msg* led_pri = container_of(inode->i_cdev,struct led_msg,drv_cdev);
	file -> private_data = led_pri;
	return ret;
}

int led_close(struct inode *inode, struct file *file){
	int ret = 0;
	return ret;
}

long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	int ret = 0;
	struct led_msg* led_pri = file -> private_data;
	if(cmd == LED_ON){
		led_pri -> flag = 0;
		gpio_set_value(led_pri -> gpio, 0);	
	}else if(cmd == LED_OFF){
		led_pri -> flag = 0;
		gpio_set_value(led_pri -> gpio, 1);
	}else if(cmd == LED_BLINK){
		led_pri -> flag = 1;
		mod_timer(&(led_pri -> led_timer),jiffies+HZ/10);
	}else{
		printk("Command error\n");
	}
	return ret;
}

static const struct file_operations led_ops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_close,
	.unlocked_ioctl = led_ioctl,
};

int led_probe(struct platform_device *platform_dev){
	int ret = 0;
	printk("enter %s \n",__func__);
	/*device_create函数返回值*/
	struct device* drv_device = NULL;
	/*获取设备信息struct device*/
	struct device* dev = &(platform_dev -> dev);
	/*分配私有信息结构体并绑定设备*/
	struct led_msg* led_pri = devm_kzalloc(dev, sizeof(struct led_msg), GFP_KERNEL);
	if (!led_pri){
		ret = -ENOMEM;
		goto malloc_err;
	}
	led_pri -> flag = 0;
	led_pri -> stat = 0;
	/*存储私有信息到设备信息struct device*/
	dev -> driver_data = led_pri;
	/*动态分配设备号*/
	if (alloc_chrdev_region(&(led_pri->dev_id), 0, 1,(dev->of_node->name))) {
		pr_err("QAT: unable to allocate chrdev region\n");
		ret = -EFAULT;
		goto alloc_chrdev_err;
	}
	/*初始化cdev*/
	cdev_init(&(led_pri -> drv_cdev), &led_ops);
	if (cdev_add(&(led_pri -> drv_cdev), led_pri->dev_id, 1)) {
		pr_err("QAT: cdev add failed\n");
		goto cdev_err;
	}
	/*创建设备节点*/
	drv_device = device_create(cls, NULL,
				   led_pri -> dev_id,
				   NULL, (dev->of_node->name));
	if(IS_ERR(drv_device)){
		ret = PTR_ERR(drv_device);
		goto device_err;
	}
	/*获取gpio管脚号*/
	led_pri -> gpio = of_get_named_gpio(dev->of_node,"led-gpio",0);
	/*申请gpio*/
	gpio_request(led_pri -> gpio,(dev->of_node->name));
	
	/*设置gpio输出*/
    	gpio_direction_output(led_pri -> gpio,1);

	/*设置超时时间*/
	/*
		HZ = 100 
		HZ/2 = 50;
	*/
	led_pri -> led_timer.expires = jiffies + HZ/2;
	led_pri -> led_timer.function = tim_handler;
	led_pri -> led_timer.data = (unsigned long)(&(led_pri -> led_timer));
	init_timer_key(&(led_pri -> led_timer),0,NULL,NULL);
	return ret;

device_err:
	cdev_del(&(led_pri -> drv_cdev));
cdev_err:
	unregister_chrdev_region(led_pri->dev_id, 1);
alloc_chrdev_err:
malloc_err:
	return ret;
}
int led_remove(struct platform_device *platform_dev){
	int ret = 0;
	printk("enter %s \n",__func__);
	struct device* dev = &(platform_dev -> dev);
	struct led_msg* led_pri = dev -> driver_data;

	del_timer(&(led_pri -> led_timer));
	gpio_direction_output(led_pri -> gpio,1);
	gpio_free(led_pri->gpio);         
	device_destroy(cls, led_pri->dev_id);
	cdev_del(&(led_pri -> drv_cdev));
	unregister_chrdev_region(led_pri->dev_id, 1);
	return ret;
}


static const struct of_device_id led_id[] = {
	{ .compatible = "mjk,leds",},
	{},
};

static struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name	= "leds",
		.of_match_table = led_id,
	},
};

static int __init LED_init(void){
	int ret;
        cls = class_create(THIS_MODULE, "mjk-led");
        if(IS_ERR(cls)){
                ret = PTR_ERR(cls);
                goto cls_err;
        }

	ret = platform_driver_register(&led_driver);	
	if(ret){
		printk(KERN_ERR "%s: failed to add led driver\n", __func__);
		goto reg_err;
	}
	return ret;
reg_err:
	class_destroy(cls);
cls_err:
	return ret;
}

static void __exit LED_exit(void){
	platform_driver_unregister(&led_driver);
  	class_destroy(cls);
}

module_init(LED_init);
module_exit(LED_exit);
