#ifndef __AP3216C_H__
#define __AP3216C_H__

#include <linux/i2c.h>
#include <linux/types.h>

/* ============ I2C 从机地址 ============ */
#define AP3216C_I2C_ADDR            0x1E

/* ============ 寄存器地址 ============ */
#define AP3216C_REG_SYS_CONF        0x00
#define AP3216C_REG_INT_STATUS      0x01
#define AP3216C_REG_INT_CLR         0x02
#define AP3216C_REG_IR_DATA_L       0x0A
#define AP3216C_REG_IR_DATA_H       0x0B
#define AP3216C_REG_ALS_DATA_L      0x0C
#define AP3216C_REG_ALS_DATA_H      0x0D
#define AP3216C_REG_PS_DATA_L       0x0E
#define AP3216C_REG_PS_DATA_H       0x0F
#define AP3216C_REG_ALS_CONF        0x10
#define AP3216C_REG_ALS_CALIB       0x19
#define AP3216C_REG_PS_CONF         0x20
#define AP3216C_REG_PS_LED          0x21

/* ============ System Mode (reg 0x00, bit2-0) ============ */
#define AP3216C_MODE_POWER_DOWN     0x00
#define AP3216C_MODE_ALS_ONLY       0x01
#define AP3216C_MODE_PS_IR          0x02
#define AP3216C_MODE_ALS_PS_IR      0x03
#define AP3216C_MODE_SW_RESET       0x04
#define AP3216C_MODE_ALS_ONCE       0x05
#define AP3216C_MODE_PS_IR_ONCE     0x06
#define AP3216C_MODE_ALS_PS_IR_ONCE 0x07
#define AP3216C_MODE_MASK           0x07

/* ============ 转换时间 (ms) ============ */
#define AP3216C_RESET_DELAY_MS      10
#define AP3216C_ALS_PS_IR_DELAY_MS  113

/* ============ 数据掩码/标志位 ============ */
#define AP3216C_IR_DATA_MASK        0x03FF  /* 10 bit */
#define AP3216C_PS_DATA_MASK        0x03FF  /* 10 bit */
#define AP3216C_IR_OF_BIT           BIT(7)
#define AP3216C_PS_OBJ_BIT          BIT(7)
#define AP3216C_PS_IROF_BIT         BIT(6)

/* ============ 函数声明 ============ */
int ap3216c_set_mode(struct i2c_client *client, u8 mode);
int ap3216c_get_mode(struct i2c_client *client, u8 *mode);
int ap3216c_read_als(struct i2c_client *client, u16 *val);
int ap3216c_read_ps(struct i2c_client *client, u16 *val);
int ap3216c_read_ir(struct i2c_client *client, u16 *val);
int ap3216c_init(struct i2c_client *client);

#endif 



