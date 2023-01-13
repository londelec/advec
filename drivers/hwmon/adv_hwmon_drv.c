/*****************************************************************************
  Copyright (c) 2018, Advantech Automation Corp.
  THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
  INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

  ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
  ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_hwmon_drv.c
 * Version:     1.00  <11/05/2015>
 * Author:      Jiangwei.Zhu
 *
 * Description: The adv_hwmon_drv driver is for controlling EC hwmon.
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <11/05/2015> Jiangwei.Zhu
 *              - Initial version
 *              Version 1.01 <03/04/2016> Jiangwei.Zhu
 *              - Support UNO-1372G-E3AE, TPC-1782H-433AE, APAX-5580-433AE
 *              Version 1.02 <05/09/2016> Ji.Xu
 *              - Support APAX-5580-473AE/4C3AE
 *              - Modify the device name check method to fuzzy matching.
 *              Version 1.03 <05/09/2017> Ji.Xu
 *              - Support UNO-2271G-E2xAE
 *              - Support UNO-2271G-E02xAE
 *              - Support ECU-4784
 *              - Support UNO-2473G-JxAE
 *              Version 1.04 <09/20/2017> Ji.Xu
 *              - Support UNO-2484G-633xAE
 *              - Support UNO-2484G-653xAE
 *              - Support UNO-2484G-673xAE
 *              - Support UNO-2484G-733xAE
 *              - Support UNO-2484G-753xAE
 *              - Support UNO-2484G-773xAE
 *              Version 1.05 <10/26/2017> Ji.Xu
 *              - Support PR/VR4
 *              - Support UNO-3283G-674AE
 *              - Support UNO-3285G-674AE
 *              Version 1.06 <11/16/2017> Zhang.Yang
 *              - Support UNO-1372G-J021AE/J031AE
 *              - Support UNO-2372G
 *              Version 1.07 <02/02/2018> Ji.Xu
 *              - Convert the driver to use new hwmon API after kernel version 4.10.0
 *              - Support EC TPC-B500-6??AE
 *              - Support EC TPC-5???T-6??AE
 *              Version 1.08 <02/20/2019> Ji.Xu
 *              - Support EC UNO-420
 *              - Support EC TPC-B200-???AE
 *              - Support EC TPC-2???T-???AE
 *              - Support EC TPC-2???W-???AE
 *              Version 1.09 <04/24/2020> Yao.Kang
 *              - Support EC UNO-2473G
 *              Version 1.10 <07/21/2020> Yao.Kang
 *              - Support EC UNO-3283G/3285G-674AE
 *              - Support EC UNO-3272
 *              Version 1.11 <05/24/2021> pengcheng.du
 *              - Support EC UNO-137-E1
 *              - Support EC UNO-410-E1
 *              - Support EC WISE-5580
 -----------------------------------------------------------------------------*/

#include <linux/version.h>
#ifndef KERNEL_VERSION
#define  KERNEL_VERSION(a, b, c) KERNEL_VERSION((a)*65536+(b)*256+(c))
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <linux/config.h>
#endif
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/msr.h>
#include <asm/msr-index.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/sysfs.h>
#include "../mfd-ec/ec.h"

#define ADVANTECH_EC_HWMON_VER     "1.11"
#define ADVANTECH_EC_HWMON_DATE    "05/24/2021"

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x2d, 0x2e, I2C_CLIENT_END };

enum chips { f75373, f75375, f75387 };

/* Fintek F75375 registers  */
#define F75375_REG_CONFIG0      0x0
#define F75375_REG_CONFIG1      0x1
#define F75375_REG_CONFIG2      0x2
#define F75375_REG_CONFIG3      0x3
#define F75375_REG_ADDR         0x4
#define F75375_REG_INTR         0x31
#define F75375_CHIP_ID          0x5A
#define F75375_REG_VERSION      0x5C
#define F75375_REG_VENDOR       0x5D

#define F75375_REG_TEMP(nr)     (0x14 + (nr))
#define F75387_REG_TEMP11_LSB(nr)   (0x1c + (nr))
#define F75375_REG_TEMP_HIGH(nr)    (0x28 + (nr) * 2)
#define F75375_REG_TEMP_HYST(nr)    (0x29 + (nr) * 2)

/*
 * Data structures and manipulation thereof
 */

struct f75375_data {
    unsigned short addr;
    struct device *hwmon_dev;

    const char *name;
    int kind;
    struct mutex update_lock; /* protect register access */
    char valid;
    unsigned long last_updated; /* In jiffies */
    unsigned long last_limits;  /* In jiffies */

    /* Register values */
    /*
     * f75387: For remote temperature reading, it uses signed 11-bit
     * values with LSB = 0.125 degree Celsius, left-justified in 16-bit
     * registers. For original 8-bit temp readings, the LSB just is 0.
     */
    s16 temp11[2];
    s8 temp_high[2];
    s8 temp_max_hyst[2];
};

static int f75375_detect(struct i2c_client *client, struct i2c_board_info *info);
static int f75375_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int f75375_remove(struct i2c_client *client);
static int adspname_detect(const char *bios_product_name, const char *standard_name);

static const struct i2c_device_id f75375_id[] = {
    { "f75387", f75387 },
    { }
};
MODULE_DEVICE_TABLE(i2c, f75375_id);

struct EC_HWMON_DATA lmsensor_data;
struct HW_PIN_TBL *ptbl = &lmsensor_data.pin_tbl;
static unsigned long    resolution, resolution_vin, resolution_sys, resolution_curr, resolution_power;
static unsigned long    r1, r1_vin, r1_sys, r1_curr, r1_power;
static unsigned long    r2, r2_vin, r2_sys, r2_curr, r2_power;
static int      offset;
static struct device *ec_hwmon_dev;
static struct attribute_group ec_hwmon_group;
static struct EC_SMBOEM0 ec_smboem0;

static struct i2c_driver f75375_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name = "f75375",
    },
    .probe = f75375_probe,
    .remove = f75375_remove,
    .id_table = f75375_id,
    .detect = f75375_detect,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32)
    .address_list = normal_i2c,
#endif
};

static inline int f75375_read8(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

/* in most cases, should be called while holding update_lock */
static inline u16 f75375_read16(struct i2c_client *client, u8 reg)
{
    return (i2c_smbus_read_byte_data(client, reg) << 8)
        | i2c_smbus_read_byte_data(client, reg + 1);
}

static inline void f75375_write8(struct i2c_client *client, u8 reg,
        u8 value)
{
    i2c_smbus_write_byte_data(client, reg, value);
}

static inline void f75375_write16(struct i2c_client *client, u8 reg,
        u16 value)
{
    int err = i2c_smbus_write_byte_data(client, reg, (value >> 8));
    if (err)
        return;
    i2c_smbus_write_byte_data(client, reg + 1, (value & 0xFF));
}


static struct f75375_data *f75375_update_device(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct f75375_data *data = i2c_get_clientdata(client);
    int nr;

    mutex_lock(&data->update_lock);

    /* Limit registers cache is refreshed after 60 seconds */
    if (time_after(jiffies, data->last_limits + 60 * HZ)
            || !data->valid) {
        for (nr = 0; nr < 2; nr++) {
            data->temp_high[nr] =
                f75375_read8(client, F75375_REG_TEMP_HIGH(nr));
            data->temp_max_hyst[nr] =
                f75375_read8(client, F75375_REG_TEMP_HYST(nr));
        }
        data->last_limits = jiffies;
    }

    /* Measurement registers cache is refreshed after 2 second */
    if (time_after(jiffies, data->last_updated + 2 * HZ)
            || !data->valid) {
        for (nr = 0; nr < 2; nr++) {
            /* assign MSB, therefore shift it by 8 bits */
            data->temp11[nr] = f75375_read8(client, F75375_REG_TEMP(nr)) << 8;
            if (data->kind == f75387)
                /* merge F75387's temperature LSB (11-bit) */
                data->temp11[nr] |= f75375_read8(client, F75387_REG_TEMP11_LSB(nr));
        }
        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);
    return data;
}

#define TEMP_FROM_REG(val) ((val) * 1000)
#define TEMP_TO_REG(val) ((val) / 1000)
#define TEMP11_FROM_REG(reg)    ((reg) / 32 * 125)

static ssize_t show_temp11(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int nr = to_sensor_dev_attr(attr)->index;
    struct f75375_data *data = f75375_update_device(dev);
    return sprintf(buf, "%d\n", TEMP11_FROM_REG(data->temp11[nr]));
}

static ssize_t show_temp_max(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int nr = to_sensor_dev_attr(attr)->index;
    struct f75375_data *data = f75375_update_device(dev);
    return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp_high[nr]));
}

static ssize_t show_temp_max_hyst(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int nr = to_sensor_dev_attr(attr)->index;
    struct f75375_data *data = f75375_update_device(dev);
    return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp_max_hyst[nr]));
}

static ssize_t set_temp_max(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    int nr = to_sensor_dev_attr(attr)->index;
    struct i2c_client *client = to_i2c_client(dev);
    struct f75375_data *data = i2c_get_clientdata(client);
    unsigned long val;
    int err;

    err = kstrtoul(buf, 10, &val);
    if (err < 0)
        return err;

    val = clamp_val(TEMP_TO_REG(val), 0, 127);
    mutex_lock(&data->update_lock);
    data->temp_high[nr] = val;
    f75375_write8(client, F75375_REG_TEMP_HIGH(nr), data->temp_high[nr]);
    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t set_temp_max_hyst(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    int nr = to_sensor_dev_attr(attr)->index;
    struct i2c_client *client = to_i2c_client(dev);
    struct f75375_data *data = i2c_get_clientdata(client);
    unsigned long val;
    int err;

    err = kstrtoul(buf, 10, &val);
    if (err < 0)
        return err;

    val = clamp_val(TEMP_TO_REG(val), 0, 127);
    mutex_lock(&data->update_lock);
    data->temp_max_hyst[nr] = val;
    f75375_write8(client, F75375_REG_TEMP_HYST(nr),
            data->temp_max_hyst[nr]);
    mutex_unlock(&data->update_lock);
    return count;
}


static ssize_t get_ec_hwmon_name(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "advhwmon\n");
}

static ssize_t get_ec_in1_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "VBAT\n");
}

static ssize_t get_ec_in2_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "5VSB\n");
}

static ssize_t get_ec_in3_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "VIN\n");
}

static ssize_t get_ec_in4_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "VCORE\n");
}

static ssize_t get_ec_in5_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "VIN1\n");
}

static ssize_t get_ec_in6_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "VIN2\n");
}

static ssize_t get_ec_in7_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "System Voltage\n");
}

static ssize_t get_ec_in8_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return  sprintf(buf, "Vin\n");
}

static ssize_t get_ec_in9_label(struct device *dev,struct device_attribute *attr,char *buf)
{
        return  sprintf(buf, "+V3.3\n");
}

static ssize_t get_ec_in10_label(struct device *dev,struct device_attribute *attr,char *buf)
{
        return  sprintf(buf, "DCIN1\n");
}

static ssize_t get_ec_in11_label(struct device *dev,struct device_attribute *attr,char *buf)
{
        return  sprintf(buf, "DCIN2\n");
}

static ssize_t get_ec_curr1_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Current\n");
}

static ssize_t get_ec_curr2_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Current\n");
}

static ssize_t get_ec_curr3_label(struct device *dev,struct device_attribute *attr,char *buf)
{
        return sprintf(buf, "IMON\n");
}

static ssize_t get_ec_power1_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Power\n");
}

static ssize_t get_ec_temp1_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Temp Board\n");
}

static ssize_t get_ec_temp2_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Temp CPU\n");
}

static ssize_t get_ec_temp2_crit(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "100000\n");
}

static ssize_t get_ec_temp2_crit_alarm(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "0\n");
}

static ssize_t get_ec_temp3_label(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Temp System\n");
}

static ssize_t get_ec_temp3_crit(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "100000\n");
}

static ssize_t get_ec_temp3_crit_alarm(struct device *dev,struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "0\n");
}

static ssize_t get_ec_in1_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in2_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in3_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in4_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in5_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in6_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in7_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in8_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in9_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in10_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_in11_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_curr1_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_curr2_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_curr3_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_power1_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_temp2_input(struct device *dev,struct device_attribute *attr,char *buf);
static ssize_t get_ec_temp3_input(struct device *dev,struct device_attribute *attr,char *buf);

static SENSOR_DEVICE_ATTR(name,S_IRUGO,get_ec_hwmon_name,NULL,0);
static SENSOR_DEVICE_ATTR(in1_input,S_IRUGO,get_ec_in1_input,NULL,0);
static SENSOR_DEVICE_ATTR(in1_label,S_IRUGO,get_ec_in1_label,NULL,0);
static SENSOR_DEVICE_ATTR(in2_input,S_IRUGO,get_ec_in2_input,NULL,0);
static SENSOR_DEVICE_ATTR(in2_label,S_IRUGO,get_ec_in2_label,NULL,0);
static SENSOR_DEVICE_ATTR(in3_input,S_IRUGO,get_ec_in3_input,NULL,0);
static SENSOR_DEVICE_ATTR(in3_label,S_IRUGO,get_ec_in3_label,NULL,0);
static SENSOR_DEVICE_ATTR(in4_input,S_IRUGO,get_ec_in4_input,NULL,0);
static SENSOR_DEVICE_ATTR(in4_label,S_IRUGO,get_ec_in4_label,NULL,0);
static SENSOR_DEVICE_ATTR(in5_input,S_IRUGO,get_ec_in5_input,NULL,0);
static SENSOR_DEVICE_ATTR(in5_label,S_IRUGO,get_ec_in5_label,NULL,0);
static SENSOR_DEVICE_ATTR(in6_input,S_IRUGO,get_ec_in6_input,NULL,0);
static SENSOR_DEVICE_ATTR(in6_label,S_IRUGO,get_ec_in6_label,NULL,0);
static SENSOR_DEVICE_ATTR(in7_input,S_IRUGO,get_ec_in7_input,NULL,0);
static SENSOR_DEVICE_ATTR(in7_label,S_IRUGO,get_ec_in7_label,NULL,0);
static SENSOR_DEVICE_ATTR(in8_input,S_IRUGO,get_ec_in8_input,NULL,0);
static SENSOR_DEVICE_ATTR(in8_label,S_IRUGO,get_ec_in8_label,NULL,0);
static SENSOR_DEVICE_ATTR(in9_input,S_IRUGO,get_ec_in9_input,NULL,0);
static SENSOR_DEVICE_ATTR(in9_label,S_IRUGO,get_ec_in9_label,NULL,0);
static SENSOR_DEVICE_ATTR(in10_input,S_IRUGO,get_ec_in10_input,NULL,0);
static SENSOR_DEVICE_ATTR(in10_label,S_IRUGO,get_ec_in10_label,NULL,0);
static SENSOR_DEVICE_ATTR(in11_input,S_IRUGO,get_ec_in11_input,NULL,0);
static SENSOR_DEVICE_ATTR(in11_label,S_IRUGO,get_ec_in11_label,NULL,0);
static SENSOR_DEVICE_ATTR(curr1_label,S_IRUGO,get_ec_curr1_label,NULL,0);
static SENSOR_DEVICE_ATTR(curr1_input,S_IRUGO,get_ec_curr1_input,NULL,0);
static SENSOR_DEVICE_ATTR(curr2_label,S_IRUGO,get_ec_curr2_label,NULL,0);
static SENSOR_DEVICE_ATTR(curr2_input,S_IRUGO,get_ec_curr2_input,NULL,0);
static SENSOR_DEVICE_ATTR(curr3_label,S_IRUGO,get_ec_curr3_label,NULL,0);
static SENSOR_DEVICE_ATTR(curr3_input,S_IRUGO,get_ec_curr3_input,NULL,0);
static SENSOR_DEVICE_ATTR(power1_label,S_IRUGO,get_ec_power1_label,NULL,0);
static SENSOR_DEVICE_ATTR(power1_input,S_IRUGO,get_ec_power1_input,NULL,0);
static SENSOR_DEVICE_ATTR(temp1_label,S_IRUGO,get_ec_temp1_label,NULL,0);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp11, NULL, 0);
static SENSOR_DEVICE_ATTR(temp1_max_hyst, S_IRUGO|S_IWUSR, show_temp_max_hyst, set_temp_max_hyst, 0);
static SENSOR_DEVICE_ATTR(temp1_max, S_IRUGO|S_IWUSR, show_temp_max, set_temp_max, 0);
static SENSOR_DEVICE_ATTR(temp2_label,S_IRUGO,get_ec_temp2_label,NULL,0);
static SENSOR_DEVICE_ATTR(temp2_input,S_IRUGO,get_ec_temp2_input,NULL,0);
static SENSOR_DEVICE_ATTR(temp2_crit,S_IRUSR,get_ec_temp2_crit,NULL,0);
static SENSOR_DEVICE_ATTR(temp2_crit_alarm,S_IRUSR,get_ec_temp2_crit_alarm,NULL,0);
static SENSOR_DEVICE_ATTR(temp3_label,S_IRUGO,get_ec_temp3_label,NULL,0);
static SENSOR_DEVICE_ATTR(temp3_input,S_IRUGO,get_ec_temp3_input,NULL,0);
static SENSOR_DEVICE_ATTR(temp3_crit,S_IRUSR,get_ec_temp3_crit,NULL,0);
static SENSOR_DEVICE_ATTR(temp3_crit_alarm,S_IRUSR,get_ec_temp3_crit_alarm,NULL,0);

// Support list:
// TPC-8100TR, TPC-651T-E3AE, TPC-1251T-E3AE, TPC-1551T-E3AE,
// TPC-1751T-E3AE, TPC-1051WP-E3AE, TPC-1551WP-E3AE, TPC-1581WP-433AE
// TPC-1782H-433AE,
// UNO-1483G-434AE, UNO-2483G-434AE, UNO-3483G-374AE, UNO-2473G
// UNO-2484G-6???AE, UNO-2484G-7???AE, UNO-3283G-674AE, UNO-3285G-674AE
static const struct attribute *ec_hwmon_attrs_TEMPLATE[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in3_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_curr1_label.dev_attr.attr,
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// TPC-B500-6??AE
// TPC-5???T-6??AE
// TPC-5???W-6??AE
// TPC-B200-???AE
// TPC-2???T-???AE
// TPC-2???W-???AE
static const struct attribute *ec_hwmon_attrs_TPC5XXX[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in3_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// PR/VR4
// UNO-137-E1
// UNO-410-E1
static const struct attribute *ec_hwmon_attrs_PRVR4[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in3_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    &sensor_dev_attr_temp3_label.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp3_crit.dev_attr.attr,
    &sensor_dev_attr_temp3_crit_alarm.dev_attr.attr,
    NULL
};

// WISE-5580
static const struct attribute *ec_hwmon_attrs_WISE5580[] = {
        &sensor_dev_attr_name.dev_attr.attr,
        &sensor_dev_attr_in1_input.dev_attr.attr,
        &sensor_dev_attr_in1_label.dev_attr.attr,
        &sensor_dev_attr_in2_input.dev_attr.attr,
        &sensor_dev_attr_in2_label.dev_attr.attr,
        &sensor_dev_attr_in3_input.dev_attr.attr,
        &sensor_dev_attr_in3_label.dev_attr.attr,
        &sensor_dev_attr_in9_input.dev_attr.attr,
        &sensor_dev_attr_in9_label.dev_attr.attr,
        &sensor_dev_attr_in10_input.dev_attr.attr,
        &sensor_dev_attr_in10_label.dev_attr.attr,
        &sensor_dev_attr_in11_input.dev_attr.attr,
        &sensor_dev_attr_in11_label.dev_attr.attr,
        &sensor_dev_attr_curr3_label.dev_attr.attr,
        &sensor_dev_attr_curr3_input.dev_attr.attr,
        &sensor_dev_attr_temp2_label.dev_attr.attr,
        &sensor_dev_attr_temp2_input.dev_attr.attr,
        &sensor_dev_attr_temp3_label.dev_attr.attr,
        &sensor_dev_attr_temp3_input.dev_attr.attr,
        NULL
};

// Support list:
// UNO-2271G-E22AE/E23AE/E022AE/E023AE, UNO-420
static const struct attribute *ec_hwmon_attrs_UNO2271G[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in3_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// UNO-1172A
// ECU-4784
static const struct attribute *ec_hwmon_attrs_UNO1172A[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in3_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// UNO-1372G
static const struct attribute *ec_hwmon_attrs_UNO1372G[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_in8_input.dev_attr.attr,
    &sensor_dev_attr_in8_label.dev_attr.attr,
    &sensor_dev_attr_curr1_label.dev_attr.attr,
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// UNO-2372G
// UNO-1372G-J021AE/J031AE
static const struct attribute *ec_hwmon_attrs_UNO2372G[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in4_label.dev_attr.attr,
    &sensor_dev_attr_in8_input.dev_attr.attr,
    &sensor_dev_attr_in8_label.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    NULL
};

// Support list:
// APAX-5580-433AE
// APAX-5580-473AE
// APAX-5580-4C3AE
static const struct attribute *ec_hwmon_attrs_APAX5580[] = {
    &sensor_dev_attr_name.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in1_label.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in2_label.dev_attr.attr,
    &sensor_dev_attr_in5_input.dev_attr.attr,
    &sensor_dev_attr_in5_label.dev_attr.attr,
    &sensor_dev_attr_in6_input.dev_attr.attr,
    &sensor_dev_attr_in6_label.dev_attr.attr,
    &sensor_dev_attr_in7_input.dev_attr.attr,
    &sensor_dev_attr_in7_label.dev_attr.attr,
    &sensor_dev_attr_curr2_label.dev_attr.attr,
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_power1_label.dev_attr.attr,
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_label.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_crit.dev_attr.attr,
    &sensor_dev_attr_temp2_crit_alarm.dev_attr.attr,
    &sensor_dev_attr_temp3_label.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp3_crit.dev_attr.attr,
    &sensor_dev_attr_temp3_crit_alarm.dev_attr.attr,
    NULL
};

static struct attribute *f75375_attributes[] = {
    &sensor_dev_attr_temp1_label.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_max.dev_attr.attr,
    &sensor_dev_attr_temp1_max_hyst.dev_attr.attr,
    NULL
};

static const struct attribute_group f75375_group = {
    .attrs = f75375_attributes,
};

void ec_hwmon_profile(void)
{
    if(
            (!adspname_detect(BIOS_Product_Name, "TPC-8100TR"))
            || (!adspname_detect(BIOS_Product_Name,"TPC-*51T-E??E"))
            || (!adspname_detect(BIOS_Product_Name,"TPC-*51WP-E?AE"))
            || (!adspname_detect(BIOS_Product_Name,"TPC-*81WP-4???E"))
            || (!adspname_detect(BIOS_Product_Name,"TPC-1?82H-4???E"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-1483G-4??AE"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-2473G"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-2483G-4??AE"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-2484G-6???AE"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-2484G-7???AE"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-3283G/3285G-674AE"))
            || (!adspname_detect(BIOS_Product_Name,"FST-2482P"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-3272"))
            || (!adspname_detect(BIOS_Product_Name,"UNO-3483G-3??AE"))
            )
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_TEMPLATE;
    }
    else if ((!adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-B200-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-2???T-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-2???W-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-300-?8??A"))) {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_TPC5XXX;
    }
    else if ((!adspname_detect(BIOS_Product_Name,"PR/VR4"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-137-E1"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-410-E1")))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_PRVR4;
    }
    else if (!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        resolution = 2929;
        r1 = 1000;
        r2 = 59;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_WISE5580;
    }
    else if(!adspname_detect(BIOS_Product_Name,"ECU-4784"))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO1172A;
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2271G-E??AE"))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO2271G;
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2271G-E???AE"))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO2271G;
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-420"))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO2271G;
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1172A"))
    {
        resolution = 2929;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO1172A;
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1372G-E?AE"))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO1372G;
    }
    else if((!adspname_detect(BIOS_Product_Name,"UNO-2372G"))
           || (!adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE")))
    {
        resolution = 2929;
        r1 = 1912;
        r2 = 1000;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_UNO2372G;
    }
    else if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        resolution = 2929;
        resolution_vin = 8000;
        resolution_sys = 1250;
        resolution_curr = 1000;
        resolution_power = 25000;
        r1 = 0;
        r1_vin = 1120;
        r1_sys = 0;
        r1_curr = 0;
        r1_power = 0;
        r2 = 0;
        r2_vin = 56;
        r2_sys = 0;
        r2_curr = 0;
        r2_power = 0;
        offset = 0;
        ec_hwmon_group.attrs = (struct attribute **)ec_hwmon_attrs_APAX5580;
    }
}

void check_hwpin_number(void)
{
    int i;

    for(i=0;i<EC_MAX_TBL_NUM;i++)
    {
//      printk("\nPDynamic_Tab[%d].DeviceID: 0x%X \n", i, PDynamic_Tab[i].DeviceID);
//      printk("PDynamic_Tab[%d].HWPinNumber: 0x%X \n", i, PDynamic_Tab[i].HWPinNumber);

        switch(PDynamic_Tab[i].DeviceID)
        {
        case EC_DID_CMOSBAT:
            ptbl->vbat[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vbat[1] = 1;
            break;
        case EC_DID_CMOSBAT_X2:
            ptbl->vbat[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vbat[1] = 2;
            break;
        case EC_DID_CMOSBAT_X10:
            ptbl->vbat[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vbat[1] = 10;
            break;
        case EC_DID_5VS0:
        case EC_DID_5VS5:
            ptbl->v5[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v5[1] = 1;
            break;
        case EC_DID_5VS0_X2:
        case EC_DID_5VS5_X2:
            ptbl->v5[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v5[1] = 2;
            break;
        case EC_DID_5VS0_X10:
        case EC_DID_5VS5_X10:
            ptbl->v5[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v5[1] = 10;
            break;
        case EC_DID_12VS0:
            ptbl->v12[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v12[1] = 1;
            break;
        case EC_DID_12VS0_X2:
            ptbl->v12[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v12[1] = 2;
            break;
        case EC_DID_12VS0_X10:
            ptbl->v12[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->v12[1] = 10;
            break;
        case EC_DID_VCOREA:
        case EC_DID_VCOREB:
            ptbl->vcore[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vcore[1] = 1;
            break;
        case EC_DID_VCOREA_X2:
        case EC_DID_VCOREB_X2:
            ptbl->vcore[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vcore[1] = 2;
            break;
        case EC_DID_VCOREA_X10:
        case EC_DID_VCOREB_X10:
            ptbl->vcore[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vcore[1] = 10;
            break;
        case EC_DID_DC:
            ptbl->vdc[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vdc[1] = 1;
            break;
        case EC_DID_DC_X2:
            ptbl->vdc[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vdc[1] = 2;
            break;
        case EC_DID_DC_X10:
            ptbl->vdc[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->vdc[1] = 10;
            break;
        case EC_DID_CURRENT:
            ptbl->ec_current[0] = PDynamic_Tab[i].HWPinNumber;
            ptbl->ec_current[1] = 1;
            break;
        case EC_DID_SMBOEM0:
            ec_smboem0.HWPinNumber = PDynamic_Tab[i].HWPinNumber;
            break;
        default:
            break;
        }
    }
}

static ssize_t get_ec_in1_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    uchar voltage = 0;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->vbat[0], ptbl->vbat[1]);

//  if(!adspname_detect(BIOS_Product_Name,"ECU-4784")) {
//      if(r2 != 0){
//          voltage = temp * r1 / r2;
//      }
//      lmsensor_data.voltage[0] = voltage;
//      return sprintf(buf,"%d\n",lmsensor_data.voltage[0]);
//  }

    if(r2 != 0){
        voltage = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        voltage =  temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        voltage += (int)offset * 100;
    }

    lmsensor_data.voltage[0] = 10*voltage;
    return sprintf(buf,"%d\n",lmsensor_data.voltage[0]);
}

static ssize_t get_ec_in2_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    uchar voltage = 0;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->v5[0], ptbl->v5[1]);

//  if(!adspname_detect(BIOS_Product_Name,"ECU-4784")) {
//      if(r2 != 0){
//          voltage = temp * r1 / r2;
//      }
//      lmsensor_data.voltage[0] = voltage;
//      return sprintf(buf,"%d\n",lmsensor_data.voltage[1]);
//  }

    if(r2 != 0){
        voltage = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        voltage =  temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        voltage += (int)offset * 100;
    }

    lmsensor_data.voltage[1] = 10*voltage;
    return sprintf(buf,"%d\n",lmsensor_data.voltage[1]);
}

static ssize_t get_ec_in3_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    uchar voltage = 0;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->v12[0], ptbl->v12[1]);
    if(temp == -1)
    {
        temp  =  read_ad_value(ptbl->vdc[0], ptbl->vdc[1]);
    }

//  if(!adspname_detect(BIOS_Product_Name,"ECU-4784")) {
//      if(r2 != 0){
//          voltage = temp * r1 / r2;
//      }
//      lmsensor_data.voltage[0] = voltage;
//      return sprintf(buf,"%d\n",lmsensor_data.voltage[2]);
//  }

    if(r2 != 0){
        voltage = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        voltage =  temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        voltage += (int)offset * 100;
    }

    lmsensor_data.voltage[2] = 10*voltage;
    return sprintf(buf,"%d\n",lmsensor_data.voltage[2]);
}

static ssize_t get_ec_in4_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    uchar voltage = 0;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->vcore[0], ptbl->vcore[1]);

    if(r2 != 0){
        voltage = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        voltage = temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        voltage += (int)offset * 100;
    }

    lmsensor_data.voltage[3] = 10*voltage;
    return sprintf(buf,"%d\n",lmsensor_data.voltage[3]);
}

static ssize_t get_ec_curr1_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->ec_current[0], ptbl->ec_current[1]);

    if(r2 != 0){
        temp = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        temp =  temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        temp += (int)offset * 100;
    }

    lmsensor_data.ec_current[3] = temp;
    return sprintf(buf,"%d\n",10*lmsensor_data.ec_current[3]);
}

static void get_temperaturedts(uchar *pvalue)
{
    u32 eax,edx;
    uchar temp;

    rdmsr_on_cpu(1,MSR_IA32_THERM_STATUS,&eax,&edx);
    temp = 100000-((eax>>16)&0x7f) * 1000;
    *pvalue = temp;
}

static void ec_get_temperature_value_via_lm96163_ec_smbus(uchar *temperature)
{
    int ret = -1;
    struct EC_SMBUS_READ_BYTE in_data = {
Channel:    NSLM96163_CHANNEL,
            Address:    NSLM96163_ADDR,
            Register:   NSLM96163_LOC_TEMP,
            Data:   0
    };

    if((ret = smbus_read_byte(&in_data))){
        printk("smbus_read_byte error.\n");
    }else{
        *temperature = (unsigned long)in_data.Data * 10;
    }
}

static ssize_t ec_get_sys_temperature_value_via_f75387_ec_smbus(uchar *temperature)
{
    int ret = -1;
    uchar Temp_MSB = 0;
    uchar Temp_LSB = 0;

    struct EC_SMBUS_READ_BYTE in_data = {
Channel:    ec_smboem0.HWPinNumber & 0x03,
            Address:    LMF75387_SMBUS_SLAVE_ADDRESS_5A,
            Register:   F75387_REG_R_TEMP0_MSB,
            Data:   0
    };

    if((ret = smbus_read_byte(&in_data))){
        printk("smbus_read_byte error.\n");
    }
    Temp_MSB = in_data.Data;
    if(Temp_MSB != 0xFF)
    {
        in_data.Register = F75387_REG_R_TEMP0_LSB;
        if((ret = smbus_read_byte(&in_data))){
            printk("smbus_read_byte error.\n");
        }
        Temp_LSB = in_data.Data;
    }
    else
    {
        Temp_MSB = 0;
        Temp_LSB = 0;
    }
    *temperature = Temp_MSB + Temp_LSB/256;
    return 0;
}

static int ec_get_voltage_v1_value_via_f75387_ec_smbus(uchar *voltage)
{
    int ret = -1;

    struct EC_SMBUS_READ_BYTE in_data = {
Channel:    ec_smboem0.HWPinNumber & 0x03,
            Address:    LMF75387_SMBUS_SLAVE_ADDRESS_5A,
            Register:   F75387_REG_R_V1,
            Data:   0
    };

    if((ret = smbus_read_byte(&in_data))){
        printk("smbus_read_byte error.\n");
    }
    *voltage = in_data.Data;
    return 0;
}

static int ec_get_voltage_v2_value_via_f75387_ec_smbus(uchar *voltage)
{
    int ret = -1;
    struct EC_SMBUS_READ_BYTE in_data = {
Channel:    ec_smboem0.HWPinNumber & 0x03,
            Address:    LMF75387_SMBUS_SLAVE_ADDRESS_5A,
            Register:   F75387_REG_R_V2,
            Data:   0
    };
    if((ret = smbus_read_byte(&in_data))){
        printk("smbus_read_byte error.\n");
    }
    *voltage = in_data.Data;
    return 0;
}

static int ec_get_voltage_system_value_via_ina226_ec_smbus(uchar *voltage)
{
    int ret = -1;
    struct EC_SMBUS_WORD_DATA in_data = {
Channel:     ec_smboem0.HWPinNumber & 0x03,
             Address:    INA266_SMBUS_SLAVE_ADDRESS_8A,
             Register:   INA266_REG_VOLTAGE,
             Value:      0
    };
    if((ret = smbus_read_word(&in_data))){
        printk("smbus_read_word error.\n");
    }
    *voltage = in_data.Value;
    return 0;
}

static int ec_get_current_value_via_ina226_ec_smbus(uchar *curr)
{
    int ret = -1;
    struct EC_SMBUS_WORD_DATA in_data = {
Channel:     ec_smboem0.HWPinNumber & 0x03,
             Address:    INA266_SMBUS_SLAVE_ADDRESS_8A,
             Register:   INA266_REG_CURRENT,
             Value:      0
    };
    if((ret = smbus_read_word(&in_data))){
        printk("smbus_read_word error.\n");
    }
    *curr = in_data.Value;
    return 0;
}

static int ec_get_power_value_via_ina226_ec_smbus(uchar *power)
{
    int ret = -1;
    struct EC_SMBUS_WORD_DATA in_data = {
Channel:     ec_smboem0.HWPinNumber & 0x03,
             Address:    INA266_SMBUS_SLAVE_ADDRESS_8A,
             Register:   INA266_REG_POWER,
             Value:      0
    };
    if((ret = smbus_read_word(&in_data))){
        printk("smbus_read_word error.\n");
    }
    *power = in_data.Value;
    return 0;
}

static ssize_t get_ec_temp2_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp = 0;
    uchar value;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"TPC-8100TR"))
    {
        get_temperaturedts(&temp);
        return sprintf(buf,"%d\n",temp);
    }
    else if(!adspname_detect(BIOS_Product_Name,"TPC-*51T-E??E"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if((!adspname_detect(BIOS_Product_Name,"TPC-*51WP-E?AE"))
            || (!adspname_detect(BIOS_Product_Name,"TPC-*81WP-4???E")))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"TPC-1?82H-4???E"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if ((!adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-B200-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-2???T-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-2???W-???AE"))
                || (!adspname_detect(BIOS_Product_Name,"TPC-300-?8??A")))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1172A"))
    {
        ec_get_temperature_value_via_lm96163_ec_smbus(&temp);
        return sprintf(buf,"%d\n",100*temp);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1372G-E?AE"))
    {
        read_acpi_value(0x61, &value);
        return sprintf(buf, "%d\n", 1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE"))
    {
        read_acpi_value(0x61, &value);
        return sprintf(buf, "%d\n", 1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-1483G-4??AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2372G"))
    {
        read_acpi_value(0x61, &value);
        return sprintf(buf, "%d\n", 1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2473G"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2271G-E??AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2271G-E???AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-420"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2483G-4??AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2484G-6???AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-2484G-7???AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-3283G/3285G-674AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"FST-2482P"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-3272"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"UNO-3483G-3??AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if((!adspname_detect(BIOS_Product_Name,"PR/VR4"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-137-E1"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-410-E1"))
    || (!adspname_detect(BIOS_Product_Name,"WISE-5580")))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"ECU-4784"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    else if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        read_acpi_value(0x61,&value);
        return sprintf(buf,"%d\n",1000*value);
    }
    return 0;
}

static ssize_t get_ec_temp3_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_sys_temperature_value_via_f75387_ec_smbus(&temp);
        return sprintf(buf,"%d\n",1000*temp);
    }
    else if((!adspname_detect(BIOS_Product_Name,"PR/VR4"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-137-E1"))
    || (!adspname_detect(BIOS_Product_Name,"UNO-410-E1")))
    {
        read_acpi_value(0x60,&temp);
        return sprintf(buf,"%d\n",1000*temp);
    }
    else if(!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        read_acpi_value(0x83,&temp);
        return sprintf(buf,"%d\n",1000*temp);
    }
    return 0;
}

static ssize_t get_ec_in5_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar voltage = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_voltage_v1_value_via_f75387_ec_smbus(&voltage);

        if(r2_vin != 0){
            voltage = voltage * (r1_vin + r2_vin) / r2_vin;
        }

        if(resolution_vin != 0){
            voltage = voltage * resolution_vin / 1000 ;
        }

        if(offset != 0)
        {
            voltage += (int)offset * 100;
        }

        lmsensor_data.voltage[4] = voltage;
        return sprintf(buf,"%d\n",lmsensor_data.voltage[4]);

    }
    return 0;
}

static ssize_t get_ec_in6_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar voltage = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_voltage_v2_value_via_f75387_ec_smbus(&voltage);

        if(r2_vin != 0){
            voltage = voltage * (r1_vin + r2_vin) / r2_vin;
        }

        if(resolution_vin != 0){
            voltage = voltage * resolution_vin / 1000 ;
        }

        if(offset != 0)
        {
            voltage += (int)offset * 100;
        }

        lmsensor_data.voltage[5] = voltage;
        return sprintf(buf,"%d\n",lmsensor_data.voltage[5]);

    }
    return 0;
}

static ssize_t get_ec_in7_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar voltage = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_voltage_system_value_via_ina226_ec_smbus(&voltage);

        if(r2_sys != 0){
            voltage = voltage * (r1_sys + r2_sys) / r2_sys;
        }

        if(resolution_sys != 0){
            voltage = voltage * resolution_sys / 1000 ;
        }

        if(offset != 0)
        {
            voltage += (int)offset * 100;
        }

        lmsensor_data.voltage[5] = voltage;
        return sprintf(buf,"%d\n",lmsensor_data.voltage[5]);

    }
    return 0;
}

static ssize_t get_ec_in8_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp;
    uchar voltage = 0;
    ec_hwmon_profile();
    check_hwpin_number();
    temp  =  read_ad_value(ptbl->v12[0], ptbl->v12[1]);
    if(temp == -1)
    {
        temp  =  read_ad_value(ptbl->vdc[0], ptbl->vdc[1]);
    }

    if(r2 != 0){
        voltage = temp * (r1 + r2) / r2;
    }

    if(resolution != 0){
        voltage =  temp * resolution / 1000 / 1000;
    }

    if(offset != 0)
    {
        voltage += (int)offset * 100;
    }

    lmsensor_data.voltage[2] = 10*voltage;
    return sprintf(buf,"%d\n",lmsensor_data.voltage[2]);
}

static ssize_t get_ec_in9_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar v1;
    uchar v2;
    unsigned long value;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        read_acpi_value(0x84,&v1);
        read_acpi_value(0x85,&v2);
        value=(v1 << 2) | (v2 >> 6);
        return sprintf(buf,"%lu\n",4*value);
    }
    return 0;
}

static ssize_t get_ec_in10_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar v1;
    uchar v2;
    unsigned long value;
    unsigned long voltage=0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        read_acpi_value(0x88,&v1);
        read_acpi_value(0x89,&v2);
        value=(v1 << 2) | (v2 >> 6);
        if(r2 != 0){
                voltage = value * (r1 + r2) / r2;
        }
        return sprintf(buf,"%lu\n",2*voltage);
    }
    return 0;
}

static ssize_t get_ec_in11_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar v1;
    uchar v2;
    unsigned long value;
    unsigned long voltage=0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        read_acpi_value(0x8A,&v1);
        read_acpi_value(0x8B,&v2);
        value=(v1 << 2) | (v2 >> 6);
        if(r2 != 0){
                voltage = value * (r1 + r2) / r2;
        }
        return sprintf(buf,"%lu\n",2*voltage);
    }
    return 0;
}

static ssize_t get_ec_curr2_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_current_value_via_ina226_ec_smbus(&temp);
        if(r2_curr != 0){
            temp = temp * (r1_curr + r2_curr) / r2_curr;
        }

        if(resolution_curr != 0){
            temp = temp * resolution_curr / 1000 ;
        }

        if(offset != 0)
        {
            temp += (int)offset * 100;
        }

        lmsensor_data.ec_current[4] = temp;
        return sprintf(buf,"%d\n",lmsensor_data.ec_current[4]);

    }
    return 0;
}

static ssize_t get_ec_curr3_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar v1;
    uchar v2;
    unsigned long value;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"WISE-5580"))
    {
        read_acpi_value(0x86,&v1);
        read_acpi_value(0x87,&v2);
        value=(v1 << 2) | (v2 >> 6);
        return sprintf(buf,"%lu\n",2*value);
    }
    return 0;
}

static ssize_t get_ec_power1_input(struct device *dev,struct device_attribute *attr,char *buf)
{
    uchar temp = 0;
    char *product;
    product = BIOS_Product_Name;
    if(!adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
    {
        ec_get_power_value_via_ina226_ec_smbus(&temp);
        if(r2_power != 0){
            temp = temp * (r1_power + r2_power) / r2_power;
        }

        if(resolution_power != 0){
            temp = temp * resolution_power / 1000 ;
        }

        if(offset != 0)
        {
            temp += (int)offset * 100;
        }

        lmsensor_data.power[1] = 1000*temp;
        return sprintf(buf,"%d\n",lmsensor_data.power[1]);

    }
    return 0;
}

static int f75375_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct f75375_data *data;
    int err;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;
    data = devm_kzalloc(&client->dev, sizeof(struct f75375_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->kind = id->driver_data;

    err = sysfs_create_group(&client->dev.kobj, &f75375_group);
    if (err)
        return err;
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
//  data->hwmon_dev = hwmon_device_register_with_info(&client->dev, NULL, NULL, NULL, NULL);
//#else
    data->hwmon_dev = hwmon_device_register(&client->dev);
//#endif
    if (IS_ERR(data->hwmon_dev)) {
        err = PTR_ERR(data->hwmon_dev);
        goto exit_remove;
    }
    return 0;

exit_remove:
    sysfs_remove_group(&client->dev.kobj, &f75375_group);
    return err;
}

static int f75375_remove(struct i2c_client *client)
{
    struct f75375_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &ec_hwmon_group);
    return 0;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int f75375_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    u16 vendid, chipid;
    u8 version;
    const char *name;
    vendid = f75375_read16(client, F75375_REG_VENDOR);
    chipid = f75375_read16(client, F75375_CHIP_ID);
    printk(KERN_INFO "VendID: 0x%x, ChipID: 0x%x \n", vendid, chipid);
    if (vendid != 0x1934)
        return -ENODEV;

    if (chipid == 0x0306)
        name = "f75375";
    else if (chipid == 0x0204)
        name = "f75373";
    else if (chipid == 0x0410)
        name = "f75387";
    else
        return -ENODEV;

    version = f75375_read8(client, F75375_REG_VERSION);
    dev_info(&adapter->dev, "found %s version: %02X\n", name, version);
    strlcpy(info->type, name, I2C_NAME_SIZE);
    return 0;
}

static void ec_hwmon_exit(void)
{
    printk("Advantech EC hwmon exit!\n");
    sysfs_remove_group(&ec_hwmon_dev->kobj, &ec_hwmon_group);
    hwmon_device_unregister(ec_hwmon_dev);
    if(!adspname_detect(BIOS_Product_Name,"UNO-1172A"))
    {
        i2c_del_driver(&f75375_driver);
    }

}

static int adspname_detect(const char *bios_product_name, const char *standard_name)
{
    int i = 0, j = 0;

    for(j = 0; j < strlen(bios_product_name); j++) {
        if(standard_name[i] == '*') {
            if(i) {
                if(bios_product_name[j] == standard_name[(i + 1)]) {
                    i += 2;
                }
                if(i >= (strlen(standard_name) - 1)) {
                    return 0;
                }
            }
        } else if(standard_name[i] == '?') {
            if(i) {
                i++;
                if(i >= strlen(standard_name)) {
                    return 0;
                }
            }
        } else if(bios_product_name[j] == standard_name[i]) {
            i++;
            if(i >= strlen(standard_name)) {
                return 0;
            }
        }
    }
    return 1;
}

static int ec_hwmon_init(void)
{
    int ret;
    char *product;

    product = BIOS_Product_Name;
    if((adspname_detect(BIOS_Product_Name,"TPC-8100TR"))
            && (adspname_detect(BIOS_Product_Name,"MIO-2263"))
            && (adspname_detect(BIOS_Product_Name,"MIO-5251"))
            && (adspname_detect(BIOS_Product_Name,"TPC-*51T-E??E"))
            && (adspname_detect(BIOS_Product_Name,"TPC-*51WP-E?AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-*81WP-4???E"))
            && (adspname_detect(BIOS_Product_Name,"TPC-1?82H-4???E"))
            && (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-B200-???AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-2???T-???AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-2???W-???AE"))
            && (adspname_detect(BIOS_Product_Name,"TPC-300-?8??A"))
            && (adspname_detect(BIOS_Product_Name,"UNO-1172A"))
            && (adspname_detect(BIOS_Product_Name,"UNO-1372G-E?AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-1483G-4??AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2271G-E??AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2271G-E???AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-420"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2372G"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2473G"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2483G-4??AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2484G-6???AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-2484G-7???AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-3283G/3285G-674AE"))
            && (adspname_detect(BIOS_Product_Name,"FST-2482P"))
            && (adspname_detect(BIOS_Product_Name,"UNO-3483G-3??AE"))
            && (adspname_detect(BIOS_Product_Name,"UNO-3272"))
            && (adspname_detect(BIOS_Product_Name,"ECU-4784"))
            && (adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
            && (adspname_detect(BIOS_Product_Name,"PR/VR4"))
            && (adspname_detect(BIOS_Product_Name,"UNO-137-E1"))
            && (adspname_detect(BIOS_Product_Name,"UNO-410-E1"))
            && (adspname_detect(BIOS_Product_Name,"WISE-5580"))
            ) {
        printk(KERN_INFO "%s is not support EC hwmon! \n", BIOS_Product_Name);
        return -ENODEV;
    }
    ec_hwmon_profile();
    check_hwpin_number();
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
//  ec_hwmon_dev = hwmon_device_register_with_info(NULL, NULL, NULL, NULL, NULL);
//#else
    ec_hwmon_dev = hwmon_device_register(NULL);
//#endif
    if(IS_ERR(ec_hwmon_dev))
    {
        ret = -ENOMEM;
        printk(KERN_ERR "ec_hwmon_dev register failed\n");
        goto fail_hwmon_device_register;
    }

    ret = sysfs_create_group(&ec_hwmon_dev->kobj,&ec_hwmon_group);
    if(ret)
    {
        printk(KERN_ERR "failed to creat ec hwmon\n");
        goto fail_create_group_hwmon;

    }

    if(!adspname_detect(BIOS_Product_Name,"UNO-1172A"))
    {
        ret = i2c_add_driver(&f75375_driver);
        if (ret)
            printk(KERN_ERR "failed to register driver f75375.\n");
    }

    printk("=====================================================\n");
    printk("       Advantech ec hwmon driver V%s [%s]\n",
            ADVANTECH_EC_HWMON_VER, ADVANTECH_EC_HWMON_DATE);
    printk("=====================================================\n");
    return ret;

fail_create_group_hwmon:
    sysfs_remove_group(&ec_hwmon_dev->kobj, &ec_hwmon_group);
fail_hwmon_device_register:
    hwmon_device_unregister(ec_hwmon_dev);
    return ret;
}

module_init(ec_hwmon_init);
module_exit(ec_hwmon_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jiangwei.Zhu");
MODULE_DESCRIPTION("Advantech EC Hwmon Driver.");
MODULE_VERSION(ADVANTECH_EC_HWMON_VER);
