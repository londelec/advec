/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_gpio_drv.c
 * Version:     1.00  <09/15/2014>
 * Author:      Sun.Lang
 *
 * Description: The adv_gpio_drv is driver for controlling EC gpio.
 *                              
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <09/15/2014> Sun.Lang
 *              - Initial version
 *              Version 1.01 <12/30/2015> Jiangwei.Zhu
 *              - Modify adv_gpio_init function to install the driver to 
 *              - the support devices.
 *              Version 1.02 <03/04/2016> Jiangwei.Zhu
 *              - Support UNO-1372G-E3AE
 *              Version 1.03 <05/09/2016> Ji.Xu
 *              - Modify the device name check method to fuzzy matching.
 *              Version 1.04 <05/09/2017> Ji.Xu
 *              - Support UNO-2473G-JxAE.
 *              Version 1.05 <11/16/2017> Zhang.Yang
 *              - Support UNO-1372G-J021AE/J031AE.
 *              Version 1.06 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
 *              Version 1.07 <10/17/2018> Ji.Xu
 *              - Support UNO-420
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
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/uaccess.h>
#endif
#include <linux/delay.h>
#include <linux/device.h>
#include "../mfd-ec/ec.h"
//ioctl command 
#define EC_MAGIC		'p'
#define SETGPIODIR              _IO(EC_MAGIC, 7)
#define GETGPIODIR              _IO(EC_MAGIC, 8)
#define ECSETGPIOSTATUS         _IO(EC_MAGIC, 9)
#define ECGETGPIOSTATUS         _IO(EC_MAGIC, 0x0a)

#define ADVANTECH_EC_GPIO_VER           "1.07"
#define ADVANTECH_EC_GPIO_DATE          "10/17/2018" 

static int major_gpio = 0;

static struct class *gpio_class;
static uchar gpio_table[8];

static int adv_get_gpio(void)
{
    int i;
    if(NULL == PDynamic_Tab){
        printk("null pointer\n");
        return -ENODATA;
    }

    for(i=0;i<EC_MAX_TBL_NUM;i++)
    {
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_0)
        {
            gpio_table[0] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }

        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_1)
        {
            gpio_table[1] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }

        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_2)
        {
            gpio_table[2] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_3)
        {
            gpio_table[3] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_4)
        {
            gpio_table[4] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_5)
        {
            gpio_table[5] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_6)
        {
            gpio_table[6] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if( PDynamic_Tab[i].DeviceID == EC_DID_ALTGPIO_7)
        {
            gpio_table[7] = PDynamic_Tab[i].HWPinNumber;
            continue;
        }
        if((0 != gpio_table[0]) && (0 != gpio_table[1]) && (0 != gpio_table[2]) && (0 != gpio_table[3]) && (0 != gpio_table[4]) && (0 != gpio_table[5]) && (0 != gpio_table[6]) && (0 != gpio_table[7]))
        {
            break;
        }
    }
    return 0;
}

unsigned int checkHwpinNumber(unsigned int index)
{
    unsigned int PinNumber;
    if(index <0 || index > 7)
    {
        printk("adv_gpio set wrong index!!");
        return false;
    }
    else
    {
        PinNumber = gpio_table[index];
    }
    return PinNumber;
}

static int adv_gpio_set_dir(uchar value,uchar index)
{
    uchar pin_number=checkHwpinNumber(index);
    return write_gpio_dir(pin_number,value);
}

static int adv_gpio_get_dir(uchar *pvalue, uchar index)
{
    uchar pin_number=checkHwpinNumber(index);
    return read_gpio_dir(pin_number,pvalue);
}

static int adv_gpio_set_status(uchar value,uchar index)
{
    uchar pin_number=checkHwpinNumber(index);
    return write_gpio_status(pin_number,value);
}
static int adv_gpio_get_status(uchar *pvalue, uchar index)
{
    uchar pin_number=checkHwpinNumber(index);
    return read_gpio_status(pin_number,pvalue);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int adv_gpio_ioctl (
        struct inode *inode, 
        struct file *filp, 
        unsigned int cmd, 
        unsigned long arg)
#else
static long adv_gpio_ioctl (
        struct file* filp,
        unsigned int cmd,
        unsigned long arg )
#endif
{
    unsigned int data[2];

    switch ( cmd )
    {
        case SETGPIODIR:
            if(copy_from_user(data, (void *)arg, 2 * sizeof(signed int)))
            {
                printk("copy from user error.\n");
                return -EFAULT;
            }
            if((data[0] < 0) || (data[0] > 7))
            {
                return -EINVAL;
            }
            adv_gpio_set_dir(data[1],data[0]);
            break;

        case GETGPIODIR:
            if(copy_from_user(data,(void *)arg,sizeof(unsigned int)))
            {
                printk("copy from user error\n");
                return -EFAULT;
            }
            if((data[0] < 0) || (data[0] > 7))
            {
                return -EINVAL;
            }
            adv_gpio_get_dir(&data[1],data[0]);
            if(copy_to_user((void *)arg,data,2 * sizeof(unsigned int)))
            {
                return -EFAULT;
            }
            break;

        case ECSETGPIOSTATUS:
            if(copy_from_user(data, (void *)arg, 2 * sizeof(signed int)))
            {	
                return -EFAULT;
            }
            if((data[0] < 0) || (data[0] > 7))
            {
                return -EINVAL;
            }
            adv_gpio_set_status(data[1],data[0]);
            break;

        case ECGETGPIOSTATUS:
            if(copy_from_user(data,(void *)arg,sizeof(unsigned int)))
            {
                return -EFAULT;
            }
            if((data[0] < 0) || (data[0] > 7))
            {
                return -EINVAL;
            }
            adv_gpio_get_status(&data[1],data[0]);
            if(copy_to_user((void *)arg,data,2 * sizeof(unsigned int)))
            {
                return EFAULT;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

static int adv_gpio_open (
        struct inode *inode, 
        struct file *file )
{
    return 0;
}

static int adv_gpio_release (
        struct inode *inode, 
        struct file *file )
{
    return 0;
}

static struct file_operations adv_gpio_fops = {
owner:		THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
            ioctl:		adv_gpio_ioctl,
#else
            unlocked_ioctl: adv_gpio_ioctl,
#endif
            open:		adv_gpio_open,
            release:	adv_gpio_release,
};

static void adv_ec_cleanup (void)
{
    device_destroy(gpio_class, MKDEV(major_gpio, 0));
    class_destroy(gpio_class);
    unregister_chrdev( major_gpio, "advgpio" );
    printk("Advantech EC gpio exit!\n");
}

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

static int adv_ec_init (void)
{
    char *product;
    product = BIOS_Product_Name;
	if((adspname_detect(BIOS_Product_Name,"TPC-8100TR"))
			&& (adspname_detect(BIOS_Product_Name,"UNO-1372G-E?AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-420")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-1483G-4??AE"))
			&& (adspname_detect(BIOS_Product_Name,"UNO-2473G-J?AE"))) 
    {
		printk(KERN_INFO "%s is not support EC gpio!\n", BIOS_Product_Name);
		return -ENODEV;
    }
    
    if((major_gpio = register_chrdev(0, "advgpio", &adv_gpio_fops)) < 0)
    {
        printk("register gpio chrdev failed!\n");
        return -ENODEV;
    }
    gpio_class = class_create(THIS_MODULE, "advgpio");
    if(IS_ERR(gpio_class))
    {
        printk(KERN_ERR "Error creating gpio class.\n");
        return -1;
    }
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
    device_create(gpio_class, NULL, MKDEV(major_gpio, 0), NULL, "advgpio");
#else
    class_device_create(gpio_class, NULL, MKDEV(major_gpio, 0), NULL, "advgpio");
#endif
    adv_get_gpio();
    printk("=====================================================\n");
    printk("     Advantech ec gpio driver V%s [%s]\n", 
            ADVANTECH_EC_GPIO_VER, ADVANTECH_EC_GPIO_DATE);
    printk("=====================================================\n");
    return 0;
}

module_init( adv_ec_init );
module_exit( adv_ec_cleanup );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SunLang");
MODULE_DESCRIPTION("Advantech EC GPIO Driver.");
