==================================================
INPUT Subsystem Platform Driver README
==================================================

1. Linux 内核源码版本
Linux 4.1.15

2. 代码功能
   本驱动基于 Linux 4.1.15，整合 GPIO、中断、软件定时器、input 子系统与设备树，
   实现一个支持多按键、可动态扩展的输入驱动。

   主要特性：

    （1）父子节点分层：设备树父节点对应 struct dev_set，子节点对应
   struct dev_msg。驱动按子节点个数动态分配 dev_msg 数组，
   一份驱动服务多个按键实例，新增按键只需修改设备树。

    （2）中断 + 定时器延时消抖：中断处理仅调用 mod_timer() 推迟
    HZ/50（约 20 ms）后再采样，避开机械抖动窗口；双边沿触发
 （IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING）可同时捕获按下与抬起。

    （3）input 子系统上报：定时器回调读取 GPIO 电平后，通过
    input_event() 上报 EV_KEY 与 EV_SYN。所有按键共享同一个
    input_dev，以 code 区分。

   （4）AUTOREPEAT 支持：根据设备树 autorepeat 属性动态设置
    EV_REP，启用长按重复上报。

   （5）涉及的子系统：GPIO、中断、软件定时器、input、设备树 OF。

  主要函数：

    KEY_init() / KEY_exit()               模块加载与卸载
    key_probe() / key_remove()            平台驱动
    exti_trigger_handler()                中断处理（登记定时器）
    timer_handler()                       定时器回调（采样并上报）
    devm_input_allocate_device()          input 设备分配
    input_register_device() / input_event() input 注册与上报
    of_get_named_gpio_flags() / devm_gpio_request()  GPIO 获取与申请
    of_irq_get() / devm_request_irq()     中断号获取与申请
    setup_timer() / mod_timer() / del_timer_sync()  软件定时器
3. 编写目的
   本代码的主要目标有三：

   （1）系统性掌握 input 子系统
   相较于 LED 等简单字符驱动，input 驱动的调用链更长
 （中断 → 定时器 → input → 用户空间），涉及子系统更多，
   是一次较为完整的综合练习。

    （2）探索可动态扩展、多实例化的驱动写法
    尽量通过设备树子节点数动态决定资源数量，做到"增加一个子节点
    就多一个按键"，避免将按键数量、GPIO、键值写死在代码里。

   （3）在陌生版本内核中独立查找可用 API
    刻意不照抄教程，而是在 4.1.15 源码树中逐个确认 API 的签名与
    语义。过程中遇到若干问题：

  （4）gpiod_* 函数族在 4.1.15 尚不完善，部分接口尚未稳定，
    最终回退到老式的 gpio_* 接口（devm_gpio_request +
    gpio_direction_input + gpio_get_value）。

  （5）定时器 API 在不同版本差异显著：4.1 使用 setup_timer()，
    回调签名为 void (*)(unsigned long)，需通过 data 字段
    传入宿主结构体地址；4.15 之后改为 timer_setup() +
    from_timer()。这让我意识到内核 API 并非稳定接口，
    写驱动前必须先确定目标版本。

  （6）中断与 input 注册的时序问题：最初将
    input_register_device() 放在中断注册之后，导致中断可能
    在 input_dev 尚未注册时触发。修正为先注册 input，再打开
    中断源。

  （7）错误路径的清理边界：while(--i >= 0) 与 while(i-- >= 0)
   的差异、删除未初始化定时器在 CONFIG_DEBUG_OBJECTS_TIMERS
   下的告警，都需要实际编译运行后才能发现。

4. 目前缺陷
   缺少并发保护
   中断、定时器回调、remove 路径之间存在潜在竞态。定时器回调
   运行于软中断上下文，不能睡眠，若引入运行时状态修改需使用
   spin_lock_irqsave() 或 atomic_t，不能使用 mutex。

5. 设备树节点写法

  input {
        compatible = "mjk,keys";
        autorepeat;

        key-1 {
                label = "KEY UP";
                interrupt-parent = <&gpio1>;
                interrupts = <18 1>;
                linux,code = <KEY_UP>;
                key-gpio = <&gpio1 18 0>;
        };

        key-2 {
                label = "KEY DOWN";
                interrupt-parent = <&gpio1>;
                interrupts = <19 1>;
                linux,code = <KEY_DOWN>;
                key-gpio = <&gpio1 19 0>;
        };
  };

说明：

   父节点 compatible 与驱动 of_device_id 匹配，autorepeat
为可选布尔属性；

   每个子节点描述一个按键：中断控制器与中断号、键值、GPIO、可选 label；

   增加子节点即可扩展按键，无需修改驱动。

6. 设备树写法查找路径

Documentation/devicetree/bindings/input/gpio-beeper.txt
Documentation/devicetree/bindings/input/gpio-keys-polled.txt
Documentation/devicetree/bindings/interrupt-controller/interrupts.txt

input 子系统本身没有专属的设备树绑定——它是对底层驱动（GPIO、
中断、I2C、SPI 等）之上的一层抽象封装。参考 gpio-keys 等
上游驱动的绑定文档，可使设备树节点更加规范。

7. API 确定路径

include/linux/timer.h             软件定时器
include/linux/of_gpio.h           设备树 GPIO 获取
include/linux/gpio.h              GPIO 操作
include/linux/gpio/consumer.h     GPIO descriptor 新接口（4.1 中不完善）
include/linux/interrupt.h         中断申请与处理
include/linux/input.h             input 子系统核心 API

8. 核心结构体
struct input_dev — input 设备抽象
关键字段：name（设备名）、evbit[]（支持的事件类型）、
keybit[]（支持的键值）、open() / close()（打开与关闭回调）。
本驱动通过 __set_bit() 登记 EV_KEY 及各键值，再由
input_event() 完成上报。

struct timer_list — 软件定时器
关键字段：expires（到期时间）、function（回调）、
data（回调参数）、entry / base（内核内部使用）。

本驱动通过：


setup_timer(&key_msg[i].timer_device, timer_handler,
            (unsigned long)&key_msg[i]);

将宿主 dev_msg 地址传入 data，回调中强转即可取得上下文。

struct dev_set / struct dev_msg — 驱动私有数据
text
struct dev_set {
    int              cnt;            // 按键个数
    struct input_dev *input_device;  // 所有按键共享
    struct dev_msg   *key_msg;       // dev_msg 数组
};

struct dev_msg {
    int              irq_id;         // 中断号
    int              code;           // 键值
    int              gpio_id;        // GPIO 号
    struct timer_list timer_device;  // 每键独立消抖定时器
    struct input_dev *input_device;  // 指向共享 input_dev
};
"父结构体管理共享资源、子结构体管理独立资源"的分层设计，是
本驱动多实例、可扩展的基础。

9. 测试方式
  Linux 将 input 事件以 struct input_event 的形式暴露在
  /dev/input/eventX。字段为：

  struct input_event {
    struct timeval time;   /* 时间戳 */
    __u16 type;            /* EV_KEY / EV_SYN ... */
    __u16 code;            /* KEY_UP 等 */
    __s32 value;           /* 1 按下，0 抬起，2 自动重复 */
  };

10. 使用步骤
  加载驱动后确认设备节点：

  cat /proc/bus/input/devices
  找到 mjk,key 对应的 eventX。

  用 hexdump 实时观察事件：

  hexdump -C /dev/input/event2

  按下 / 抬起时会看到 EV_KEY（0x01）与 EV_SYN（0x00）
  两组事件，value 分别为 1 与 0。

11. 测试用例
      单键：按一次 KEY_UP，应上报一组 EV_KEY + EV_SYN；

      长按：若设备树有 autorepeat;，长按将周期性出现
      value = 2 的重复事件；

      多键：同时按 KEY_UP 与 KEY_DOWN，事件互不干扰，

==================================================
结束
==================================================