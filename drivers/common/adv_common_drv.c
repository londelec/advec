/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_common_drv.c
 * Version:     1.00  <05/11/2018>
 * Author:      Ji.Xu
 *
 * Description: The adv_common_drv is driver for get EC common status.
 *
 * Status:      working
 *
 * Change Log:
 *              EC Power Driver Version 1.00 <10/23/2017> Ji.Xu
 *              - Initial version
 *              Version 1.01 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
 *
 *              EC Common Driver Version 1.00 <05/11/2018> Ji.Xu
 *              - Initial version
 *              - Support UNO-1372G-J021AE/J031AE
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

#define COMMON_MAGIC				'p'
#define IOCTL_EC_GET_STATUS			_IO(COMMON_MAGIC, 0xB1)
#define IOCTL_EC_SET_STATUS			_IOW(COMMON_MAGIC, 0xB2, int)

#define ADVANTECH_EC_COMMON_VER		"1.00"
#define ADVANTECH_EC_COMMON_DATE	"05/11/2018" 

//#define DEBUG_MESSAGE
//#define USE_CLASS
#define POWER_LOW				0
#define	POWER_NORMAL			1

static int major_common = 0;
#ifdef USE_CLASS
static struct class *common_class;
#endif
struct mutex lock_ioctl;
typedef struct __common_command {
	uint	id;
	uint	status;
} _common_command;
_common_command common_command;

static int ioctl_oem_get_status(unsigned long pmsg)
{
	memset(&common_command, 0, sizeof(_common_command));
	if(copy_from_user(&common_command, 
				(struct _common_command *)pmsg, 
				sizeof(_common_command))) {
		printk(KERN_ERR "ioctl_get_common: copy_from_user error. \n");
		return -1;
	}

	if (ec_oem_get_status(common_command.id, &(common_command.status))) {
		printk(KERN_ERR "oem_get_status: error. \n");
		return -1;
	}

	if(copy_to_user((struct _common_command *)pmsg, 
				&common_command, 
				sizeof(_common_command))) {
		printk(KERN_ERR "ioctl_get_common: copy_to_user error. \n");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(ioctl_oem_get_status);

static int ioctl_oem_set_status(unsigned long pmsg)
{
	memset(&common_command, 0, sizeof(_common_command));
	if(copy_from_user(&common_command, (struct _common_command *)pmsg, sizeof(_common_command))) {
		printk(KERN_ERR "ioctl_set_common: copy_from_user error. \n");
		return -1;
	}

	if (ec_oem_set_status(common_command.id, common_command.status)) {
		printk(KERN_ERR "oem_set_status: error. \n");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(ioctl_oem_set_status);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int adv_common_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long adv_common_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int ret = -1;
	mutex_lock(&lock_ioctl);

	switch(cmd) 
	{
	case IOCTL_EC_GET_STATUS: 
		ret = ioctl_oem_get_status(arg);
		break;
	case IOCTL_EC_SET_STATUS: 
		ret = ioctl_oem_set_status(arg);
		break;
	default:
		ret = -1;
		break;
	}

	mutex_unlock(&lock_ioctl);
	return ret;
}

static int adv_common_open(struct inode *inode, struct file *file )
{
    return 0;
}

static int adv_common_release(struct inode *inode, struct file *file )
{
    return 0;
}

static ssize_t adv_common_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t adv_common_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	return 0;
}

static struct file_operations adv_common_fops = {
owner:		THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
			ioctl:		adv_common_ioctl,
#else
			unlocked_ioctl:	adv_common_ioctl,
#endif
			read:		adv_common_read,
			write:		adv_common_write,
			open:		adv_common_open,
			release:	adv_common_release,
};

static void adv_common_cleanup(void)
{
#ifdef USE_CLASS
    device_destroy(common_class, MKDEV(major_common, 0));
    class_destroy(common_class);
#endif
    unregister_chrdev( major_common, "adv_common" );
    printk("Advantech EC common exit!\n");
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

static int adv_common_init(void)
{
    char *product;
	product = BIOS_Product_Name;
	if ((adspname_detect(BIOS_Product_Name,"UNO-3283G/3285G-674AE")) 
				&& (adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE"))
			) {
		printk(KERN_INFO "%s is not support EC common!\n", BIOS_Product_Name);
		return -ENODEV;
    }
   
	mutex_init(&lock_ioctl);

    if ((major_common = register_chrdev(0, "adv_common", &adv_common_fops)) < 0) {
        printk("register common chrdev failed!\n");
        return -ENODEV;
    }

#ifdef USE_CLASS
    common_class = class_create(THIS_MODULE, "adv_common");
    if (IS_ERR(common_class)) {
        printk(KERN_ERR "Error creating common class.\n");
        return -1;
    }
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
    device_create(common_class, NULL, MKDEV(major_common, 0), NULL, "adv_common");
	#else
    class_device_create(common_class, NULL, MKDEV(major_common, 0), NULL, "adv_common");
	#endif
#endif
    
	printk("=====================================================\n");
    printk("     Advantech ec common driver V%s [%s]\n", 
            ADVANTECH_EC_COMMON_VER, ADVANTECH_EC_COMMON_DATE);
    printk("=====================================================\n");
    
	return 0;
}

module_init(adv_common_init);
module_exit(adv_common_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JiXu");
MODULE_DESCRIPTION("Advantech EC COMMON Driver.");
