/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_watchdog_drv.c
 * Version:     1.00  <10/10/2014>
 * Author:      Sun.Lang
 *
 * Description: The adv_watchdog_drv is driver for controlling EC watchdog.
 *                              
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <10/10/2014> Sun.Lang
 *              - Initial version
 *              Version 1.01 <12/30/2015> Jiangwei.Zhu
 *              - Modify adv_watchdog_init function to install the driver to 
 *              - the support devices.
 *              Version 1.02 <03/04/2016> Jiangwei.Zhu
 *              - Support UNO-1372G-E3AE, TPC-1782H-433AE, APAX-5580-433AE
 *              Version 1.03 <05/09/2016> Ji.Xu
 *              - Support EC watchdog mini-board on UNO-3083G/3085G-D44E/D64E
 *              - APAX-5580-473AE/4C3AE.
 *              - Modify the timeout unit to 1 second.
 *              - Modify the device name check method to fuzzy matching.
 *              Version 1.04 <06/28/2017> Ji.Xu
 *              - Support EC UNO-2271G-E2xAE.
 *              - Support EC UNO-2271G-E02xAE.
 *              - Support EC UNO-2473G-JxAE.
 *              - Support proc filesystem.
 *              Version 1.05 <09/20/2017> Ji.Xu
 *              - Support EC UNO-2484G-633xAE.
 *              - Support EC UNO-2484G-653xAE.
 *              - Support EC UNO-2484G-673xAE.
 *              - Support EC UNO-2484G-733xAE.
 *              - Support EC UNO-2484G-753xAE.
 *              - Support EC UNO-2484G-773xAE.
 *              Version 1.06 <10/26/2017> Ji.Xu
 *              - Support EC UNO-3283G-674AE
 *              - Support EC UNO-3285G-674AE
 *              Version 1.07 <11/16/2017> Zhang.Yang
 *              - Support EC UNO-1372G-J021AE/J031AE
 *              - Support EC UNO-2372G
 *              Version 1.08 <02/02/2018> Ji.Xu
 *              - Support EC TPC-B500-6??AE
 *              - Support EC TPC-5???T-6??AE
 *              - Support EC TPC-5???W-6??AE
 *              Version 1.09 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
 *              Version 1.10 <10/11/2018> Ji.Xu
 *              - Support EC UNO-420
 -----------------------------------------------------------------------------*/

#include <linux/version.h>
#ifndef KERNEL_VERSION
#define  KERNEL_VERSION(a, b, c) KERNEL_VERSION((a)*65536+(b)*256+(c))
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/uaccess.h>
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
#include <asm/switch_to.h>
#else
#include <asm/system.h>
#endif
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include "../mfd-ec/ec.h"

#define USE_PROC
//#define USE_MISC
#ifdef ADVANTECH_DEBUG
#define DEBUGPRINT     printk
#else
#define DEBUGPRINT(a, ...)
#endif

#define ADVANTECH_EC_WDT_VER        "1.10"
#define ADVANTECH_EC_WDT_DATE       "10/11/2018"

static unsigned long advwdt_is_open;
static unsigned short timeout = 450;
static char adv_expect_close;
static unsigned int major = 0;
struct mutex lock_ioctl;

#ifdef USE_PROC

#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define PROCFS_MAX_SIZE     128
//#define PROC_DIR          "advwdt"

#if 0
static char S_procfs_buffer[PROCFS_MAX_SIZE];  
static char S_debug = 0;  
#endif

typedef struct _wdt_info {
	unsigned char chip_name[32];  
	unsigned long current_timeout;  
	unsigned char is_enable[8];  
} wdt_info;     

wdt_info wdt_data[] = {
	{
		.chip_name = "Advantech Embedded Controller",
		.current_timeout = 45,
		.is_enable = "No",
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
	unsigned char *name, *chip_name, *is_enable;
	unsigned long current_timeout = 0;

	name = m->private;

	chip_name = wdt_data[0].chip_name;  
	current_timeout = wdt_data[0].current_timeout;  
	is_enable = wdt_data[0].is_enable;  

	seq_printf(m, "name       : %s\n", chip_name);  
	seq_printf(m, "timeout    : %ld\n", current_timeout);
	seq_printf(m, "is_enable  : %s\n", is_enable);  
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

static struct file_operations proc_fops = {  
	.open  = wdt_proc_open,  
	.read  = seq_read,  
//	.write = wdt_proc_write,        
};  

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

#ifdef CONFIG_WATCHDOG_NOWAYOUT
static int nowayout = 1;
#else
static int nowayout = 0;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
MODULE_PARM(nowayout,"i");
#else
module_param(nowayout, int, 0);
#endif
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once start(default=CONFIG_WATCHDOG_NOWAYOUT)");

#ifndef MODULE
static int __init adv_setup(char *str)
{
	int ints[4];
	str = get_options(str, ARRAY_SIZE(ints), ints);

	if(ints[0] > 0)
	{
		advwdt_base_addr = ints[1];
		if(ints[0] > 1)
			advwdt_base_addr = ints[2];
	}
	return 1;
}

__setup("advwdt=", adv_setup);

#endif /* !MODULE */
	static ssize_t
advwdt_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
	if (ppos != &file->f_pos)
		return -ESPIPE;
#endif
	return count;
}


static int set_delay(unsigned short delay_timeout)
{
	if ( write_hw_ram(EC_RESET_DELAY_TIME_L, delay_timeout & 0x00FF))
	{
		printk("Failed to set Watchdog Retset Time Low byte.\n");
		return -EINVAL;
	}
	if ( write_hw_ram(EC_RESET_DELAY_TIME_H, (delay_timeout & 0xFF00)>>8))
	{
		printk("Failed to set Watchdog Retset Time Hight byte.\n");
		return -EINVAL;
	}
	return 0;
	/**reset end**/
}

static int advwdt_set_heartbeat(unsigned long t)
{
	if(t < 1 || t > 6553 )
	{
		return -EINVAL;
	}
	else
	{
		timeout = (t*10);
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int advwdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
#else
static long advwdt_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
#endif
{
	unsigned long new_timeout;
	void __user *argp = (void __user *)arg;
	int __user *p = argp;

	static struct watchdog_info ident = 
	{
options:            WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE, 

					firmware_version:     0,
					identity:            "Advantech WDT"
	};

	mutex_lock(&lock_ioctl);
	if(advwdt_is_open < 1)
	{
		printk("watchdog does not open.\n");
		mutex_unlock(&lock_ioctl);
		return -1;
	}

	switch ( cmd ) 
	{
	case WDIOC_GETSUPPORT:
		if (copy_to_user(argp, &ident, sizeof(ident)))
		{
			mutex_unlock(&lock_ioctl);
			return -EFAULT;
		}
		break;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		mutex_unlock(&lock_ioctl);
		return put_user(0, p);

	case WDIOC_KEEPALIVE:
		if (write_hwram_command(EC_WDT_RESET))
		{
			printk("Failed to set Watchdog reset.\n");
			return -EINVAL;
		}
		break;

	case WDIOC_SETTIMEOUT:
		if (get_user( new_timeout, (unsigned long *)arg))
		{
			mutex_unlock(&lock_ioctl);
			return -EFAULT;
		}
		if( advwdt_set_heartbeat(new_timeout) )   
		{
			printk("Advantch WDT: the input timeout is out of range.\n");
			printk("Please choose valid data between 1 ~ 6553.\n");
			// printk("Because an unit means 0.1 seconds, data 100 is 10 seconds.\n");
			mutex_unlock(&lock_ioctl);

			return -EINVAL; 
		}
		else
		{
			// if (set_delay((unsigned short)(new_timeout-1)))
			if (set_delay((unsigned short)(timeout-1)))
			{
				printk("Falied to set Watchdog delay.\n");
				return -EINVAL;
			}
			if ( write_hwram_command(EC_WDT_START))
			{
				printk("Failed to set Watchdog start.\n");
				return -EINVAL;
			}
			wdt_data[0].is_enable[0] = 'Y';
			wdt_data[0].is_enable[1] = 'e';
			wdt_data[0].is_enable[2] = 's';
		}
		wdt_data[0].current_timeout = timeout/10;
		break;

	case WDIOC_GETTIMEOUT: 
		if( timeout==0 )   	   
		{
			mutex_unlock(&lock_ioctl);
			return -EFAULT;
		}   
		mutex_unlock(&lock_ioctl);
		// return put_user( timeout, (unsigned long *)arg);
		return put_user( timeout/10, (unsigned long *)arg);

	case WDIOC_SETOPTIONS:	
		{
			int options;
			if (get_user(options, p))
			{
				mutex_unlock(&lock_ioctl);
				return -EFAULT;
			}
			if (options & WDIOS_DISABLECARD) 
			{
				if (write_hwram_command(EC_WDT_STOP))
				{
					printk("Failed to set Watchdog stop.\n");
					return -EINVAL;
				}
				wdt_data[0].is_enable[0] = 'N';
				wdt_data[0].is_enable[1] = 'o';
				wdt_data[0].is_enable[2] = '\0';
			}
			if (options & WDIOS_ENABLECARD) 
			{
				if ( write_hwram_command(EC_WDT_STOP))
				{
					printk("Failed to set Watchdog stop.\n");
					return -EINVAL;
				}
				if (set_delay((unsigned short)(timeout-1)))
				{
					printk("Failed to set Watchdog delay.\n");
					return -EINVAL;
				}
				if ( write_hwram_command(EC_WDT_START))
				{
					printk("Failed to set Watchdog start.\n");
					return -EINVAL;
				}
				wdt_data[0].is_enable[0] = 'Y';
				wdt_data[0].is_enable[1] = 'e';
				wdt_data[0].is_enable[2] = 's';
			}
			mutex_unlock(&lock_ioctl);
			return 0;
		}

	default:
		mutex_unlock(&lock_ioctl);
		return -ENOTTY;
	}
	mutex_unlock(&lock_ioctl);
	return 0;
}

	static int
advwdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &advwdt_is_open))
		return -EBUSY;
	if ( write_hwram_command(EC_WDT_STOP))
	{
		printk("Failed to set Watchdog stop.\n");
		return -EINVAL;
	}
	wdt_data[0].is_enable[0] = 'N';
	wdt_data[0].is_enable[1] = 'o';
	wdt_data[0].is_enable[2] = '\0';
	return 0;
}

	static int
advwdt_close(struct inode *inode, struct file *file)
{
	clear_bit(0, &advwdt_is_open);
	adv_expect_close = 0;
	return 0;
}

/*
 *	Notifier for system down
 */
	static int
advwdt_notify_sys(struct notifier_block *this, unsigned long code, void *unused)
{
	if (code == SYS_DOWN || code == SYS_HALT) {
		/* Turn the WDT off */
		if (write_hwram_command(EC_WDT_STOP)) {
			printk("Failed to set Watchdog stop.\n");
			return -EINVAL;
		}
		wdt_data[0].is_enable[0] = 'N';
		wdt_data[0].is_enable[1] = 'o';
		wdt_data[0].is_enable[2] = '\0';
		printk("sys shutdown be notified by advwdt_notify_sys() \n");
	}
	return NOTIFY_DONE;
}

/*
 *	Kernel Interfaces
 */
static struct file_operations advwdt_fops = {
owner:		THIS_MODULE,
			write:	advwdt_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
			ioctl:      advwdt_ioctl,
#else
			unlocked_ioctl: advwdt_ioctl,
#endif
			open:		advwdt_open,
			release:	advwdt_close,
};

#ifdef USE_MISC
static struct miscdevice advwdt_miscdev = {
minor:	WATCHDOG_MINOR,
	name:	"watchdog",			
	fops:	&advwdt_fops,
};
#endif

/*
 *	The WDT needs to learn about soft shutdowns in order to
 *	turn the timebomb registers off.
 */ 

static struct notifier_block advwdt_notifier = {
	advwdt_notify_sys,
	NULL,
	0
};

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

static int __init advwdt_init(void)
{   
	char *product;
	product = BIOS_Product_Name;
	if((adspname_detect(BIOS_Product_Name,"TPC-8100TR")) 
			&& (adspname_detect(BIOS_Product_Name,"MIO-2263")) 
			&& (adspname_detect(BIOS_Product_Name,"MIO-5251")) 
			&& (adspname_detect(BIOS_Product_Name,"TPC-*51T-E??E")) 
			&& (adspname_detect(BIOS_Product_Name,"TPC-*51WP-E?AE"))
			&& (adspname_detect(BIOS_Product_Name,"TPC-1?81WP-4??AE"))
			&& (adspname_detect(BIOS_Product_Name,"TPC-1?82H-4???E")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-1372G-E?AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-1372G-J0?1AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-1483G-4??AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2271G-E??AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2271G-E???AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-420")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2372G")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2473G-J?AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2483G-4??AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2484G-6???AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-2484G-7???AE")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-3083G/3085G-D??E")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-308?G-D??E")) 
			&& (adspname_detect(BIOS_Product_Name,"UNO-3283G/3285G-674AE"))
			&& (adspname_detect(BIOS_Product_Name,"UNO-3483G-3??AE")) 
			&& (adspname_detect(BIOS_Product_Name,"APAX-5580-4??AE"))
			&& (adspname_detect(BIOS_Product_Name,"TPC-B500-6??AE"))
			&& (adspname_detect(BIOS_Product_Name,"TPC-5???T-6??AE"))
			&& (adspname_detect(BIOS_Product_Name,"TPC-5???W-6??AE"))
			&& (adspname_detect(BIOS_Product_Name,"PR/VR4"))
			) {
		printk(KERN_INFO "%s is not support EC watchdog!\n", BIOS_Product_Name);
		return -ENODEV;
	}
	mutex_init(&lock_ioctl);

#ifdef USE_MISC
	misc_register(&advwdt_miscdev);
#else
	if((major=register_chrdev(0,"adv_watchdog",&advwdt_fops))<0)
	{
		printk("Advwdt register chrdev failed!\n");
	}
#endif
	register_reboot_notifier(&advwdt_notifier);

#ifdef USE_PROC
#ifdef PROC_DIR
	wdt_create_proc_parentdir();  
#endif
	wdt_create_proc("advwdtinfo");  
	wdt_data[0].current_timeout = timeout/10;
	wdt_data[0].is_enable[0] = 'N';
	wdt_data[0].is_enable[1] = 'o';
	wdt_data[0].is_enable[2] = '\0';
#endif

	printk("=====================================================\n");
	printk("     Advantech ec watchdog driver V%s [%s]\n",
			ADVANTECH_EC_WDT_VER, ADVANTECH_EC_WDT_DATE);
	printk("=====================================================\n");

	return 0;
}

static void __exit advwdt_exit(void)
{
	if ( write_hwram_command(EC_WDT_STOP))
	{
		printk("Failed to set Watchdog stop.\n");
		return;
	}
	wdt_data[0].is_enable[0] = 'N';
	wdt_data[0].is_enable[1] = 'o';
	wdt_data[0].is_enable[2] = '\0';
	clear_bit(0, &advwdt_is_open);
	adv_expect_close = 0;
	printk("Driver uninstall, set Watchdog stop.\n");
	unregister_reboot_notifier(&advwdt_notifier);
#ifdef USE_MISC
	misc_deregister(&advwdt_miscdev);
#else
	unregister_chrdev(major,"adv_watchdog");
#endif

#ifdef USE_PROC
	wdt_remove_proc("advwdtinfo");
#ifdef PROC_DIR
	wdt_remove_proc_parentdir();
#endif
#endif
	printk("Advantech EC watchdog exit!\n");
}

module_init(advwdt_init);
module_exit(advwdt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Advantech EC Watchdog Driver.");



