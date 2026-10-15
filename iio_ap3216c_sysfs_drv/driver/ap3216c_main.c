#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/iio/iio.h>
#include <linux/iio/types.h>
#include "i2c-common.h"
#include "ap3216c.h"


struct ap3216c_dev{
	struct i2c_client *client;
};

enum ap3216c_chan {
    CHAN_IR = 0,
    CHAN_ALS,
    CHAN_PS,
};

static const struct iio_chan_spec ap3216c_channels[] = {
    {
        .type = IIO_INTENSITY,
        .channel = CHAN_IR,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .modified = 1,
        .channel2 = IIO_MOD_LIGHT_IR,
        .scan_type = {
            .sign = 'u',
            .realbits = 10,
            .storagebits = 16,
            .endianness = IIO_CPU,
        },
    },
    {
        .type = IIO_LIGHT,
        .channel = CHAN_ALS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .scan_type = {
            .sign = 'u',
            .realbits = 16,
            .storagebits = 16,
            .endianness = IIO_CPU,
        },
    },
    {
        .type = IIO_PROXIMITY,
        .channel = CHAN_PS,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .scan_type = {
            .sign = 'u',
            .realbits = 10,
            .storagebits = 16,
            .endianness = IIO_CPU,
        },
    },
};

static int ap3216c_read_raw(struct iio_dev *indio_dev,
                            struct iio_chan_spec const *chan,
                            int *val, int *val2, long mask){
	u16 data;
	int ret = 0;
	struct ap3216c_dev *ap3216c_msg = iio_priv(indio_dev);

	if (mask != IIO_CHAN_INFO_RAW)
        	return -EINVAL;

        switch (chan->type) {
        	case IIO_LIGHT:
            		ret = ap3216c_read_als(ap3216c_msg->client, &data);
            		if (ret)
                		return ret;
            		*val = data;
            			return IIO_VAL_INT;

        	case IIO_PROXIMITY:
            		ret = ap3216c_read_ps(ap3216c_msg->client, &data);
            		if (ret)
                		return ret;
            		*val = data;
            			return IIO_VAL_INT;

        	case IIO_INTENSITY:
            		ret = ap3216c_read_ir(ap3216c_msg->client, &data);
            		if (ret)
                		return ret;
            		*val = data;
            			return IIO_VAL_INT;

        	default:
            		return -EINVAL;
        }
	return ret;
}

/* 读取数据 cat mode  使用ap3216c_get_mode*/
static ssize_t mode_iio_show(struct device *dev, struct device_attribute *attr, char *buf){
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ap3216c_msg = iio_priv(indio_dev);
    u8 mode = 0;
    int ret;

    ret = ap3216c_get_mode(ap3216c_msg->client, &mode);
    if (ret)
        return ret;

    return sprintf(buf, "%d\n", mode);
}

/* 存储数据 echo 使用ap3216c_set_mode*/
static ssize_t mode_iio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len){
    struct iio_dev *indio_dev = dev_to_iio_dev(dev);
    struct ap3216c_dev *ap3216c_msg = iio_priv(indio_dev);
    unsigned int mode;
    int ret;

    ret = kstrtouint(buf, 10, &mode);
    if (ret)
        return ret;

    if (mode > AP3216C_MODE_MASK)
        return -EINVAL;

    ret = ap3216c_set_mode(ap3216c_msg->client, (u8)mode);
    if (ret)
        return ret;

    return len;
}

static DEVICE_ATTR_RW(mode_iio);

static struct attribute *ap3216c_attributes[] = {
    &dev_attr_mode_iio.attr,
    NULL,
};

static const struct attribute_group ap3216c_attr_group = {
    .attrs = ap3216c_attributes,
};

static const struct iio_info ap3216c_info = {
    .driver_module = THIS_MODULE,
    .read_raw = ap3216c_read_raw,
    .attrs = &ap3216c_attr_group,
};

static int ap3216c_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int ret = 0;
    struct iio_dev *indio_dev = NULL;
    struct ap3216c_dev *ap3216c_msg = NULL;

    dev_info(&client->dev, "probe enter, addr=0x%02x, name=%s\n",
             client->addr, client->name);

    /*1.初始化ap3216c传感器*/
    ret = ap3216c_init(client);
    if(ret){
        dev_err(&client->dev, "ap3216c_init failed: %d\n", ret);
        goto iic_init_fail;
    }

    /*2.分配iio_dev*/
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(struct ap3216c_dev));
    if (!indio_dev) {
        dev_err(&client->dev, "fail to alloc iio device\n");
	ret = -ENOMEM;
	goto device_alloc_fail;
    }
    ap3216c_msg = iio_priv(indio_dev);

    //放置私有属性;
    ap3216c_msg -> client = client;

    /*3.配置iio设备属性*/
    indio_dev->name = client->dev.of_node->name;
    indio_dev->channels = ap3216c_channels;
    indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);
    indio_dev->info = &ap3216c_info;
    indio_dev->modes = INDIO_DIRECT_MODE;

    /*4.注册iio子系统*/
    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "failed to register iio device\n");
        goto device_register_fail;
    }

    /*5.初始化ap3216c传感器*/
    ret = ap3216c_init(client);
    if(ret){
	dev_err(&client->dev, "ap3216c_init failed: %d\n", ret);
	goto iic_init_fail;
    }
    return 0;

iic_init_fail:
device_register_fail:
device_alloc_fail:
    return ret;
}

static int ap3216c_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "remove enter\n");
    return 0;
}

static const struct i2c_device_id ap3216c_id[] = {
    { "ap3216c", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ap3216c_id);

static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "mjk,ap3216c" },
    { }
};
MODULE_DEVICE_TABLE(of, ap3216c_of_match);

static struct i2c_driver ap3216c_driver = {
    .driver = {
        .name           = "ap3216c_i2c",
        .of_match_table = ap3216c_of_match,
    },
    .probe    = ap3216c_probe,
    .remove   = ap3216c_remove,
    .id_table = ap3216c_id,
};

module_i2c_driver(ap3216c_driver);

MODULE_AUTHOR("J.K.Ma");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
