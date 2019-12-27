/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv_led_drv.c
 * Version:     1.00  <06/23/2016>
 * Author:      Ji.Xu
 *
 * Description: The adv_led_drv is driver for controlling EC led.
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <06/23/2016> Ji.Xu
 *              - Initial version
 *              Version 1.01 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
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

#define LED_MAGIC					'p'
#define IOCTL_EC_HWRAM_GET_VALUE	_IO(LED_MAGIC, 0x0B)	//IOCTL Command: EC Hardware Ram
#define IOCTL_EC_HWRAM_SET_VALUE	_IO(LED_MAGIC, 0x0C)
//#define IOCTL_EC_DYNAMIC_TBL_GET	_IO(LED_MAGIC, 0x0D)
#define IOCTL_EC_ONEKEY_GET_STATUS	_IO(LED_MAGIC, 0x0D)	//IOCTL Command: EC One Key Recovery
#define IOCTL_EC_ONEKEY_SET_STATUS	_IO(LED_MAGIC, 0x0E)
#define IOCTL_EC_LED_USER_DEFINE	_IOW(LED_MAGIC, 0x0F, int)	//IOCTL Command: EC LED Control
#define IOCTL_EC_LED_CONTROL_ON		_IO(LED_MAGIC, 0xA0)
#define IOCTL_EC_LED_CONTROL_OFF	_IO(LED_MAGIC, 0xA1)
#define IOCTL_EC_LED_BLINK_ON		_IO(LED_MAGIC, 0xA2)
#define IOCTL_EC_LED_BLINK_OFF		_IO(LED_MAGIC, 0xA3)
#define IOCTL_EC_LED_BLINK_SPACE	_IO(LED_MAGIC, 0xA4)

#define GET_ONE_KEY_RECOVERY_FUNCTION_FLAG	0x00
#define DISABLE_RECOVERYSTORAGE_FLAG		0x02
#define DISABLE_CREATE_STORAGE_IMAGE_FLAG	0x03
#define CREATE_STORAGE_IMAGE_BIT			0x01
#define STORAGE_RECOVERY_BIT				(0x01 << 1)

#define ADVANTECH_EC_LED_VER           "1.01"
#define ADVANTECH_EC_LED_DATE          "03/20/2018" 

//#define DEBUG_MESSAGE

static int major_led = 0;
#ifdef USE_CLASS
static struct class *led_class;
#endif
static uint led_table[8];

struct mutex lock_ioctl;

typedef struct __led_command {
	uint	device_id;
	uint	flash_type;
	uint	flash_rate;
} _led_command;
_led_command led_command;

int adv_get_led(void)
{
    int i;
    if(NULL == PDynamic_Tab) {
        printk("adv_get_led: NULL Pointer. \n");
        return -ENODATA;
    }

    for(i = 0; i < EC_MAX_TBL_NUM; i++)
    {
        if(PDynamic_Tab[i].DeviceID == EC_DID_LED_RUN) {
            led_table[0] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[0]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }

        if(PDynamic_Tab[i].DeviceID == EC_DID_LED_ERR) {
            led_table[1] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[1]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }

        if(PDynamic_Tab[i].DeviceID == EC_DID_LED_SYS_RECOVERY) {
            led_table[2] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[2]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }

        if(PDynamic_Tab[i].DeviceID == EC_DID_LED_D105_G) {
            led_table[3] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[3]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }
        
		if(PDynamic_Tab[i].DeviceID == EC_DID_LED_D106_G) {
            led_table[4] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[4]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }

        if(PDynamic_Tab[i].DeviceID == EC_DID_LED_D107_G) {
            led_table[5] = PDynamic_Tab[i].HWPinNumber;
#ifdef DEBUG_MESSAGE
        	printk("adv_get_led: led_table[5]=%02X(%d). \n", PDynamic_Tab[i].HWPinNumber, PDynamic_Tab[i].HWPinNumber);
#endif
            continue;
        }

        if((0 != led_table[0]) && (0 != led_table[1]) && (0 != led_table[2]) 
				&& (0 != led_table[3]) && (0 != led_table[4]) && (0 != led_table[5]))
            break;
    }

    return 0;
}

uint check_hw_pin_number(uint led_index)
{
//    uint hw_pin_number = 0;

    if((led_index >= EC_DID_LED_RUN) && (led_index <= EC_DID_LED_D107_G)) {
        printk("check_hw_pin_number: adv_led set right index! \n");
//        hw_pin_number = led_table[led_index];
		return true;
    } else {
        printk("check_hw_pin_number: adv_led set wrong index! \n");
		return false;
    }

//    return hw_pin_number;
}

int set_led_control_value(uint device_id, uint value)
{
	uint led_index = 0;
	uint N = 0;
	uint hw_pin_number = 0xFF;
	uint value_low = value & 0xFF;
	uint value_high = (value >> 8) & 0xFF;

	if(value == 0)
		return false;

	led_index = device_id - EC_DID_LED_RUN;
	hw_pin_number = led_table[led_index];

#ifdef DEBUG_MESSAGE
	printk("set_led_control_value: led_index: %d. \n", led_index);
	printk("set_led_control_value: hw_pin_number: %02X(%d). \n", hw_pin_number, hw_pin_number);
#endif

	if(hw_pin_number == 0xFF)
		return false;

	if(device_id > EC_DID_LED_SYS_RECOVERY)
		N = 3;
	else
		N = led_index;

	// Step 0. Write HW pin number to LED HW pin address
	if(write_hw_ram(EC_HWRAM_LED_PIN(N), hw_pin_number)) {
		return false;
	}

	// Step 1. Write 0 to Bit 4 in LED control low byte (offset 0x02) for disable control
	if(write_hw_ram(EC_HWRAM_LED_CTRL_LOBYTE(N), (value_low & ((uchar)(~LED_CTRL_ENABLE_BIT))))) {
		return false;
	}

	// Step 2. Write value to LED control high byte (offset 0x01)
	if(write_hw_ram(EC_HWRAM_LED_CTRL_HIBYTE(N), value_high)) {
		return false;
	}
	
	// Step 3. Set ENABLE Bit in LED control low byte (offset 0x02)
	value_low |= LED_CTRL_ENABLE_BIT;	

	// Step 4. Write value to LED control low byte (offset 0x02)
	if(write_hw_ram(EC_HWRAM_LED_CTRL_LOBYTE(N), value_low)) {
		return false;
	}

	return true;
}

int get_led_control_value(uint device_id, uint *pvalue)
{
	uint led_index = 0;
	uint N = 0;
	uint hw_pin_number = 0xFF;
	uint value_low = 0;
	uint value_high = 0;
	uint current_device_id = 0;
	uint ret_value = 0;

	if(pvalue == NULL) {
		printk(KERN_ERR "get_led_control_value: NULL point. \n");
		return false;
	}

	led_index = device_id - EC_DID_LED_RUN;
	hw_pin_number = led_table[led_index];

	if(hw_pin_number == 0xFF)
		return false;

	if(device_id > EC_DID_LED_SYS_RECOVERY)
		N = 3;
	else
		N = led_index;

#ifdef DEBUG_MESSAGE
	printk("get_led_control_value: led_index: %d.\tN: %d. \n", led_index, N);
	printk("get_led_control_value: HWPIN: %02X(%d). \n", hw_pin_number, hw_pin_number);
	printk("get_led_control_value: HWRAM_PIN: %02X(%d). \n", EC_HWRAM_LED_PIN(N), EC_HWRAM_LED_PIN(N));
	printk("get_led_control_value: CTRL_LOW: %02X(%d). \n", EC_HWRAM_LED_CTRL_LOBYTE(N), EC_HWRAM_LED_CTRL_LOBYTE(N));
	printk("get_led_control_value: CTRL_HIGH: %02X(%d). \n", EC_HWRAM_LED_CTRL_HIBYTE(N), EC_HWRAM_LED_CTRL_HIBYTE(N));
	printk("get_led_control_value: DEVICE_ID: %02X(%d). \n", EC_HWRAM_LED_DEVICE_ID(N), EC_HWRAM_LED_DEVICE_ID(N));
#endif

	// Step 0. Write HW pin number to LED HW pin address
	if(write_hw_ram(EC_HWRAM_LED_PIN(N), hw_pin_number)) {
		printk(KERN_WARNING "Write hw pin error. \n");
		return false;
	}

	// Step 1. Read value from LED control low byte (offset 0x02)
	if(read_hw_ram(EC_HWRAM_LED_CTRL_LOBYTE(N), &value_low)) {
		printk(KERN_ERR "Read LED ctrl low byte error. \n");
		return false;
	}
	
	// Step 2. Read value from LED control high byte (offset 0x01)
	if(read_hw_ram(EC_HWRAM_LED_CTRL_HIBYTE(N), &value_high)) {
		printk(KERN_WARNING "Read LED ctrl high byte error. \n");
		return false;
	}

	// Step 3. Read value from LED device id (offset 0x03)
	if(read_hw_ram(EC_HWRAM_LED_DEVICE_ID(N), &current_device_id)) {
		printk(KERN_WARNING "Read LED device id error. \n");
		return false;
	}

	ret_value = (value_high << 8) | value_low;
	*pvalue = ret_value;
	
#ifdef DEBUG_MESSAGE
	printk("get_led_control_value: LOW: %02X(%d), HIGH: %02X(%d). \n", value_low, value_low, value_high, value_high);
	printk("get_led_control_value: ret_value: %04X(%d). \n", ret_value, ret_value);
	printk("get_led_control_value: device_id: %04X(%d). \n", current_device_id, current_device_id);
	printk("get_led_control_value: *pvalue: %04X(%d). \n", *pvalue, *pvalue);
#endif

	return true;
}

int is_led_controllable(uint device_id)
{
	uint control_value = 0;

	if(device_id < (uint)EC_DID_LED_RUN || device_id > (uint)EC_DID_LED_D107_G) {
#ifdef DEBUG_MESSAGE
		printk("device_id: %d. \n", device_id);
		printk("EC_DID_LED_RUN: %d. \n", EC_DID_LED_RUN);
		printk("EC_DID_LED_D107_G: %d. \n", EC_DID_LED_D107_G);
#endif
		printk("is_led_controllable: The device_id is out of range ( 0xE1 ~ 0xE6 ). \n");
		return false;
	}

	if(!get_led_control_value(device_id, &control_value)) {
		printk(KERN_ERR "is_led_controllable: Get led control value error. \n");
		return false;
	}

#ifdef DEBUG_MESSAGE
	printk("control value of [%02X]: 0x%02X. \n", device_id, control_value);
	printk("init control: %d. \n", ((control_value >> 5) & 0x01));
#endif

#if 0
	/* jixu modify to fixed twice control led error */
	if(LED_CTRL_INTCTL_EXTERNAL != ((control_value >> 5) & 0x01)) {
		control_value &= ~(LED_CTRL_INTCTL_INTERNAL << 5);
		printk(KERN_ERR "LED_CTRL_INTCTL BIT is INTERNAL, change to EXTERNAL! \n");
#ifdef DEBUG_MESSAGE
		printk("control value of [%02X]: 0x%02X. \n", device_id, control_value);
		printk("init control: %d. \n", ((control_value >> 5) & 0x01));
#endif
	}
	return true;
#else
	if(LED_CTRL_INTCTL_EXTERNAL == ((control_value >> 5) & 0x01))
		return true;
	else
		return false;
#endif
}

//
//   | BIT F ~ BIT 6 (10Bit) | BIT 5  | BIT 4  | BIT 3 ~ BIT 0 |
//   |         LedBit        | IntCtl | Enable |   Polarity    |
//
//   Polarity - 0: low active; 1: high active
//   Enable   - 0: disable control; 1: enable led control
//   IntCtl   - Internal control. 1: Control by EC, external control is not allow.
//   LedBit   - Led Flash bit. Total 10 Bit. Every 100ms, EC get a bit one by one.
//             If the bit is 1, led light. Otherwise led dark.
//
int set_led_flash_rate(uint device_id, uint led_bit)
{
	uint old_value = 0;

	if(led_bit > LED_CTRL_LEDBIT_ON) {
		printk(KERN_ERR "set_led_flash_rate: The flash_rate must be smaller then 0x03FF. \n");
		return false;
	}
	
	if(!is_led_controllable(device_id)) {
		printk(KERN_ERR "set_led_flash_rate: The flash_rate IntCtl bit must be 1. \n");
		return false;
	}

	if(!get_led_control_value(device_id, &old_value)) {
		printk(KERN_ERR "set_led_flash_rate: parameter flash_type or flash_rate set error. \n");
		return false;
	}

#ifdef DEBUG_MESSAGE
	printk("set_led_flash_rate: old_value: %04X(%d). \n", old_value, old_value);
	printk("set_led_flash_rate: led_bit: %03X(%d). \n", led_bit, led_bit);
	printk("set_led_flash_rate: led_bit << 6: %03X(%d). \n", (led_bit << 6), (led_bit << 6));
#endif
	
	old_value &= ~LED_CTRL_LEDBIT_MASK;		// Clear led_bit
	old_value &= ~LED_CTRL_POLARITY_MASK;	// Clear polarity
	old_value |= (led_bit << 6);			// Apply led_bit
	old_value |= LED_CTRL_ENABLE_BIT;		// Enable LED Control

#ifdef DEBUG_MESSAGE
	printk("set_led_flash_rate: old_value: %04X. \n", old_value);
#endif

	return set_led_control_value(device_id, old_value);
}

int set_led_flash_type(uint device_id, uint flash_type, uint flash_rate)
{
	uint led_bit = 0;

#ifdef DEBUG_MESSAGE
	printk("\n========================================\n");
	printk("set_led_flash_type: device_id: %02X(%d). \n", device_id, device_id);
	printk("set_led_flash_type: flast_type: %02X(%d). \n", flash_type, flash_type);
	printk("set_led_flash_type: flash_rate: %02X(%d). \n", flash_rate, flash_rate);
#endif

	// 10 bit value from 0 ~ 0x3FF
	if((flash_type == LED_MANUAL) && (flash_rate > LED_CTRL_LEDBIT_ON)) {
		printk(KERN_ERR "set_led_flash_type: parameter flash_type or flash_rate set error. \n");
		return false;
	}

	switch(flash_type)
	{
	case LED_DISABLE:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_DISABLE. \n");
#endif
		led_bit = LED_CTRL_LEDBIT_DISABLE;
		break;
	case LED_ON:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_ON. \n");
#endif
		led_bit = LED_CTRL_LEDBIT_ON;
		break;
	case LED_FAST:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_FAST. \n");
#endif
		led_bit = LED_CTRL_LEDBIT_FAST;
		break;
	case LED_NORMAL:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_NORMAL. \n");
#endif
		led_bit = LED_CTRL_LEDBIT_NORMAL;
		break;
	case LED_SLOW:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_SLOW. \n");
#endif
		led_bit = LED_CTRL_LEDBIT_SLOW;
		break;
	case LED_MANUAL:
#ifdef DEBUG_MESSAGE
		printk("set_led_flash_type: flash_type is LED_MANUAL. \n");
#endif
		led_bit = flash_rate;
		break;
	default:
		printk("set_led_flash_type: Unknow flash_type. \n");
		return false;
	}

	return set_led_flash_rate(device_id, led_bit);
}


/* ******************************
 * Kernel Module Framwork Code
 * ******************************/

static int ioctl_set_led(unsigned long pmsg)
{
//	char *p = NULL;
//	p = (const char *)pmsg;
//	if(NULL == p) {
//		printk(KERN_ERR "ioctl_set_led: NULL pointer. \n");
//	}
//
	memset(&led_command, 0, sizeof(_led_command));
	if(copy_from_user(&led_command, (struct _led_command *)pmsg, sizeof(_led_command))) {
		printk(KERN_ERR "ioctl_set_led: copy_from_user error. \n");
		return false;
	}

	if(!set_led_flash_type(led_command.device_id, led_command.flash_type, led_command.flash_rate)) {
		printk(KERN_ERR "adv_led_write: set_led_flash_type error. \n");
		return false;
	}

	return true;
}
EXPORT_SYMBOL(ioctl_set_led);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int adv_led_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long adv_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int ret = -1;
	mutex_lock(&lock_ioctl);

	switch(cmd) 
	{
	case IOCTL_EC_HWRAM_GET_VALUE:
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_HWRAM_SET_VALUE:
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_ONEKEY_GET_STATUS:
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_ONEKEY_SET_STATUS:
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_LED_USER_DEFINE:
#ifdef DEBUG_MESSAGE
		printk("EC LED IOCTL. \n");
#endif
		if(ioctl_set_led(arg))
			ret = 0;
		break;
	case IOCTL_EC_LED_CONTROL_ON:
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_LED_CONTROL_OFF: 
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_LED_BLINK_ON: 
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_LED_BLINK_OFF: 
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	case IOCTL_EC_LED_BLINK_SPACE: 
#ifdef DEBUG_MESSAGE
		printk("Not support Currently. \n");
#endif
		break;
	default:
		ret = -1;
		break;
	}

	mutex_unlock(&lock_ioctl);
	return ret;
}

static int adv_led_open (struct inode *inode, struct file *file )
{
    return 0;
}

static int adv_led_release (struct inode *inode, struct file *file )
{
    return 0;
}

static ssize_t adv_led_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	memset(&led_command, 0, sizeof(_led_command));
	if(copy_from_user(&led_command, (struct _led_command *)buf, count)) {
		printk(KERN_ERR "adv_led_write: copy_from_user error. \n");
		return -1;
	}

	if(!set_led_flash_type(led_command.device_id, led_command.flash_type, led_command.flash_rate)) {
		printk(KERN_ERR "adv_led_write: set_led_flash_type error. \n");
		return -1;
	}

	return count;
}

static struct file_operations adv_led_fops = {
	.write = adv_led_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = adv_led_ioctl,
#else
	.unlocked_ioctl = adv_led_ioctl,
#endif
	.open = adv_led_open,
    .release = adv_led_release,
	.owner = THIS_MODULE
};

static void adv_led_cleanup (void)
{
#ifdef USE_CLASS
    device_destroy(led_class, MKDEV(major_led, 0));
    class_destroy(led_class);
#endif
    unregister_chrdev( major_led, "adv_led" );
    printk("Advantech EC led exit!\n");
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

static int adv_led_init (void)
{
    char *product;
    
	product = BIOS_Product_Name;
	if(adspname_detect(BIOS_Product_Name,"APAX-5580-433AE")) {
		printk(KERN_INFO "%s is not support EC led!\n", BIOS_Product_Name);
		return -ENODEV;
    }
   
	mutex_init(&lock_ioctl);

    if((major_led = register_chrdev(0, "adv_led", &adv_led_fops)) < 0) {
        printk("register led chrdev failed!\n");
        return -ENODEV;
    }

#ifdef USE_CLASS
    led_class = class_create(THIS_MODULE, "adv_led");
    if(IS_ERR(led_class))
    {
        printk(KERN_ERR "Error creating led class.\n");
        return -1;
    }
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
    device_create(led_class, NULL, MKDEV(major_led, 0), NULL, "adv_led");
	#else
    class_device_create(led_class, NULL, MKDEV(major_led, 0), NULL, "adv_led");
	#endif
#endif
    
	adv_get_led();
    
	printk("=====================================================\n");
    printk("     Advantech ec led driver V%s [%s]\n", 
            ADVANTECH_EC_LED_VER, ADVANTECH_EC_LED_DATE);
    printk("=====================================================\n");
    
	return 0;
}

module_init( adv_led_init );
module_exit( adv_led_cleanup );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JiXu");
MODULE_DESCRIPTION("Advantech EC LED Driver.");
