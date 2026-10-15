==================================================
LED Platform Driver README
==================================================

1. Linux 内核源码版本
   Linux 4.1.15

2. 代码的功能
   本驱动使用 Linux GPIO 子系统和软件定时器，实现基于设备树的 LED 控制。
   支持通过字符设备节点使用 ioctl 控制：
   - LED_ON：关闭定时器，输出固定电平
   - LED_OFF：关闭定时器，输出固定电平
   - LED_BLINK：启动软件定时器，周期性翻转 GPIO，实现闪烁

   框架主要函数：
   (1) 模块加载/卸载
       LED_init()、LED_exit()

   (2) 平台驱动
       led_probe()、led_remove()
       platform_driver_register()、platform_driver_unregister()
       of_device_id、of_match_table、compatible = "mjk,leds"

   (3) 字符设备
       alloc_chrdev_region()、cdev_init()、cdev_add()
       device_create()、cdev_del()、unregister_chrdev_region()、device_destroy()
       led_open()、led_close()、led_ioctl()

   (4) GPIO 子系统
       of_get_named_gpio()、gpio_request()、gpio_direction_output()
       gpio_set_value()、gpio_free()

   (5) 软件定时器
       init_timer_key()、mod_timer()、del_timer()
       tim_handler()、container_of()

   (6) 类与私有数据
       class_create()、class_destroy()
       devm_kzalloc()、container_of()

   (7) ioctl 命令
       LED_ON、LED_OFF、LED_BLINK

3. 写这个代码的目的
   为了展示自己认为比较标准的、复用性比较高的代码，
   一直在努力去提高代码的多实例化。
   通过 platform_driver、设备树匹配、devm_kzalloc、container_of 和
   每设备私有数据结构 struct led_msg，使多个 LED 设备节点可以复用同一套驱动。

4. 代码目前的缺陷
   对于多个进程访问设备节点没有进行保护。
   多个进程同时调用 ioctl，或者 ioctl 与软件定时器回调并发执行时，
   可能同时读写 led_pri->flag、led_pri->stat、GPIO 和定时器状态，
   从而造成状态竞争、闪烁异常或定时器重复修改。
   后续可考虑加入互斥锁、自旋锁或原子操作进行并发保护。
   注意：软件定时器回调属于中断/软中断上下文，不能睡眠，若加锁需选择合适锁类型。

5. 设备树节点写法
   led-1 {
           compatible = "mjk,leds";
           led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>;
   };

   led-2 {
           compatible = "mjk,leds";
           led-gpio = <&gpio5 1 GPIO_ACTIVE_LOW>;
   };

6. 设备树节点写法查找的路径
   Documentation/devicetree/bindings/gpio/gpio.txt（gpio）

7. 软件定时器 API 确定路径
   include/linux/timer.h
   kernel/time/timer.c

8. 软件定时器的核心结构体
   struct timer_list
   主要字段：
   - entry：定时器链表节点
   - expires：定时器到期时间，通常为 jiffies + 延时
   - function：定时器超时回调函数
   - data：传递给回调函数的参数
   - base：定时器内部管理基址

   本代码中使用：
   led_pri->led_timer.expires  = jiffies + HZ/2;
   led_pri->led_timer.function = tim_handler;
   led_pri->led_timer.data     = (unsigned long)(&(led_pri->led_timer));
   init_timer_key(&(led_pri->led_timer), 0, NULL, NULL);

9. 软件定时器频率的设置路径
   make menuconfig ---> Kernel Features ---> Timer frequency
   可配置系统 HZ 值，例如 100Hz、250Hz、1000Hz 等。
   HZ 会影响 jiffies 的时间粒度，也会影响代码中 HZ/10、HZ/2 的实际延时。

10. 测试代码介绍
    文件名：test_led.c
    功能：用户空间测试程序，通过 ioctl 向 LED 字符设备节点发送控制命令，
          实现对单个 LED 的亮、灭、闪烁操作，可用于验证驱动的多实例和基本功能。

    编译方法：
      本地编译：gcc -o test_led test_led.c
      交叉编译：arm-linux-gnueabihf-gcc -o test_led test_led.c

    用法：
      ./test_led <设备节点> <on|off|blink>

    示例：
      ./test_led /dev/led-1 on       // 点亮 LED
      ./test_led /dev/led-1 off      // 熄灭 LED
      ./test_led /dev/led-1 blink    // 启动闪烁

    命令说明：
      on    -> 发送 LED_ON   宏，对应驱动中关闭定时器并输出固定电平
      off   -> 发送 LED_OFF  宏，对应驱动中关闭定时器并输出固定电平
      blink -> 发送 LED_BLINK 宏，对应驱动中启动软件定时器持续翻转 GPIO

    注意事项：
      (1) 设备节点名称取决于设备树节点名，例如 led-1 对应 /dev/led-1。
      (2) 普通用户可能没有访问权限，可使用 sudo 或修改设备节点权限。
      (3) 发送 blink 后 LED 会持续闪烁，发送 on 或 off 会停止定时器并设置固定电平。
      (4) 可同时对 /dev/led-1 和 /dev/led-2 操作，验证驱动的多实例复用能力。
      (5) 由于驱动目前没有并发保护，可尝试多个进程同时操作同一设备节点，
          观察是否出现闪烁紊乱等现象，以验证 README 第 4 节提到的缺陷。

==================================================
结束
==================================================