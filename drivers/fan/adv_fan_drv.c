/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_fan_drv.c
 * Version:     1.00  <11/02/2020>
 * Author:      Jianfeng.dai
 *
 * Description: The adv_fan_drv is driver for controlling EC Fan.
 *
 *
 * Status:      working
 * Change Log:
 *              Version 1.00 <11/02/2020> Sun.Lang
 *              - Initial version
 */
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
#include <linux/uaccess.h>

#define USE_PROC
#ifdef ADVANTECH_DEBUG
#define DEBUGPRINT     printk
#else
#define DEBUGPRINT(a, ...)
#endif

#define ADVANTECH_EC_FAN_VER        "1.00"
#define ADVANTECH_EC_FAN_DATE       "11/03/2020"

#define FAN_MAGIC       'f'
#define SETFANSPEED     _IO(FAN_MAGIC, 0)
#define GETFANSPEED     _IO(FAN_MAGIC, 1)

#define DEVICE_NAME "advfan"

struct mutex lock;

static int major;
static struct class *fan_class;

static int adspname_detect(const char *bios_product_name, const char *standard_name)
{
	int i = 0, j = 0;

	for(j = 0; j < strlen(bios_product_name); j++)
	{
		if(standard_name[i] == '*')
		{
			if(i)
			{
				if(bios_product_name[j] == standard_name[(i + 1)])
				{
					i += 2;
				}
				if(i >= (strlen(standard_name) - 1))
				{
					return 0;
				}
			}
		}
		else if(standard_name[i] == '?')
		{
			if(i)
			{
				i++;
				if(i >= strlen(standard_name))
				{
					return 0;
				}
			}
		}
		else if(bios_product_name[j] == standard_name[i])
		{
			i++;
			if(i >= strlen(standard_name))
			{
				return 0;
			}
		}
	}
	return 1;
}

int get_fan_speed(unsigned int *pvalue)
{
    int ret = -1;
    uchar value;
    uchar oldvalue;

    mutex_lock(&lock);

    //config to read correct fan
    //Setup SSrc and FPT in Fan HW Ram Control to configure feedback type
    //Ssrc=0x(TACHO1), SSrc=0x02(TACHO2)
    //FPT=0x01( two pulse type) FTP=0x02(four pulse type)
    //TachoCtl =0(FAN speed Mode, to get current fan speed) TachoCtl = 0(tune fan PWM to reach the speed which system set)

    //Read the current control register
    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 1. Send "read EC HW ram" command to EC Command port(Write 0x88 to 0x29A)
    outb(EC_HW_RAM_READ, EC_COMMAND_PORT);
    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(EC_FAN0_CTL_ADDR , EC_STATUS_PORT);
    // Step 4. Wait OBF set
    if((ret = wait_obf()))
        goto error;
    // Step 5. Get "EC HW ram" data from EC Data port (Read 0x299 port)
    oldvalue = inb(EC_STATUS_PORT);
    ///////////////////////////////////////////////////////

    value = oldvalue | FAN_TACHO_MODE | FAN0_TACHO_SOURCE | FAN0_PULSE_TYPE;

    //write to HW RAM
    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 1. Send "write EC HW ram" command to EC command port(Write 0x89 to 0x29A)
    outb(EC_HW_RAM_WRITE, EC_COMMAND_PORT);
    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(EC_FAN0_CTL_ADDR, EC_STATUS_PORT);
    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 5. Send "EC HW ram" data to EC Data port (Write data to 0x299 port)
    outb(value, EC_STATUS_PORT);
    ///////////////////////////////////////

    //FAN  speed is in ACPI RAM
    //read fan speed hight byte
    if((ret = wait_ibf()))
        goto error;

    outb(EC_ACPI_RAM_READ,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_FAN0_SPEED_ADDR,EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    value = inb(EC_STATUS_PORT);
    *pvalue = value << 8;

    //read low speed hight byte
    if((ret = wait_ibf()))
        goto error;

    outb(EC_ACPI_RAM_READ,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_FAN0_SPEED_ADDR + 1,EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    value = inb(EC_STATUS_PORT);
    *pvalue += value ;
    ///////////////////////////////

    //write old value back
    //write to HW RAM
    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 1. Send "write EC HW ram" command to EC command port(Write 0x89 to 0x29A)
    outb(EC_HW_RAM_WRITE, EC_COMMAND_PORT);
    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(EC_FAN0_CTL_ADDR, EC_STATUS_PORT);
    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
    // Step 5. Send "EC HW ram" data to EC Data port (Write data to 0x299 port)
    outb(oldvalue, EC_STATUS_PORT);
    ///////////////////////////////

    mutex_unlock(&lock);

    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}

int set_fan_speed(uchar pwm)
{
    int ret = -1;
    uchar data;
    uchar fancode;

    read_hw_ram(EC_FAN0_HW_RAM_ADDR, &fancode);

    mutex_lock(&lock);

    //Write PWM Number into index
    if((ret = wait_ibf()))
        goto error;
    outb(EC_PWM_WRITE_INDEX,EC_COMMAND_PORT);
    if((ret = wait_ibf()))
        goto error;
    outb(fancode,EC_STATUS_PORT);
    if((ret = wait_obf()))
        goto error;
    data = inb(EC_STATUS_PORT);
    if (data!= fancode){
        goto error;
    }

    //write PWM
    if((ret = wait_ibf()))
        goto error;
    outb(EC_PWM_WRITE_DATA,EC_COMMAND_PORT);
    if((ret = wait_ibf()))
        goto error;
    outb(pwm,EC_STATUS_PORT);

    mutex_unlock(&lock);
    write_hw_ram(EC_FAN0_CTL_ADDR, 0x97);

    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}

static int advfan_open( struct inode * inode, struct file * filp)
{
	return 0;
}

static int advfan_release(struct inode * inode, struct file * filp)
{
	return 0;
}

static long advfan_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int err;
    unsigned int data[3];

    err = copy_from_user(data, (void *)arg, 3 * sizeof(unsigned int));

    if (err)
    {
        printk("copy from user err!!!\n");
        return err;
    }

    switch (cmd)
    {
        case SETFANSPEED:
            {
                set_fan_speed((uchar)data[0]);
                break;
            }
        case GETFANSPEED:
            {
                get_fan_speed(&data[0]);

				if (copy_to_user((void *)arg, data, 3 * sizeof(unsigned int)))
				{
					printk("copy to user error\n");
					return EFAULT;
				}

                break;
            }
    }
	return 0;
}

static struct file_operations advfan_fops = {
	.owner = THIS_MODULE,
	.open = advfan_open,
	.release = advfan_release,
    unlocked_ioctl: advfan_ioctl,
};

static int __init advfan_init(void)
{
	char *product;

	product = BIOS_Product_Name;

	if(adspname_detect(BIOS_Product_Name,"FST-2482P")){
		printk(KERN_INFO "%s is not support EC Fan!\n", BIOS_Product_Name);
		return -ENODEV;
	}

	major = register_chrdev(0, DEVICE_NAME, &advfan_fops);

    if (major < 0)
    {
        printk("advfan register failed!\n");
        return major;
    }

    fan_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(fan_class))
    {
        printk(KERN_ERR "Error create fan class\n");
        return -1;
    }
    device_create(fan_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    mutex_init(&lock);

	printk("=====================================================\n");
	printk("     Advantech ec FAN driver V%s [%s]\n",
			ADVANTECH_EC_FAN_VER, ADVANTECH_EC_FAN_DATE);
	printk("=====================================================\n");

	return 0;
}

static void __exit advfan_exit(void)
{
    device_destroy(fan_class, MKDEV(major, 0));
    class_destroy(fan_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk("Advantech EC Fan exit!\n");
}

module_init(advfan_init);
module_exit(advfan_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jianfeng Dai <jianfeng.dai@advantech.com.cn>");
MODULE_DESCRIPTION("Advantech EC Fan PWM and Fan Tacho driver");
MODULE_VERSION(ADVANTECH_EC_FAN_VER);
