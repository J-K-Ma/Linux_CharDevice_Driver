#ifndef __I2C_COMMON_H__
#define __I2C_COMMON_H__
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>

/*
 * i2c_write_reg - 向 I2C 设备连续写若干个字节
 * @client: I2C 客户端指针
 * @reg:    要写入的寄存器起始地址
 * @len:    要写入的数据长度（字节数）
 * @buf:    要写入的数据缓冲区指针
 *
 * 先发送寄存器地址，再连续写 len 个字节。
 * 注意：这里的 buf 必须连续，所以需要 kzalloc 拼接 [reg][data...]。
 *
 * 返回：成功 0，失败负错误码
 */
static inline int i2c_write_reg(struct i2c_client *client, u8 reg,
				int len, u8 *buf)
{
	int ret;
	u8 *txbuf;
	struct i2c_msg msg;

	if (!client || !buf || len <= 0) {
		pr_err("i2c_write_reg: invalid parameters\n");
		return -EINVAL;
	}

	/* 缓冲区布局：[reg][data0][data1]...[dataN-1] */
	txbuf = kzalloc(len + 1, GFP_KERNEL);
	if (!txbuf) {
		pr_err("i2c_write_reg: kzalloc failed, size=%d\n", len + 1);
		return -ENOMEM;
	}

	txbuf[0] = reg;
	memcpy(&txbuf[1], buf, len);

	msg.addr  = client->addr;
	msg.flags = 0;              /* 写方向 */
	msg.buf   = txbuf;
	msg.len   = len + 1;

	ret = i2c_transfer(client->adapter, &msg, 1);
	kfree(txbuf);

	if (ret != 1) {
		dev_err(&client->dev,
			"i2c_write_reg: transfer failed, reg=0x%02x, ret=%d\n",
			reg, ret);
		return (ret < 0) ? ret : -EIO;
	}

	return 0;
}

/*
 * i2c_read_reg - 从 I2C 设备连续读若干个字节
 * @client: I2C 客户端指针
 * @reg:    要读取的寄存器起始地址
 * @len:    要读取的数据长度（字节数）
 * @buf:    存放读取数据的缓冲区指针
 *
 * 采用“写寄存器地址 + 读数据”的组合消息方式。
 * 两条消息的 buf 各自独立，所以不需要 kzalloc。
 *
 * 返回：成功 0，失败返回负错误码
 */
static inline int i2c_read_reg(struct i2c_client *client, u8 reg,
			       int len, u8 *buf)
{
	int ret;
	struct i2c_msg msgs[2];

	if (!client || !buf || len <= 0) {
		pr_err("i2c_read_reg: invalid parameters\n");
		return -EINVAL;
	}

	/* msg[0]：写寄存器地址 */
	msgs[0].addr  = client->addr;
	msgs[0].flags = 0;              /* 写方向 */
	msgs[0].buf   = &reg;
	msgs[0].len   = 1;

	/* msg[1]：读数据 */
	msgs[1].addr  = client->addr;
	msgs[1].flags = I2C_M_RD;       /* 读方向 */
	msgs[1].buf   = buf;
	msgs[1].len   = len;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		dev_err(&client->dev,
			"i2c_read_reg: transfer failed, reg=0x%02x, ret=%d\n",
			reg, ret);
		return (ret < 0) ? ret : -EIO;
	}

	return 0;
}

#endif 
