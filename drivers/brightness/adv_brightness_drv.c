/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_brightness_drv.c
 * Version:     1.00  <10/10/2014>
 * Author:      Sun.Lang
 *
 * Description: The adv_brightness_drv is driver for controlling EC brightness.
 *                              
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <10/10/2014> Sun.Lang
 *              - Initial version
 *              Version 1.01 <12/30/2015> Jiangwei.Zhu
 *              - Modify adv_brightness_init function to install the driver to 
 *              - the support devices.
 *              Version 1.02 <03/04/2016> Jiangwei.Zhu
 *              - Support TPC-1782H-433AE
 *              Version 1.03 <05/09/2016> Ji.Xu
 *              - Modify the device name check method to fuzzy matching.
 *              Version 1.04 <06/28/2017> Ji.Xu
 *              - Support proc filesystem.
 *              Version 1.05 <10/26/2017> Ji.Xu
 *              - Support PR/VR4.
 *              Version 1.06 <02/02/2018> Ji.Xu
 *              - Support EC TPC-B500-6??AE
 *              - Support EC TPC-5???T-6??AE
 *              - Support EC TPC-5???W-6??AE
 *              Version 1.07 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
 *              Version 1.08 <02/20/2019> Ji.Xu
 *              - Support EC TPC-*81WP-4???E
 *              - Support EC TPC-B200-???AE
 *              - Support EC TPC-2???T-???AE
 *              - Support EC TPC-2???W-???AE
 *              Version 1.09 <08/30/2019> Yao.Kang
 *              - Support 32-bit programs on 64-bit kernel.
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

#define ADVANTECH_EC_BRIGHTNESS_VER           "1.09"
#define ADVANTECH_EC_BRIGHTNESS_DATE          "08/30/2019" 

//ACPI Brightness
#define BRIGHTNESS_VAL_DEFAULT 5
#define BRIGHTNESS_MIN_VALUE   0
#define BRIGHTNESS_MAX_VALUE   9

//Brightness ACPI Addr
#define BRIGHTNESS_ACPI_ADDR  0x50

#define USE_PROC

#define BRIGHTNESS_MAGIC	'p'
#define SETMINBRIGHTNESS        _IO(BRIGHTNESS_MAGIC, 1)
#define SETMAXBRIGHTNESS        _IO(BRIGHTNESS_MAGIC, 2)
#define SETBRIGHTNESS           _IO(BRIGHTNESS_MAGIC, 3)
#define GETMINBRIGHTNESS        _IO(BRIGHTNESS_MAGIC, 4)
#define GETMAXBRIGHTNESS        _IO(BRIGHTNESS_MAGIC, 5)
#define GETBRIGHTNESS           _IO(BRIGHTNESS_MAGIC, 6)

ssize_t major = 0;
static struct class *brightness_class;
static u8 brightness_min_default = BRIGHTNESS_MIN_VALUE;
static u8 brightness_max_default = BRIGHTNESS_MAX_VALUE;
static u8 brightness_value_default = BRIGHTNESS_VAL_DEFAULT;
static int adspname_detect(const char *bios_product_name, const char *standard_name);

#ifdef USE_PROC
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define PROCFS_MAX_SIZE     128
//#define PROC_DIR          "advbrightness"

#if 0
static char S_procfs_buffer[PROCFS_MAX_SIZE];  
static char S_debug = 0;  
#endif

typedef struct _wdt_info {
	unsigned int level;  
	unsigned int max_level;  
	unsigned int min_level;  
} wdt_info;     

wdt_info wdt_data[] = {
	{
		.level = BRIGHTNESS_VAL_DEFAULT,
		.max_level = BRIGHTNESS_MAX_VALUE,
		.min_level = BRIGHTNESS_MIN_VALUE,
	},
};

static int wdt_proc_read(struct seq_file *m, void *p);  

static void *c_start(struct seq_file *m, loff_t *pos)  
{  
	return *pos < 1 ? (void *)1 : NULL;  
}  

static void *c_next(struct seq_file *m, void *v, loff_t *pos)  
{  
	++*pos;  
	return NULL;  
}  

static void c_stop(struct seq_file *m, void *v)  
{  
	/*nothing to do*/  
}  

static int c_show(struct seq_file *m, void *p)  
{  
	wdt_proc_read(m, p);    
	return 0;      
}  

static struct seq_operations proc_seq_ops = {  
	.show  = c_show,  
	.start = c_start,  
	.next  = c_next,  
	.stop  = c_stop  
};  

static int wdt_proc_open(struct inode *inode, struct file *file)  
{  
	int ret = 0;      
	struct seq_file *m;      

	ret = seq_open(file, &proc_seq_ops);      
	m = file->private_data;  
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
	m->private = file->f_dentry->d_iname;
#else
	m->private = file->f_path.dentry->d_iname;
#endif

	return ret;   
}  

static int wdt_proc_read(struct seq_file *m, void *p)  
{  
	unsigned char *name;
	unsigned int level = 0, min_level = 0, max_level = 0;

	name = m->private;

	level = wdt_data[0].level;  
	min_level = wdt_data[0].min_level;  
	max_level = wdt_data[0].max_level;  

	seq_printf(m, "level      : %d\n", level);  
	seq_printf(m, "min_level  : %d\n", min_level);  
	seq_printf(m, "max_level  : %d\n", max_level);  
	seq_printf(m, "\n");      

	return 0;  
}  

#if 0
static int wdt_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *offp)  
{  
	int value;  
	int procfs_buffer_size;  

	if(count > PROCFS_MAX_SIZE) {  
		procfs_buffer_size = PROCFS_MAX_SIZE;  
	} else {  
		procfs_buffer_size = count;  
	}  

	if (copy_from_user(S_procfs_buffer, buffer, procfs_buffer_size)) {  
		return -EFAULT;  
	}  
	*(S_procfs_buffer+procfs_buffer_size) = 0;  

	if(sscanf(S_procfs_buffer , "set debug %d" , &value)==1) {  
		S_debug = value;  
	} else {  
		printk("=================\n");  
		printk("usage: \n");  
		printk("set debug <value>\n");  
		printk("=================\n");  
	}  

	return count;  
}  
#endif
#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_fops = {
  .proc_open = wdt_proc_open,
  .proc_read = seq_read,
};
#else
static struct file_operations proc_fops = {  
	.open  = wdt_proc_open,  
	.read  = seq_read,  
//	.write = wdt_proc_write,        
}; 
#endif

int wdt_create_proc(char *name)  
{  
	struct proc_dir_entry *wdt_proc_entries;  
	unsigned char proc_name[64]={0};  

#ifdef PROC_DIR
	sprintf(proc_name, "%s/%s", (const char *)PROC_DIR, name);  
#else
	sprintf(proc_name, "%s", name);  
#endif

	wdt_proc_entries = proc_create(proc_name, 0644, NULL, &proc_fops);  
	if (NULL == wdt_proc_entries) {  
		remove_proc_entry(proc_name, NULL);  
		printk("Error: Could not initialize /proc/%s\n", proc_name);  
		return -ENOMEM;  
	}  

	return 0;  
}  

int wdt_create_proc_parentdir(void)  
{  
#ifdef PROC_DIR
	struct proc_dir_entry *mydir = NULL;  

	mydir = proc_mkdir(PROC_DIR, NULL);  
	if (!mydir) {  
		printk(KERN_ERR "Can't create /proc/%s\n", (const char *)PROC_DIR);  
		return -1;  
	}  
#endif
	return 0;  
}  

void wdt_remove_proc(char *name)  
{  
	unsigned char proc_name[64]={0};  

#ifdef PROC_DIR
	sprintf(proc_name, "%s/%s", (const char *)PROC_DIR, name);  
#else
	sprintf(proc_name, "%s", name);  
#endif
	remove_proc_entry(proc_name, NULL);  
	return;  
}  

void wdt_remove_proc_parentdir(void)  
{  
#ifdef PROC_DIR
	remove_proc_entry(PROC_DIR, NULL);  
#endif
	return;  
}  
#endif // USE_PROC

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int adv_brightness_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
#else
static long adv_brightness_ioctl(struct file* filp, unsigned int cmd, unsigned long arg )
#endif
{
	uchar data;
	uchar advbrightnessvalue;

	switch ( cmd )
	{
	case SETMINBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if((data < BRIGHTNESS_MIN_VALUE) || (data > BRIGHTNESS_MAX_VALUE)) {
			return -EINVAL;
		} else {
			brightness_min_default = data;
		}
		wdt_data[0].min_level = brightness_min_default;
		break;

	case SETMAXBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if((data > BRIGHTNESS_MAX_VALUE)|(data < brightness_min_default)) {
			return -EINVAL;
		} else {
			brightness_max_default = data;
		} 
		wdt_data[0].max_level = brightness_max_default;
		break;

	case SETBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if(data < brightness_min_default) {
			return -EINVAL;
		} else if(data > brightness_max_default) {
			return -EINVAL;
		} else {
			if(!((adspname_detect(BIOS_Product_Name,"MIO-2263")) 
						&& (adspname_detect(BIOS_Product_Name,"MIO-5251"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
						)) { 
				data = BRIGHTNESS_MAX_VALUE - data;
			}
			brightness_value_default = data;
		}
		read_acpi_value(BRIGHTNESS_ACPI_ADDR,&data);	
		if(data & 0x80) {
			write_acpi_value(BRIGHTNESS_ACPI_ADDR,brightness_value_default+128);
		} else {
			write_acpi_value(BRIGHTNESS_ACPI_ADDR,brightness_value_default);
		}
		wdt_data[0].level = brightness_value_default;
		break;

	case GETMINBRIGHTNESS:
		data = brightness_min_default;
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;

	case GETMAXBRIGHTNESS:
		data = brightness_max_default;
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;

	case GETBRIGHTNESS:
		read_acpi_value(BRIGHTNESS_ACPI_ADDR,&advbrightnessvalue);
		/*Set data to 0~9*/
		if(advbrightnessvalue & 0x80) {
			data = advbrightnessvalue - 128;
		} else {
			data = advbrightnessvalue;
		}
		if(!((adspname_detect(BIOS_Product_Name,"MIO-2263")) 
					&& (adspname_detect(BIOS_Product_Name,"MIO-5251"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
					)) { 
			data = BRIGHTNESS_MAX_VALUE - data;
		}
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static long adv_brightness_compat_ioctl(struct file* filp, unsigned int cmd, unsigned long arg )
{
	uchar data;
	uchar advbrightnessvalue;

	switch ( cmd )
	{
	case SETMINBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if((data < BRIGHTNESS_MIN_VALUE) || (data > BRIGHTNESS_MAX_VALUE)) {
			return -EINVAL;
		} else {
			brightness_min_default = data;
		}
		wdt_data[0].min_level = brightness_min_default;
		break;

	case SETMAXBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if((data > BRIGHTNESS_MAX_VALUE)|(data < brightness_min_default)) {
			return -EINVAL;
		} else {
			brightness_max_default = data;
		} 
		wdt_data[0].max_level = brightness_max_default;
		break;

	case SETBRIGHTNESS:
		if(copy_from_user(&data, (void *)arg, sizeof(uchar))) {
			return -EFAULT;
		}
		if(data < brightness_min_default) {
			return -EINVAL;
		} else if(data > brightness_max_default) {
			return -EINVAL;
		} else {
			if(!((adspname_detect(BIOS_Product_Name,"MIO-2263")) 
						&& (adspname_detect(BIOS_Product_Name,"MIO-5251"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
						&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
						)) { 
				data = BRIGHTNESS_MAX_VALUE - data;
			}
			brightness_value_default = data;
		}
		read_acpi_value(BRIGHTNESS_ACPI_ADDR,&data);	
		if(data & 0x80) {
			write_acpi_value(BRIGHTNESS_ACPI_ADDR,brightness_value_default+128);
		} else {
			write_acpi_value(BRIGHTNESS_ACPI_ADDR,brightness_value_default);
		}
		wdt_data[0].level = brightness_value_default;
		break;

	case GETMINBRIGHTNESS:
		data = brightness_min_default;
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;

	case GETMAXBRIGHTNESS:
		data = brightness_max_default;
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;

	case GETBRIGHTNESS:
		read_acpi_value(BRIGHTNESS_ACPI_ADDR,&advbrightnessvalue);
		/*Set data to 0~9*/
		if(advbrightnessvalue & 0x80) {
			data = advbrightnessvalue - 128;
		} else {
			data = advbrightnessvalue;
		}
		if(!((adspname_detect(BIOS_Product_Name,"MIO-2263")) 
					&& (adspname_detect(BIOS_Product_Name,"MIO-5251"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
					&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
					)) { 
			data = BRIGHTNESS_MAX_VALUE - data;
		}
		if(copy_to_user((void *)arg, &data, sizeof(uchar))) {
			return -EFAULT;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static int adv_brightness_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int adv_brightness_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t adv_brightness_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t adv_brightness_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	return 0;
}

static struct file_operations adv_brightness_fops = {
owner:		THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
			ioctl:		adv_brightness_ioctl,
#else
			unlocked_ioctl: adv_brightness_ioctl,
#endif
			compat_ioctl:	adv_brightness_compat_ioctl,
			read:		adv_brightness_read,
			write:		adv_brightness_write,
			open:		adv_brightness_open,
			release:	adv_brightness_release,
};

void adv_brightness_cleanup(void)
{
	device_destroy(brightness_class, MKDEV(major, 0));
	class_destroy(brightness_class);
	unregister_chrdev( major, "advbrighntess" );

#ifdef USE_PROC
	wdt_remove_proc("advbrightnessinfo");
#ifdef PROC_DIR
	wdt_remove_proc_parentdir();
#endif
#endif
	printk("Advantech EC brightness exit!\n");
}

static int adspname_detect(const char *bios_product_name, const char *standard_name)
{
	int i = 0, j = 0;

	for(j = 0; j < strlen(bios_product_name); j++)
	{
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

int adv_brightness_init (void)
{
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
			&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
			) {
		printk(KERN_INFO "%s is not support EC brightness!\n", BIOS_Product_Name);
		return -ENODEV;
	}

	if((major = register_chrdev(0, "advbrightness", &adv_brightness_fops)) < 0) {
		printk("register chrdev failed!\n");
		return -ENODEV;
	}

	brightness_class = class_create(THIS_MODULE, "advbrightness");
	if(IS_ERR(brightness_class)) {
		printk(KERN_ERR "Error creating brightness class.\n");
		return -1;
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
	device_create(brightness_class, NULL, MKDEV(major, 0), NULL, "advbrightness");
#else
	class_device_create(brightness_class, NULL, MKDEV(major, 0), NULL, "advbrightness");
#endif

#ifdef USE_PROC
#ifdef PROC_DIR
	wdt_create_proc_parentdir();  
#endif
	wdt_create_proc("advbrightnessinfo");
	wdt_data[0].level = BRIGHTNESS_VAL_DEFAULT;
	wdt_data[0].max_level = BRIGHTNESS_MAX_VALUE;
	wdt_data[0].min_level = BRIGHTNESS_MIN_VALUE;
#endif

	printk("=====================================================\n");
	printk("     Advantech ec brightness driver V%s [%s]\n", 
			ADVANTECH_EC_BRIGHTNESS_VER, ADVANTECH_EC_BRIGHTNESS_DATE);
	printk("=====================================================\n");
	return 0;
}

module_init( adv_brightness_init );
module_exit( adv_brightness_cleanup );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Advantech EC Brightness Driver.");
MODULE_VERSION(ADVANTECH_EC_BRIGHTNESS_VER);

