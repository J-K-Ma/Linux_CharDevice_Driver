#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");

struct dev_msg{
	int irq_id;
	int code;
	int gpio_id;
	struct timer_list timer_device;
	struct input_dev* input_device;
};

struct dev_set{
	int cnt; //子节点的个数;
	struct input_dev* input_device;
	struct dev_msg* key_msg;
};

irqreturn_t exti_trigger_handler(int irq, void *arg){
	struct dev_msg *key_msg = (struct dev_msg*)arg;
	mod_timer(&(key_msg->timer_device),jiffies + HZ/50);
	return IRQ_HANDLED;
}


void timer_handler(unsigned long arg){
	struct dev_msg* key_msg = (struct dev_msg*)arg;
	int key_stat = gpio_get_value(key_msg->gpio_id);
        input_event(key_msg->input_device,EV_KEY,key_msg->code,!key_stat);
        input_event(key_msg->input_device,EV_SYN,0,0);

}

int key_probe(struct platform_device *platform_dev){
	int i = 0;                          //循环变量;
	int ret = 0;                        //返回值;
	int cnt = 0;                        //临时计数值;
	struct dev_set* key_set = NULL;     //按键节点信息结构体;
	struct device_node *child = NULL;   //子节点临时变量;

	struct device_node *set_node = platform_dev->dev.of_node; //这里获取set节点的device_node结构体
	
	/*获取子节点个数*/
	while((child = of_get_next_child(set_node,child))){
		cnt++;
	}

	printk("cnt:%d\n",cnt);
	
	/*分配dev_set结构体地址*/
	key_set = devm_kzalloc(&platform_dev->dev,sizeof(struct dev_set),GFP_KERNEL);
	if(!key_set){
		printk("%s kzalloc is fail\n",__func__);
		ret = -ENOMEM;
		goto key_set_alloc_fail;
	}

	key_set -> cnt = cnt;	

	/*分配dev_set->dev_msg结构体内存*/
	key_set -> key_msg = devm_kzalloc(&platform_dev->dev,sizeof(struct dev_msg)*(key_set -> cnt),GFP_KERNEL);
	if(!(key_set -> key_msg)){
		printk("%s kzalloc is fail\n",__func__);
		ret = -ENOMEM;
		goto key_msg_alloc_fail;
	}

	/*保存私有数据*/
	platform_dev->dev.driver_data = (void*)key_set;

	/*注册input子系统*/
	key_set -> input_device = devm_input_allocate_device(&(platform_dev->dev));
	if(!key_set -> input_device){
		ret = -ENOMEM;
		goto input_alloc_fail;
 	}

	/*初始化input_dev*/
	key_set -> input_device -> name = "mjk,key";
	
	/*设置事件*/
	__set_bit(EV_SYN,key_set->input_device->evbit);
	__set_bit(EV_KEY,key_set->input_device->evbit);
	
	if(of_property_read_bool(platform_dev->dev.of_node,"autorepeat"))
		__set_bit(EV_REP,key_set->input_device->evbit);
	
        ret = input_register_device(key_set->input_device);
        if(ret){
                pr_err("input_register is fail\n");
                goto input_register_fail;
        }

	for(i=0;i<key_set->cnt;i++){
		/*获取子节点*/
		child = of_get_next_child(set_node,child);
		/*获取键值*/
		ret = of_property_read_u32_index(child, "linux,code", 0, &(key_set->key_msg[i].code));
		if(ret){
			pr_err("can't get 'linux,code' %d\n", ret);
			ret = -EINVAL;
			goto of_property_read_fail;
		}
		/*设置键值*/
		__set_bit(key_set->key_msg[i].code,key_set->input_device->keybit);
		/*初始化软件定时器*/
        	key_set -> key_msg[i].timer_device.expires = jiffies + HZ/2;
		/*注册软件定时器*/    
		setup_timer(&(key_set->key_msg[i].timer_device)
				, timer_handler
				, (unsigned long)&key_set->key_msg[i]);
	        /*获取GPIO的id值*/
		key_set -> key_msg[i].gpio_id = of_get_named_gpio_flags(child
									, "key-gpio"
									, 0
									, NULL);
		if (key_set->key_msg[i].gpio_id < 0) {
    			ret = key_set->key_msg[i].gpio_id;
    			pr_err("can't get 'key-gpio': %d\n", ret);
    			goto of_get_name_flags_fail;
		}

		/*申请gpio子系统*/
		ret = devm_gpio_request(&(platform_dev->dev),key_set -> key_msg[i].gpio_id, child->name);
		if(ret){
			pr_err("can't alloc gpio\n");
			goto devm_gpio_fail;
		}
		/*设置gpio子系统为输出*/
		gpio_direction_input(key_set -> key_msg[i].gpio_id);
            	/*获取中断号*/
		key_set->key_msg[i].irq_id = of_irq_get(child, 0);
		if (key_set->key_msg[i].irq_id < 0) {
    			ret = key_set->key_msg[i].irq_id;
    			pr_err("of_irq_get failed: %d\n", ret);
    			goto of_irq_get_fail;
		}
                /*申请中断*/
                ret = devm_request_irq(&(platform_dev->dev)
                                ,key_set->key_msg[i].irq_id
                                ,exti_trigger_handler
                                ,IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING
                                ,platform_dev->dev.of_node->name
                                ,(void*)&key_set->key_msg[i]);
		if(ret){
			pr_err("devm_request_irq is fail\n");
			goto devm_request_irq;
		}
		/*传递input_dev地址*/
		key_set->key_msg[i].input_device = key_set->input_device; 
	}

	return ret;
input_register_fail:
devm_request_irq:
of_irq_get_fail:
devm_gpio_fail:
of_get_name_flags_fail:
of_property_read_fail:
	while(--i>=0){ 
		del_timer_sync(&(key_set->key_msg[i].timer_device));
	}
input_alloc_fail:
key_msg_alloc_fail:
key_set_alloc_fail:
	return ret;
}
int key_remove(struct platform_device *platform_dev){
	int i = 0;
	int ret = 0;
	/*取出私有数据*/
	struct dev_set* key_set = platform_dev -> dev.driver_data;
	for(i=0;i<key_set->cnt;i++){
		disable_irq(key_set->key_msg[i].irq_id);
		del_timer_sync(&(key_set->key_msg[i].timer_device));
	}
	return ret;
}


static const struct of_device_id key_id[] = {
	{ .compatible = "mjk,keys",},
	{},
};

static struct platform_driver key_driver = {
	.probe = key_probe,
	.remove = key_remove,
	.driver = {
		.name	= "input",
		.of_match_table = key_id,
	},
};

static int __init KEY_init(void){
	int ret;

	ret = platform_driver_register(&key_driver);	
	if(ret){
		printk(KERN_ERR "%s: failed to add key driver\n", __func__);
	}
	return ret;
}

static void __exit KEY_exit(void){
	platform_driver_unregister(&key_driver);
}

module_init(KEY_init);
module_exit(KEY_exit);
