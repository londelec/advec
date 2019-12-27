/*****************************************************************************
                  Copyright (c) 2018, Advantech Automation Corp.
      THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
               INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

    ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
               ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        adv-ec.c
 * Version:     1.00  <10/10/2014>
 * Author:      Sun.Lang
 *
 * Description: The adv-ec is multifunctional driver for controlling EC chip.
 *				
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <10/10/2014> Sun.Lang
 *              - Initial version
 *              Version 1.01 <11/05/2015> Jiangwei.Zhu
 *              - Modify read_ad_value() function. 
 *              - Add smbus_read_byte() function.
 *              - Add smbus_write_byte() function. 
 *              - Add wait_smbus_protocol_finish() function.
 *              Version 1.02 <03/04/2016> Jiangwei.Zhu
 *              - Add smbus_read_word() function.
 *              Version 1.03 <01/22/2017> Ji.Xu
 *              - Add detect to Advantech porduct name "ECU".
 *              Version 1.04 <09/20/2017> Ji.Xu
 *              - Update to support detect Advantech product name in UEFI 
 *                BIOS(DMI).
 *              Version 1.05 <11/02/2017> Ji.Xu
 *              - Fixed issue: Cache coherency error when exec 'ioremap_uncache()' 
 *                in kernel-4.10.
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
#include <asm/msr.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include "ec.h"

#define ADVANTECH_EC_MFD_VER     "1.05"
#define ADVANTECH_EC_MFD_DATE    "11/02/2017"

struct mutex lock;
struct Dynamic_Tab *PDynamic_Tab;
char *BIOS_Product_Name;

//Wait IBF (Input buffer full) clear
//static int wait_ibf(void)
int wait_ibf(void)
{
    int i;
    for(i = 0; i < EC_MAX_TIMEOUT_COUNT; i++)
    {
        if((inb(EC_COMMAND_PORT) & 0x02) == 0)
        {
            return 0;
        }
        udelay(EC_UDELAY_TIME);
    }
    return -ETIMEDOUT;
}
EXPORT_SYMBOL(wait_ibf);

//Wait OBF (Output buffer full) set
//static int wait_obf(void)
int wait_obf(void)
{
    int i;
    for(i = 0; i < EC_MAX_TIMEOUT_COUNT; i++)
    {
        if((inb(EC_COMMAND_PORT) & 0x01) != 0)
        {
            return 0;
        }
        udelay(EC_UDELAY_TIME);
    }
    return -ETIMEDOUT;
}
EXPORT_SYMBOL(wait_obf);

//static int wait_smbus_protocol_finish(void)
int wait_smbus_protocol_finish(void)
{
    uchar addr;
    uchar data;
    int retry = 1000;
    do {
        addr = EC_SMBUS_PROTOCOL;
        data = 0xFF;
        if(!(read_hw_ram(addr, &data)))
        {
            return 0;
        }
        
        if(data == 0)
        {
            return 0;
        }
        udelay(EC_UDELAY_TIME);
    } while ( retry-- > 0 );

    return -ETIMEDOUT;    
}
EXPORT_SYMBOL(wait_smbus_protocol_finish);

// Get dynamic control table
int adv_get_dynamic_tab(struct Dynamic_Tab *PDynamic_Tab)
{
    int i;
    int ret = -1;
    uchar pin_tmp;
    uchar device_id;

    mutex_lock(&lock);

    for(i=0;i<EC_MAX_TBL_NUM;i++)
    {
        PDynamic_Tab[i].DeviceID = 0xff;
        PDynamic_Tab[i].HWPinNumber = 0xff;
    }

    for(i=0;i<EC_MAX_TBL_NUM;i++)
    {
        // Step 0. Wait IBF clear
        if((ret = wait_ibf()))
            goto error;

        // Step 1. Write 0x20 to 0x29A
        // Send write item number into index command
        outb(EC_TBL_WRITE_ITEM, EC_COMMAND_PORT);

        // Step 2. Wait IBF clear
        if((ret = wait_ibf()))
            goto error;

        // Step 3. Write item number to 0x299
        // Write item number to index. Item number is limited in range 0 to 31
        outb(i,EC_STATUS_PORT);

        // Step 4. Wait OBF set
        if((ret = wait_obf()))
            goto error;

        // Step 5. Read 0x299 port
        // If item is defined, EC will return item number.
        // If table item is not defined, EC will return 0xFF.
        if(0xff == inb(EC_STATUS_PORT))
        {
            mutex_unlock(&lock);
            return -EINVAL;
        }

        // Step 6. Wait IBF clear
        if((ret = wait_ibf()))
            goto error;

        // Step 7. Write 0x21 to 0x29A
        // Send read HW pin number command
        outb(EC_TBL_GET_PIN,EC_COMMAND_PORT);

        // Step 8. Wait OBF set
        if((ret = wait_obf()))
            goto error;

        // Step 9. Read 0x299 port
        // EC will return current item HW pin number
        pin_tmp = inb(EC_STATUS_PORT) & 0xff;

        // Step 10. Wait IBF clear
        if((ret = wait_ibf()))
            goto error;

        if(0xff == pin_tmp)
        {
            mutex_unlock(&lock);
            return -EINVAL;
        }

        // Step 11. Write 0x22 to 0x29A
        // Send read device id command
        outb(EC_TBL_GET_DEVID,EC_COMMAND_PORT);


        // Step 12. Wait OBF set
        if((ret = wait_obf()))
            goto error;

        // Step 13. Read 0x299 port
        // EC will return current item Device ID
        device_id = inb(EC_STATUS_PORT) & 0xff;

        // Step 14. Save data to a database
        PDynamic_Tab[i].DeviceID = device_id;
        PDynamic_Tab[i].HWPinNumber = pin_tmp;

        //printk(KERN_DEBUG "%s: PDynamic_Tab[%d].DeviceID = 0x%02X, PDynamic_Tab[%d].HWPinNumber = 0x%02X, line: %d\n", __func__, i, PDynamic_Tab[i].DeviceID, i, PDynamic_Tab[i].HWPinNumber, __LINE__);
    }
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL(PDynamic_Tab);

int read_ad_value(uchar hwpin,uchar multi)
{
    int ret = -1;

    u32 ret_val = 0;
    uchar LSB = 0;
    uchar MSB = 0;

    mutex_lock(&lock);
    if((ret = wait_ibf()))
        goto error;

    outb(EC_AD_INDEX_WRITE,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(hwpin,EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    if(0xff == ((inb(EC_STATUS_PORT))))
    {
        mutex_unlock(&lock);
        return -1;
    }

    if((ret = wait_ibf()))
        goto error;

    outb(EC_AD_LSB_READ,EC_COMMAND_PORT);

    if((ret = wait_obf()))
        goto error;

    LSB = inb(EC_STATUS_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_AD_MSB_READ,EC_COMMAND_PORT);

    if((ret = wait_obf()))
        goto error;

    MSB = inb(EC_STATUS_PORT);
    ret_val = ((MSB << 8) | LSB) & 0x03FF;
    
    ret_val = ret_val * multi * 100;
   
    mutex_unlock(&lock);
    return ret_val;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;

}
EXPORT_SYMBOL_GPL(read_ad_value);

int read_acpi_value(uchar addr,uchar *pvalue)
{
    int ret = -1;
    uchar value;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_ACPI_RAM_READ,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(addr,EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    value = inb(EC_STATUS_PORT);
    *pvalue = value;
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(read_acpi_value);

int write_acpi_value(uchar addr,uchar value)
{
    int ret = -1;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_ACPI_DATA_WRITE, EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(addr, EC_STATUS_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(value,EC_STATUS_PORT);

    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF  too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_acpi_value);

int read_gpio_status(uchar PinNumber,uchar *pvalue)
{
    int ret = -1;

    uchar gpio_status_value;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_INDEX_WRITE, EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(PinNumber, EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    if(0xff == inb(EC_STATUS_PORT))
    {
        printk("Read Pin Number error!!");
        mutex_unlock(&lock);
        return -1;
    }

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_STATUS_READ,EC_COMMAND_PORT);

    if((ret = wait_obf()))
        goto error;

    gpio_status_value = inb(EC_STATUS_PORT);
    *pvalue = gpio_status_value;
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(read_gpio_status);

int write_gpio_status(uchar PinNumber,uchar value)
{
    int ret = -1;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_INDEX_WRITE, EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(PinNumber, EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    if(0xff == inb(EC_STATUS_PORT))
    {
        printk("Read Pin Number error!!");
        mutex_unlock(&lock);
        return -1;
    }

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_STATUS_WRITE,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(value,EC_STATUS_PORT);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_gpio_status);

int read_gpio_dir(uchar PinNumber,uchar *pvalue)
{
    int ret = -1;
    uchar gpio_dir_value;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_INDEX_WRITE,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(PinNumber,EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    if(0xff == inb(EC_STATUS_PORT))
    {
        printk("Read Pin Number error!!");
        mutex_unlock(&lock);
        return -1;
    }

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_DIR_READ,EC_COMMAND_PORT);

    if((ret = wait_obf()))
        goto error;

    gpio_dir_value = inb(EC_STATUS_PORT);
    *pvalue = gpio_dir_value;
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(read_gpio_dir);

int write_gpio_dir(uchar PinNumber,uchar value)
{
    int ret = -1;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_INDEX_WRITE, EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(PinNumber, EC_STATUS_PORT);

    if((ret = wait_obf()))
        goto error;

    if(0xff == inb(EC_STATUS_PORT))
    {
        printk("Read Pin Number error!!");
        mutex_unlock(&lock);
        return -1;
    }

    if((ret = wait_ibf()))
        goto error;

    outb(EC_GPIO_DIR_WRITE,EC_COMMAND_PORT);

    if((ret = wait_ibf()))
        goto error;

    outb(value,EC_STATUS_PORT);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_gpio_dir);

// Read data from EC HW ram
int read_hw_ram(uchar addr, uchar *data)
{
    int ret = -1;
    mutex_lock(&lock);

    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;
	
    // Step 1. Send "read EC HW ram" command to EC Command port(Write 0x88 to 0x29A)
    outb(EC_HW_RAM_READ, EC_COMMAND_PORT);

    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 4. Wait OBF set
    if((ret = wait_obf()))
        goto error;
	
    // Step 5. Get "EC HW ram" data from EC Data port (Read 0x299 port)
    *data= inb(EC_STATUS_PORT);

    mutex_unlock(&lock);

    //printk(KERN_DEBUG "%s: addr= 0x%02X, data= 0x%X, line: %d\n", __func__ , addr, *data, __LINE__);
    return ret;

error:
    mutex_unlock(&lock);
    printk(KERN_DEBUG "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(read_hw_ram);

// Write data to EC HW ram
int write_hw_ram(uchar addr,uchar data)
{
    int ret = -1;

    mutex_lock(&lock);
    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "write EC HW ram" command to EC command port(Write 0x89 to 0x29A)
    outb(EC_HW_RAM_WRITE, EC_COMMAND_PORT);
    
    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 5. Send "EC HW ram" data to EC Data port (Write data to 0x299 port)
    outb(data, EC_STATUS_PORT);

    mutex_unlock(&lock);

    //printk(KERN_DEBUG "%s: addr= 0x%02X, data= 0x%X, line: %d\n", __func__ , addr, data, __LINE__);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_DEBUG "%s: Wait for IBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_hw_ram);

// Write data to EC HW ram
int write_hw_extend_ram(uchar addr,uchar data)
{
    int ret = -1;

    mutex_lock(&lock);
    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "write EC HW ram" command to EC command port(Write 0x89 to 0x29A)
    outb(EC_HW_EXTEND_RAM_WRITE, EC_COMMAND_PORT);
    
    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "EC HW ram" address to EC Data port(Write ram address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 5. Send "EC HW ram" data to EC Data port (Write data to 0x299 port)
    outb(data, EC_STATUS_PORT);

    mutex_unlock(&lock);

    //printk(KERN_DEBUG "%s: addr= 0x%02X, data= 0x%X, line: %d\n", __func__ , addr, data, __LINE__);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_DEBUG "%s: Wait for IBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_hw_extend_ram);

int write_hwram_command(uchar data)
{
    int ret = -1;

    mutex_lock(&lock);

    if((ret = wait_ibf()))
        goto error;

    outb(data, EC_COMMAND_PORT);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_DEBUG "%s: Wait for IBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_hwram_command);

int smbus_read_word(struct EC_SMBUS_WORD_DATA *ptr_ec_smbus_word_data)
{

    int ret = -1;
    
    uchar sm_ready;
    uchar LSB = 0;
    uchar MSB = 0;
    unsigned short Value = 0;
    uchar addr;
    uchar data;
    
    // 
    // Step 1. Select SMBus channel
    // 
    addr = EC_SMBUS_CHANNEL;
    data = ptr_ec_smbus_word_data->Channel;
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Select SMBus channel Failed\n");
        goto error;
    }

    //
    // Step 2. Set SMBUS device address EX: 0x98
    // 
    addr = EC_SMBUS_SLV_ADDR;
    data = (ptr_ec_smbus_word_data->Address);
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Select SMBus device address (0x%02X) Failed\n\n", ptr_ec_smbus_word_data->Address);
        goto error;
    }

    // 
    // Step 3. Set Chip (EX: INA266) Register Address 
    // 
    addr = EC_SMBUS_CMD;
    data = ptr_ec_smbus_word_data->Register;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Select Chip Register Address(0x%02X) Failed\n\n", ptr_ec_smbus_word_data->Register);
        goto error;
    }
    
    // Step 4. Set EC SMBUS read word Mode 
    // 
    addr = EC_SMBUS_PROTOCOL;
    data = SMBUS_WORD_READ;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Set EC SMBUS read Byte Mode Failed\n\n");
        goto error;
    }
    
    // 
    // Step 5. Check EC Smbus states
    // 
    if((ret = wait_smbus_protocol_finish()))
    {
        printk(KERN_ERR "Wait SmBus Protocol Finish Failed!!\n\n");
        goto error;    
    }
    
    addr = EC_SMBUS_STATUS;
    if((ret = read_hw_ram(addr, &data)))
    {
        printk(KERN_ERR "Check EC Smbus states Failed\n\n");
        goto error;
    } 
    sm_ready = data;

    // check no error
    if(sm_ready != 0x80)
    {
        printk(KERN_ERR "SMBUS ERR:(0x%02X)\n\n", sm_ready);
        goto error;
    }
    
    //
    // Step 6. Get Value
    //
    addr = EC_SMBUS_DAT_OFFSET(0);
    if((ret = read_hw_ram(addr, &data)))
    {
        printk(KERN_ERR "Get Value Failed\n");
        goto error;
    }
    MSB = data;

    addr = EC_SMBUS_DAT_OFFSET(1);
    if((ret = read_hw_ram(addr, &data)))
    {
        printk(KERN_ERR "Get Value Failed\n");
        goto error;
    }
    LSB = data;

    Value = (MSB << 8) | LSB;
    ptr_ec_smbus_word_data->Value = Value;
    
    return 0;

error:
    printk("-------smbus_read_word Exception!-------\n");
    return ret;
}
EXPORT_SYMBOL_GPL(smbus_read_word);

int smbus_read_byte(struct EC_SMBUS_READ_BYTE *ptr_ec_smbus_read_byte)
{
    int ret = -1;
    uchar sm_ready;
    uchar addr;
    uchar data;

    //CHECK_PARAMETER
    if(ptr_ec_smbus_read_byte == NULL){
        return EINVAL;
    }

    // 
    // Step 1. Select SMBus channel
    // 
    addr = EC_SMBUS_CHANNEL;
    data = ptr_ec_smbus_read_byte->Channel;
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Select SMBus channel Failed\n");
        goto error;
    }

    //
    // Step 2. Set SMBUS device address EX: 0x98
    // 
    addr = EC_SMBUS_SLV_ADDR;
    data = (ptr_ec_smbus_read_byte->Address);
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Select SMBus device address (0x%02X) Failed\n\n", ptr_ec_smbus_read_byte->Address);
        goto error;
    }

    // 
    // Step 3. Set Chip (EX: MCP23008) Register Address 
    // 
    addr = EC_SMBUS_CMD;
    data = ptr_ec_smbus_read_byte->Register;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Select Chip Register Address(0x%02X) Failed\n\n", ptr_ec_smbus_read_byte->Register);
        goto error;
    }
    
    // 
    // Step 4. Set EC SMBUS read Byte Mode 
    // 
    addr = EC_SMBUS_PROTOCOL;
    data = SMBUS_BYTE_READ;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Set EC SMBUS read Byte Mode Failed\n\n");
        goto error;
    }
        
    // 
    // Step 5. Check EC Smbus states
    // 
    if((ret = wait_smbus_protocol_finish()))
    {
        printk(KERN_ERR "Wait SmBus Protocol Finish Failed!!\n\n");
        goto error;    
    }
    
    addr = EC_SMBUS_STATUS; 
    if((ret = read_hw_ram(addr, &data)))
    {
        printk(KERN_ERR "Check EC Smbus states Failed\n\n");
        goto error;
    } 
    sm_ready = data;

    // check no error
    if(sm_ready != 0x80)
    {
        printk(KERN_ERR "SMBUS ERR:(0x%02X)\n\n", sm_ready);
        goto error;
    }

    //
    // Step 6. Get Value
    //
    addr = EC_SMBUS_DATA;
    if((ret = read_hw_ram(addr, &data)))
    {
        printk(KERN_ERR "Get Value Failed\n");
        goto error;
    }
    
    ptr_ec_smbus_read_byte->Data = (data & 0xFF);
    //printk(KERN_DEBUG "-------adv_ec_smbus_read_byte Value = (0x%X) -------\n", ptr_ec_smbus_read_byte->Data);
    return 0;

error:
    printk(KERN_WARNING "-------smbus_read_byte Exception!-------\n");
    return ret;
}
EXPORT_SYMBOL_GPL(smbus_read_byte);

int smbus_write_byte(struct EC_SMBUS_WRITE_BYTE *ptr_ec_smbus_write_byte)
{
    int ret = -1;
    uchar sm_ready;
    uchar addr;
    uchar data;
    
    // 
    // Step 1. Select SMBus channel
    // 
    addr = EC_SMBUS_CHANNEL;
    data = ptr_ec_smbus_write_byte->Channel;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Select SMBus channel Failed\n");
        goto error;
    }

    //
    // Step 2. Set SMBUS device address EX: 0x98
    // 
    addr = EC_SMBUS_SLV_ADDR;
    data = (ptr_ec_smbus_write_byte->Address);
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Select SMBus device address (0x%02X) Failed\n", ptr_ec_smbus_write_byte->Address);
        goto error;
    }

    // 
    // Step 3. Set Chip (EX: MCP23008) Register Address 
    // 
    addr = EC_SMBUS_CMD;
    data = ptr_ec_smbus_write_byte->Register;
    if((ret = write_hw_ram(addr, data)))
    {
        printk(KERN_ERR "Select Chip Register Address(0x%02X) Failed\n", ptr_ec_smbus_write_byte->Register);
        goto error;
    }

    
    // 
    // Step 4. Set Data to SMBUS
    // 
    addr = EC_SMBUS_DATA;
    data = ptr_ec_smbus_write_byte->Data;
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Set Data(0x%02X) to SMBUS Failed\n", ptr_ec_smbus_write_byte->Data);
        goto error;
    }
  
    // 
    // Step 5. Set EC SMBUS write Byte Mode 
    // 
    addr = EC_SMBUS_PROTOCOL;
    data = SMBUS_BYTE_WRITE;
    if((ret = write_hw_ram(addr,data)))
    {
        printk(KERN_ERR "Set EC SMBUS write Byte Mode Failed\n");
        goto error;
    }
    // 
    // Step 6. Check EC Smbus states
    //  
    if((ret = wait_smbus_protocol_finish()))
    {
        printk(KERN_ERR "Wait SmBus Protocol Finish Failed!!\n\n");
        goto error;    
    }
    
    addr = EC_SMBUS_STATUS;    
    if((ret = read_hw_ram(addr,&data)))
    {
        printk(KERN_ERR "Check EC Smbus states Failed\n\n");
        goto error;
    }
    sm_ready = data;

    // check no error
    if(sm_ready != 0x80)
    {
        printk(KERN_ERR "SMBUS ERR:(0x%02X)\n", sm_ready);
        goto error;
    }
    mutex_unlock(&lock);
    return 0;

error:
    printk(KERN_ERR "-------smbus_write_byte Exception!-------\n");
    mutex_unlock(&lock);
    return ret;
}
EXPORT_SYMBOL_GPL(smbus_write_byte);

// Get One Key Recovery status
int read_onekey_status(uchar addr, uchar *pdata)
{
    int ret = -1;

    mutex_lock(&lock);

    // Init return value
    *pdata = 0;

    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "One Key Recovery" command to EC Command port(Write 0x9C to 0x29A)
    outb(EC_ONE_KEY_FLAG, EC_COMMAND_PORT);

    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "One Key Recovery function" address to EC Data port(Write function address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 4. Wait OBF set
    if((ret = wait_obf()))
        goto error;

    // Step 5. Get "One Key Recovery function" data from EC Data port (Read 0x299 port)
    *pdata = inb(EC_STATUS_PORT);

    printk(KERN_DEBUG "%s: data= %d, line: %d\n", __func__ , *pdata, __LINE__);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(read_onekey_status);

// Set One Key Recovery status
int write_onekey_status(uchar addr)
{
    int ret = -1;

    mutex_lock(&lock);

    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "One Key Recovery" command to EC Command port(Write 0x9C to 0x29A)
    outb(EC_ONE_KEY_FLAG, EC_COMMAND_PORT);

    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "One Key Recovery function" address to EC Data port(Write function address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    mutex_unlock(&lock);

    printk(KERN_DEBUG "%s: addr= %d, line: %d\n", __func__ , addr, __LINE__);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(write_onekey_status);

// EC OEM get status
int ec_oem_get_status(uchar addr, uchar *pdata)
{
    int ret = -1;

    mutex_lock(&lock);

    // Init return value
    *pdata = 0;

    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "ASG OEM" command to EC Command port(Write 0xEA to 0x29A)
    outb(EC_ASG_OEM, EC_COMMAND_PORT);

    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "ASG OEM STATUS READ" address to EC Data port(Write 0x00 to 0x299 port)
    outb(EC_ASG_OEM_READ, EC_STATUS_PORT);

    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 5. Send "OEM STATUS" address to EC Data port(Write function address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 6. Wait OBF set
    if((ret = wait_obf()))
        goto error;

    // Step 7. Get "OEM STATUS" data from EC Data port (Read 0x299 port)
    *pdata = inb(EC_STATUS_PORT);

    printk(KERN_DEBUG "%s: data= %d, line: %d\n", __func__ , *pdata, __LINE__);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(ec_oem_get_status);

// EC OEM set status
int ec_oem_set_status(uchar addr, uchar pdata)
{
    int ret = -1;

    mutex_lock(&lock);

    // Step 0. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 1. Send "ASG OEM" command to EC Command port(Write 0xEA to 0x29A)
    outb(EC_ASG_OEM, EC_COMMAND_PORT);

    // Step 2. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 3. Send "ASG OEM STATUS WRITE" address to EC Data port(Write 0x01 to 0x299 port)
    outb(EC_ASG_OEM_WRITE, EC_STATUS_PORT);

    // Step 4. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 5. Send "OEM STATUS" address to EC Data port(Write function address to 0x299 port)
    outb(addr, EC_STATUS_PORT);

    // Step 6. Wait IBF clear
    if((ret = wait_ibf()))
        goto error;

    // Step 7. Send "OEM STATUS" status to EC Data port (Write status to 0x299 port)
    outb(pdata, EC_STATUS_PORT);

    printk(KERN_DEBUG "%s: data= %d, line: %d\n", __func__ , pdata, __LINE__);
    mutex_unlock(&lock);
    return 0;

error:
    mutex_unlock(&lock);
    printk(KERN_WARNING "%s: Wait for IBF or OBF too long. line: %d\n", __func__ , __LINE__);
    return ret;
}
EXPORT_SYMBOL_GPL(ec_oem_set_status);

int get_productname(char *product)
{
	adv_bios_info adspname_info;
    static unsigned char *uc_ptaddr;
	static unsigned char *uc_epsaddr;
    int index = 0;
    int i = 0;
	int length = 0;
	int type0_str = 0;
	int type1_str = 0;
	int is_advantech = 0;

	if (!(uc_ptaddr = ioremap_nocache(AMI_UEFI_ADVANTECH_BOARD_NAME_ADDRESS , AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH))) {
		printk(KERN_ERR "Error: ioremap_nocache() \n");
		return -ENXIO;
	}

	// Try to Read the product name from UEFI BIOS(DMI) EPS table
	for (index = 0; index < AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH; index++) {
		if (uc_ptaddr[index] == '_' 
					&& uc_ptaddr[index+0x1] == 'S' 
					&& uc_ptaddr[index+0x2] == 'M' 
					&& uc_ptaddr[index+0x3] == '_'
					&& uc_ptaddr[index+0x10] == '_'
					&& uc_ptaddr[index+0x11] == 'D'
					&& uc_ptaddr[index+0x12] == 'M'
					&& uc_ptaddr[index+0x13] == 'I'
					&& uc_ptaddr[index+0x14] == '_'
					) {
			//printk(KERN_INFO "UEFI BIOS \n");
			adspname_info.eps_table = 1;
			break;
		}
	}

	// If EPS table exist, read type1(system information)
	if (adspname_info.eps_table) {
		if (!(uc_epsaddr = (char *)ioremap_nocache(((unsigned int *)&uc_ptaddr[index+0x18])[0], 
						((unsigned short *)&uc_ptaddr[index+0x16])[0]))) {
			if (!(uc_epsaddr = (char *)ioremap_cache(((unsigned int *)&uc_ptaddr[index+0x18])[0], 
							((unsigned short *)&uc_ptaddr[index+0x16])[0]))) {
				printk(KERN_ERR "Error: both ioremap_nocache() and ioremap_cache() exec failed! \n");
				return -ENXIO;
			}
		}
		type0_str = (int)uc_epsaddr[1];
		for (i = type0_str; i < (type0_str+512); i++) {
			if (uc_epsaddr[i] == 0 && uc_epsaddr[i+1] == 0 && uc_epsaddr[i+2] == 1) {
				type1_str = i + uc_epsaddr[i+3];
				break;
			}
		}
		for (i = type1_str; i < (type1_str+512); i++) {
			if (uc_epsaddr[i] == 'A' && uc_epsaddr[i+1] == 'd' && uc_epsaddr[i+2] == 'v' 
					&& uc_epsaddr[i+3] == 'a' && uc_epsaddr[i+4] == 'n' && uc_epsaddr[i+5] == 't' 
					&& uc_epsaddr[i+6] == 'e' && uc_epsaddr[i+7] == 'c' &&uc_epsaddr[i+8] == 'h') {
				is_advantech = 1;
				//printk(KERN_INFO "match Advantech \n");
			}
			if (uc_epsaddr[i] == 0) {
				i++;
				type1_str = i;
				break;
			}
		}
		length = 2;
		while ((uc_epsaddr[type1_str + length] != 0)
				&& (length < AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH)) {
			length += 1;
		}
		memmove(product, &uc_epsaddr[type1_str], length);
		iounmap((void *)uc_epsaddr);
		if (is_advantech) {
			iounmap(( void* )uc_ptaddr);
			return 0;
		}
	} 

	// It is an old BIOS, read from 0x000F0000
    for (index = 0; index < (AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH - 3); index++) {
        if((uc_ptaddr[index]=='T' && uc_ptaddr[index+1]=='P' && uc_ptaddr[index+2]=='C')
                || (uc_ptaddr[index]=='U' && uc_ptaddr[index+1]=='N' && uc_ptaddr[index+2]=='O')
                || (uc_ptaddr[index]=='I' && uc_ptaddr[index+1]=='T' && uc_ptaddr[index+2]=='A')
                || (uc_ptaddr[index]=='M' && uc_ptaddr[index+1]=='I' && uc_ptaddr[index+2]=='O')
                || (uc_ptaddr[index]=='E' && uc_ptaddr[index+1]=='C' && uc_ptaddr[index+2]=='U')
                || (uc_ptaddr[index]=='A' && uc_ptaddr[index+1]=='P' && uc_ptaddr[index+2]=='A' && uc_ptaddr[index+3]=='X')) {
            break;
        }
    }

    if(index == (AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH - 3)) {
        printk(KERN_WARNING "%s: Can't find the product name or we don't support this product, line: %d\n", __func__, __LINE__);
        product[0] = '\0';
        iounmap((void *)uc_ptaddr);
        return -ENODATA;
    }

    // Use char "Space" (ASCII code: 32) to check the end of the Product Name.
    for(i=0; (uc_ptaddr[index+i] != 32) && (i < 31) ; i++)
    {
        product[i] = uc_ptaddr[index+i];
    }

    product[i] = '\0';
    printk(KERN_DEBUG "%s: BIOS Product Name = %s, line: %d\n", __func__ , product, __LINE__);

    iounmap((void *)uc_ptaddr);
    return 0;
}
EXPORT_SYMBOL_GPL(BIOS_Product_Name);

static int ec_init(void)
{
	int ret = -1;
    mutex_init(&lock);

    BIOS_Product_Name = kmalloc(AMI_ADVANTECH_BOARD_ID_LENGTH*sizeof(char), GFP_KERNEL);
    if(!BIOS_Product_Name)
	{
		ret = -EPERM;
		goto err0;
	}
	get_productname(BIOS_Product_Name);
//	memset(BIOS_Product_Name, 0, AMI_ADVANTECH_BOARD_ID_LENGTH*sizeof(char));
//	strncpy(BIOS_Product_Name, "UNO-2484G-673xAE\0", 17);

    PDynamic_Tab = kmalloc(EC_MAX_TBL_NUM*sizeof(struct Dynamic_Tab),GFP_KERNEL);
    if(!PDynamic_Tab)
    {
		ret = -EPERM;
		goto err1;
    }
    adv_get_dynamic_tab(PDynamic_Tab);

    printk("=====================================================\n");
    printk("     Advantech ec mfd driver V%s [%s]\n",
            ADVANTECH_EC_MFD_VER, ADVANTECH_EC_MFD_DATE);
    printk("=====================================================\n");
    return 0;

err1:
	kfree(BIOS_Product_Name);
err0:
	return ret;
}

static void ec_cleanup(void)
{
    kfree(PDynamic_Tab);
    kfree(BIOS_Product_Name);
    printk("Advantech EC mfd exit!\n");
}

module_init( ec_init );
module_exit( ec_cleanup );

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("sun.lang");
MODULE_DESCRIPTION("Advantech EC MFD Driver.");

