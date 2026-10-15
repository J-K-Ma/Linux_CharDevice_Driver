==================================================
AP3216C IIO I2C Driver README
==================================================

1. Linux 内核源码版本
Linux 4.1.15

2. 代码功能

   本驱动基于 Linux 4.1.15，使用 I2C 子系统、IIO 子系统、sysfs 设备属性
   与设备树 OF，实现 AP3216C 环境光 ALS、接近 PS、红外 IR 传感器的驱动。

   文件组织：

       i2c-common.h      通用 I2C 多字节读 / 多字节写封装
       ap3216c.h         AP3216C 寄存器、模式、延时、函数声明
       ap3216c.c         AP3216C 寄存器级操作：模式设置、数据读取、初始化
       ap3216c_main.c    I2C 驱动入口、IIO 设备注册、sysfs 属性
       Makefile          内核模块编译，链接 ap3216c_main.o + ap3216c.o

   主要特性：

    （1）三层分层解耦

        第一层：I2C 通用读写层，位于 i2c-common.h。
        实现 i2c_write_reg() / i2c_read_reg()，用 i2c_msg + i2c_transfer
        封装“寄存器地址 + 数据”的读写过程。该层不关心 AP3216C 的具体寄存器，
        只负责 I2C 多字节传输。

        第二层：AP3216C 器件操作层，位于 ap3216c.c / ap3216c.h。
        在通用 I2C 读写之上，封装传感器语义：
            ap3216c_set_mode()
            ap3216c_get_mode()
            ap3216c_read_als()
            ap3216c_read_ps()
            ap3216c_read_ir()
            ap3216c_init()
        该层只关心 AP3216C 的寄存器含义、位域组合、转换时间。

        第三层：用户态与内核态交互层，位于 ap3216c_main.c。
        使用 IIO 子系统完成 iio_dev 分配、通道配置、read_raw 回调注册、
        sysfs 属性注册。通过 Makefile 将 ap3216c_main.o 与 ap3216c.o
        链接为 ap3216c_drv.ko：

            obj-m += ap3216c_drv.o
            ap3216c_drv-objs := ap3216c_main.o ap3216c.o

    （2）IIO 标准接口读取数据

        驱动注册 IIO 设备，分配 iio_dev，设置 iio_chan_spec 通道：
            IIO_LIGHT       对应 ALS 环境光
            IIO_PROXIMITY   对应 PS 接近
            IIO_INTENSITY   配合 IIO_MOD_LIGHT_IR 对应 IR 红外

        每个通道通过 info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
        暴露 RAW 数据，用户态使用标准 IIO sysfs 接口读取：

            cat /sys/bus/iio/devices/iio:deviceX/in_light_raw
            cat /sys/bus/iio/devices/iio:deviceX/in_proximity_raw
            cat /sys/bus/iio/devices/iio:deviceX/in_intensity_ir_raw

        实际文件名以开发板 ls 结果为准。read_raw 回调中根据通道类型分派到
        ap3216c_read_als() / ap3216c_read_ps() / ap3216c_read_ir()。

    （3）sysfs 非标准接口设置模式

        驱动使用 DEVICE_ATTR_RW(mode_iio) 创建 mode_iio 属性，
        通过 mode_iio_show() / mode_iio_store() 读写 AP3216C 系统模式寄存器。
        用户态使用：

            cat /sys/bus/iio/devices/iio:deviceX/mode_iio
            echo 3 > /sys/bus/iio/devices/iio:deviceX/mode_iio

        该接口不是 IIO 标准通道接口，而是驱动私有 sysfs 属性，用于设置
        AP3216C 的 POWER_DOWN、ALS_ONLY、PS_IR、ALS_PS_IR、SW_RESET、
        单次测量等模式。

    （4）设备树匹配与 I2C 从设备注册

        父节点为 I2C 控制器节点，AP3216C 作为 I2C 从设备挂在 &i2c1 下。
        compatible = "mjk,ap3216c" 与 of_device_id 匹配，reg = <0x1e>
        对应 AP3216C 的 7 位 I2C 从机地址 0x1E。

    （5）涉及的子系统

        I2C 子系统、IIO 子系统、sysfs / 设备属性、设备树 OF、
        Linux 设备模型、内核模块机制。

   主要函数：

        i2c_write_reg() / i2c_read_reg()         通用 I2C 多字节读写
        ap3216c_set_mode() / ap3216c_get_mode()  系统模式读写
        ap3216c_read_als() / ap3216c_read_ps() / ap3216c_read_ir()
                                                 ALS / PS / IR 数据读取
        ap3216c_init()                           软复位并进入连续测量模式
        ap3216c_read_raw()                       IIO 标准 RAW 读取回调
        mode_iio_show() / mode_iio_store()       sysfs 私有模式属性读写
        ap3216c_probe() / ap3216c_remove()       I2C 驱动 probe / remove
        devm_iio_device_alloc()                  IIO 设备分配
        devm_iio_device_register()               IIO 设备注册
        module_i2c_driver()                      I2C 驱动模块入口

3. 编写目的

   （1）实现 AP3216C 传感器驱动，并重新设计代码架构，重点体现分层解耦思维。
        将“通用 I2C 读写”“AP3216C 寄存器语义”“IIO / sysfs 用户接口”
        分成不同层次，降低耦合，方便移植和扩展。

   （2）锻炼 IIO 子系统标准接口与非标准接口的设计能力。
        标准接口用于 cat 读取 RAW 数据，非标准 sysfs 属性用于 echo 设置模式，
        同时练习 I2C 从设备驱动代码的编写流程。

   （3）在 Linux 4.1.15 这类较老内核中锻炼阅读源码、确认 API 签名与语义的能力。
        不照抄教程，而是在目标内核源码树中确认 i2c、iio、sysfs、of 等接口。

4. 目前缺陷

   （1）缺少并发保护。
        IIO read_raw、mode_iio_show、mode_iio_store 以及 remove 路径之间
        可能并发访问同一个 i2c_client。尤其是 ap3216c_set_mode() 是
        “读-改-写”过程，多进程同时 echo mode_iio 可能产生竞态。
        当前未加 mutex / spinlock / atomic 保护。


5. 设备树节点写法

   示例：

    &i2c1 {
        clock-frequency = <100000>;
        pinctrl-names = "default";
        pinctrl-0 = <&pinctrl_i2c1>;
        status = "okay";

        ap3216c_i2c: ap3216c@1e {
                compatible = "mjk,ap3216c";
                reg = <0x1e>;
                interrupt-parent = <&gpio1>;
                interrupts = <1 0>;
        };
    };

   说明：

       &i2c1 是 I2C 控制器节点；
       ap3216c@1e 是挂在该 I2C 总线上的从设备节点；
       reg = <0x1e> 表示 AP3216C 的 I2C 从机地址为 0x1E；
       compatible = "mjk,ap3216c" 与驱动 of_device_id 匹配；
       interrupt-parent / interrupts 当前驱动未使用，保留或删除均可。

6. 设备树写法查找路径

   在 Linux 4.1.15 中，AP3216C 没有找到专属设备树绑定文档，因此
   compatible 采用自定义写法 "mjk,ap3216c"。

   可参考路径：

       Documentation/devicetree/bindings/i2c/
           i2c-imx.txt              I2C 控制器节点写法
           trivial-devices.txt      简单 I2C 设备节点写法
           i2c.txt                  通用 I2C 绑定说明，若存在
           ...

       Documentation/devicetree/bindings/interrupt-controller/interrupts.txt
           中断属性写法

   一般 I2C 从设备节点至少需要：
       compatible、reg；
   若有中断，再加上：
       interrupt-parent、interrupts；
   若控制器需要引脚复用，则在 I2C 控制器节点中配置 pinctrl。

7. API 确定路径

   include/linux/i2c.h              I2C 子系统：i2c_client、i2c_driver、
                                    i2c_msg、i2c_transfer、i2c_device_id
   include/linux/iio/iio.h          IIO 子系统：iio_dev、iio_chan_spec、
                                    iio_info、devm_iio_device_alloc、
                                    iio_priv、devm_iio_device_register
   include/linux/iio/types.h        IIO 类型：IIO_LIGHT、IIO_PROXIMITY、
                                    IIO_INTENSITY、IIO_MOD_LIGHT_IR
   include/linux/device.h           DEVICE_ATTR_RW、attribute_group
   include/linux/of.h               of_device_id、设备树匹配
   include/linux/module.h           模块加载卸载相关宏

   另外，i2c-common.h 是本驱动自封装的通用 I2C 读写层。

8. 核心结构体

   struct i2c_client
       I2C 从设备实例，关键字段：
           addr        从机地址
           name        设备名
           adapter     I2C 适配器
           dev         设备模型对象
           of_node     设备树节点

   struct i2c_driver
       I2C 驱动，关键字段：
           probe / remove
           id_table
           driver.of_match_table
           driver.name

   struct i2c_msg
       I2C 传输消息，关键字段：
           addr        从机地址
           flags       读写方向
           buf         数据缓冲区
           len         数据长度

   struct iio_dev
       IIO 设备抽象，关键字段：
           name            设备名
           channels        通道数组
           num_channels    通道数量
           info            IIO 回调
           modes           INDIO_DIRECT_MODE 等
           priv            私有数据

   struct iio_chan_spec
       IIO 通道描述，关键字段：
           type                  IIO_LIGHT / IIO_PROXIMITY / IIO_INTENSITY
           channel               通道号
           info_mask_separate    单独属性掩码，如 BIT(IIO_CHAN_INFO_RAW)
           modified / channel2   IIO_MOD_LIGHT_IR 等修饰
           scan_type             realbits、storagebits、endianness

   struct iio_info
       IIO 操作集合，关键字段：
           driver_module
           read_raw
           attrs

   struct ap3216c_dev
       本驱动私有数据：

           struct ap3216c_dev {
               struct i2c_client *client;
           };

       通过 iio_priv(indio_dev) 取得，用于在 IIO 回调中访问 i2c_client。

   DEVICE_ATTR_RW(mode_iio)
       生成 mode_iio 的 sysfs 属性，对应 mode_iio_show / mode_iio_store。

9. 测试方式

   Linux 将 IIO 设备暴露在：

       /sys/bus/iio/devices/iio:deviceX/

   其中 X 是 IIO 设备编号。进入目录后可以看到：

       in_light_raw
       in_proximity_raw
       in_intensity_ir_raw
       mode_iio
       ...

   标准接口读取数据：

       cat /sys/bus/iio/devices/iio:deviceX/in_light_raw
       cat /sys/bus/iio/devices/iio:deviceX/in_proximity_raw
       cat /sys/bus/iio/devices/iio:deviceX/in_intensity_ir_raw

   非标准接口设置模式：

       cat /sys/bus/iio/devices/iio:deviceX/mode_iio
       echo 3 > /sys/bus/iio/devices/iio:deviceX/mode_iio

   模式值参考：

       0 = POWER_DOWN
       1 = ALS_ONLY
       2 = PS_IR
       3 = ALS_PS_IR
       4 = SW_RESET
       5 = ALS_ONCE
       6 = PS_IR_ONCE
       7 = ALS_PS_IR_ONCE

10. 使用步骤

   （1）编译驱动：

        make

        Makefile 中：
            KERNEL_PATH=/home/apprentice/Intelligent_Security_system/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga
            obj-m += ap3216c_drv.o
            ap3216c_drv-objs := ap3216c_main.o ap3216c.o

        编译成功后生成 ap3216c_drv.ko，并可拷贝到：
            /nfs/rootfs/drv

   （2）加载模块：

        insmod ap3216c_drv.ko

        或使用 modprobe，取决于开发板环境。

   （3）查看 probe 日志：

        dmesg | tail

   （4）确认 IIO 设备：

        ls /sys/bus/iio/devices/

        找到与 ap3216c 对应的 iio:deviceX。

   （5）查看属性：

        ls /sys/bus/iio/devices/iio:deviceX/

   （6）读取数据：

        cat /sys/bus/iio/devices/iio:deviceX/in_light_raw
        cat /sys/bus/iio/devices/iio:deviceX/in_proximity_raw
        cat /sys/bus/iio/devices/iio:deviceX/in_intensity_ir_raw

   （7）设置模式：

        cat /sys/bus/iio/devices/iio:deviceX/mode_iio
        echo 3 > /sys/bus/iio/devices/iio:deviceX/mode_iio

       设置后建议等待约 113 ms 再读取数据。

   （8）卸载模块：

        rmmod ap3216c_drv

11. 测试用例

   （1）默认连续测量：
        probe 中 ap3216c_init() 会将传感器设置为 ALS + PS + IR 连续测量。
        直接 cat in_light_raw / in_proximity_raw / in_intensity_ir_raw，
        应能读到数据。

   （2）ALS 环境光测试：
        echo 1 > mode_iio   // ALS only
        等待后读取 in_light_raw。
        用手电照射或遮挡传感器，观察数值变化。

   （3）PS 接近测试：
        echo 2 > mode_iio   // PS + IR
        等待后读取 in_proximity_raw。
        用手靠近或遮挡传感器，观察数值变化。

   （4）IR 测试：
        echo 2 > mode_iio   // PS + IR
        等待后读取 in_intensity_ir_raw。
        改变反射面或遮挡，观察数值变化。

   （5）模式读取：
        cat mode_iio
        应与最近一次 echo 写入的模式一致。

   （6）软复位测试：
        echo 4 > mode_iio   // SW_RESET
        等待约 10 ms 后，再 echo 3 > mode_iio 恢复连续测量。

   （7）单次测量测试：
        echo 5 > mode_iio   // ALS once
        echo 6 > mode_iio   // PS + IR once
        echo 7 > mode_iio   // ALS + PS + IR once
        等待约 113 ms 后读取对应 RAW 数据。

   （8）多进程并发测试：
        当前驱动未实现并发保护。
        可同时运行多个 cat / echo 命令观察是否存在模式读写交叉、
        I2C 访问竞态等问题。该测试用于暴露后续需要补充 mutex 的位置。

==================================================
结束
==================================================