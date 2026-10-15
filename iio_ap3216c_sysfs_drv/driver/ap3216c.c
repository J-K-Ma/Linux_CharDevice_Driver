#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include "i2c-common.h"
#include "ap3216c.h"

/*
 * ap3216c_set_mode - 设置 AP3216C 系统模式
 * @client: I2C 客户端
 * @mode:   系统模式值 (AP3216C_MODE_xxx)
 *
 * 寄存器 0x00 的 B7-B3 保留，采用读-改-写策略，
 * 只修改 bit2-0，保留其他位。
 */
int ap3216c_set_mode(struct i2c_client *client, u8 mode)
{
    u8 val;
    int ret;

    ret = i2c_read_reg(client, AP3216C_REG_SYS_CONF, 1, &val);
    if (ret)
        return ret;

    val = (val & ~AP3216C_MODE_MASK) | (mode & AP3216C_MODE_MASK);

    return i2c_write_reg(client, AP3216C_REG_SYS_CONF, 1, &val);
}

/*
 * ap3216c_get_mode - 获取 AP3216C 当前系统模式
 * @client: I2C 客户端
 * @mode:   输出参数，返回当前模式值 (bit2-0)
 *
 * 读取寄存器 0x00，取出低 3 位作为当前模式。
 * 高位 B7-B3 保留，读出来后屏蔽掉。
 */
int ap3216c_get_mode(struct i2c_client *client, u8 *mode)
{
    u8 val;
    int ret;

    if (!mode)
        return -EINVAL;

    ret = i2c_read_reg(client, AP3216C_REG_SYS_CONF, 1, &val);
    if (ret)
        return ret;

    *mode = val & AP3216C_MODE_MASK;
    return 0;
}

/*
 * ap3216c_read_als - 读取 ALS 环境光 16bit 数据
 *
 * 0x0C 低字节，0x0D 高字节，小端。
 */
int ap3216c_read_als(struct i2c_client *client, u16 *val)
{
    u8 buf[2];
    int ret;

    ret = i2c_read_reg(client, AP3216C_REG_ALS_DATA_L, 2, buf);
    if (ret)
        return ret;

    *val = ((u16)buf[1] << 8) | buf[0];
    return 0;
}

/*
 * ap3216c_read_ps - 读取 PS 接近 10bit 数据
 *
 * 0x0E: B7=OBJ, B6=IR_OF, B3-B0=数据低
 * 0x0F: B7=OBJ, B6=IR_OF, B5-B0=数据高
 * 组合成 10bit：高 6bit + 低 4bit
 */
int ap3216c_read_ps(struct i2c_client *client, u16 *val)
{
    u8 buf[2];
    int ret;

    ret = i2c_read_reg(client, AP3216C_REG_PS_DATA_L, 2, buf);
    if (ret)
        return ret;

    *val = (((u16)(buf[1] & 0x3F)) << 4) | (buf[0] & 0x0F);
    return 0;
}

/*
 * ap3216c_read_ir - 读取 IR 10bit 数据
 *
 * 0x0A: B7=IR_OF, B1-B0=数据低
 * 0x0B: B7-B0=数据高
 * 组合成 10bit：高 8bit + 低 2bit
 */
int ap3216c_read_ir(struct i2c_client *client, u16 *val)
{
    u8 buf[2];
    int ret;

    ret = i2c_read_reg(client, AP3216C_REG_IR_DATA_L, 2, buf);
    if (ret)
        return ret;

    *val = (((u16)buf[1] << 2) | (buf[0] & 0x03)) & AP3216C_IR_DATA_MASK;
    return 0;
}

/*
 * ap3216c_init - 上电初始化
 *
 * 1. 软复位：写 0x00 = 0x04，等待 >= 10ms
 * 2. 连续测量模式：ALS + PS + IR (0x03)，等待 >= 113ms
 */
int ap3216c_init(struct i2c_client *client)
{
    int ret;

    /* 1. 软复位 */
    ret = ap3216c_set_mode(client, AP3216C_MODE_SW_RESET);
    if (ret)
        return ret;
    msleep(AP3216C_RESET_DELAY_MS);

    /* 2. 设置为 ALS + PS + IR 同时连续测量 */
    ret = ap3216c_set_mode(client, AP3216C_MODE_ALS_PS_IR);
    if (ret)
        return ret;
    msleep(AP3216C_ALS_PS_IR_DELAY_MS);

    return 0;
}
