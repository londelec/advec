/*****************************************************************************
  Copyright (c) 2018, Advantech Automation Corp.
  THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY
  INFORMATION WHICH IS THE PROPERTY OF ADVANTECH AUTOMATION CORP.

  ANY DISCLOSURE, USE, OR REPRODUCTION, WITHOUT WRITTEN AUTHORIZATION FROM
  ADVANTECH AUTOMATION CORP., IS STRICTLY PROHIBITED.
 *****************************************************************************
 *
 * File:        ec_eeprom_drv.c
 * Version:     1.00  <06/28/2017>
 * Author:      Ji.Xu
 *
 * Description: The ec_eeprom_drv is driver for controlling EC eeprom.
 *              Following win EC driver.
 *
 * Status:      working
 *
 * Change Log:
 *              Version 1.00 <06/28/2017> Ji.Xu
 *              - Initial version
 *              Version 1.01 <03/20/2018> Ji.Xu
 *              - Support for compiling in kernel-4.10 and below.
 *
 -----------------------------------------------------------------------------*/
#include <linux/version.h>
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) KERNEL_VERSION((a)*65536+(b)*256+(c))
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
#include <linux/fs.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#else
#include <crypto/hash.h>
#endif

#include <asm/io.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/uaccess.h>
#endif

#include "../mfd-ec/ec.h"
#include "adv_eeprom_drv.h"

#define ADVANTECH_EC_EEPROM_VER           "1.01"
#define ADVANTECH_EC_EEPROM_DATE          "03/20/2018" 
//#define QATEST
//#define PASSWDCLEAN

// magic number refer to linux-source-4.4.0/Documentation/ioctl/ioctl-number.txt
#define EEPROM_MAGIC	'g'
#define IOCTL_COMMON_EC_I2C_GET_CONFIG				_IO(EEPROM_MAGIC, 0x30)
#define IOCTL_COMMON_EC_I2C_TRANSFER				_IOWR(EEPROM_MAGIC, 0x31, pec_i2c_data)
#define IOCTL_COMMON_EC_I2C_GET_FREQ				_IO(EEPROM_MAGIC, 0x32)
#define IOCTL_COMMON_EC_I2C_SET_FREQ				_IO(EEPROM_MAGIC, 0x33)
#define IOCTL_COMMON_EC_BARCODE_EEPROM_GET_CONFIG	_IOWR(EEPROM_MAGIC, 0x34, peeprom_config)
#define IOCTL_COMMON_EC_BARCODE_EEPROM_SET_PROTECT	_IOWR(EEPROM_MAGIC, 0x35, pec_barcode_eeprom_protect_data)

ssize_t major = 0;
static struct class *eeprom_class;
static char g_asg_key[14] = "Advantech_ASG\0";
const unsigned char F75111_SlavAddr_List[] = { 0x6E, 0x9C };
static unsigned char DeviceId2HWPinNum[EC_MAX_DEVICE_ID_NUM];
static bool bIsMailBoxSupported = false;

static int ec_command_port		= EC_COMMAND_PORT1;
static int ec_status_port		= EC_STATUS_PORT1;
static int ECMBOX_OFFSET_PORT	= EC_MBOX_OFFSET_PORT;
static int ECMBOX_DATA_PORT		= EC_MBOX_DATA_PORT;

static DEFINE_MUTEX(EC_SuperIOMutex);
static DEFINE_MUTEX(EC_SMBusMutex);

void delay_1s(void)
{
	int i;
	for (i = 0; i < 1000; i++) {
		udelay(1000);
	}
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
void sha1(unsigned char *hval, const unsigned char *data, unsigned int len)
{   
	struct scatterlist sg = {0}; 
	struct hash_desc desc = {0}; 
	unsigned char test_str[14] = "Advantech_ASG\0";

#ifdef QATEST
	int i;
	for (i = 0; i < len; i++) {
//		printk("g_asg_key[%d]=%c \n", i, data[i]);
		printk("sha1:test_str[%d]=%c \n", i, test_str[i]);
	}
#endif

//	sg_init_one(&sg, data, len);
	sg_init_one(&sg, test_str, len);
	desc.tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	crypto_hash_init(&desc);
	crypto_hash_update(&desc, &sg, len);
	crypto_hash_final(&desc, hval);
	crypto_free_hash(desc.tfm);

#ifdef QATEST
	for (i = 0; i < 32; i++) {
		printk("sha1:hash_key[%d]=0x%X \n", i, hval[i]);
	}
#endif
}
#else
void sha1(unsigned char *hval, const unsigned char *data, unsigned int len)
{   
	unsigned char test_str[14] = "Advantech_ASG\0";
	struct crypto_shash *sha1 = NULL;
	struct shash_desc *desc = NULL; 
	unsigned int size = 0;

#ifdef QATEST
	int i;
	for (i = 0; i < len; i++) {
//		printk("g_asg_key[%d]=%c \n", i, data[i]);
		printk("sha1:test_str[%d]=%c \n", i, test_str[i]);
	}
#endif

	sha1 = crypto_alloc_shash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(sha1)) {
		printk("crypto_alloc_shash: crypto_shash alloc error! \n");
		return;
	}

	size = sizeof(struct shash_desc) + crypto_shash_descsize(sha1);
	desc = kmalloc(size, GFP_KERNEL);
	if (!desc) {
		printk("kmalloc: shash_desc alloc error! \n");
		goto kmalloc_err;
	}

	desc->tfm = sha1;
	desc->flags = 0x0;
	if (crypto_shash_init(desc)) {
		printk("crypto_shash_init: init shash_desc error! \n");
		goto init_err;
	}

	crypto_shash_update(desc, test_str, len);
	crypto_shash_final(desc, hval);

#ifdef QATEST
	for (i = 0; i < 32; i++) {
		printk("sha1:hash_key[%d]=0x%X \n", i, hval[i]);
	}
#endif

init_err:
	kfree(desc);
kmalloc_err:
	crypto_free_shash(sha1);
	return;
}
#endif

bool ec_wait_mbox_cmd_clear(void)
{
    unsigned char uTemp = 0xff;
    int i = 0;
    for( i = 0; i < 5000; i++ )
    {
        if (ec_mbox_get_value(EC_MBOX_CMD_OFFSET, &uTemp) < 0)
            break;
        if ( uTemp == 0 )
            return true;
		udelay(1000);
    }
    
    printk(KERN_DEBUG "ec_wait_mbox_cmd_clear: Failed !! \n\n");
    return false;
}

bool ec_mbox_set_value(unsigned char uCmd, unsigned char uValue)
{
	outb(uCmd, ECMBOX_OFFSET_PORT);
	outb(uValue, ECMBOX_DATA_PORT);
    
    return true;
}


bool ec_mbox_get_value(unsigned char uCmd, unsigned char *puValue)
{
    if (puValue == NULL)
    {
        printk(KERN_ERR "Error: ec_mbox_get_valueIO Invalid Parameter, puValue == NULL \n\n");
        return false;
    }
    // Wait IBF clear
    if (wait_ibf()) 
		return false;

    //clear output buffer flag, prevent unfinished command
	inb(EC_STATUS_PORT);

    //send Read mailbox command
	outb((uCmd + 0xA0), EC_COMMAND_PORT);

    if (wait_obf()) 
		return false;
    
    *puValue = inb(EC_STATUS_PORT);
    
    return true;
}

// 0xC1 - Read buffer RAM
bool ec_mbox_read_buffer_ram(int ReadLength, unsigned char *pReadBuffer)
{
	unsigned char uStatus;
	int i, j;
	int banknum;
	int addition;

	if (pReadBuffer == NULL)
		goto ErrorReturn;
	if (ReadLength == 0)
		goto ErrorReturn;
	if (ReadLength > 256)
		ReadLength = 256;
	banknum = ReadLength / 32;

    if (ec_wait_mbox_cmd_clear() < 0)
        goto ErrorReturn;
	for (i = 0; i <= banknum; i++)
	{
		ec_mbox_set_value(EC_MBOX_PARA_OFFSET, (unsigned char)i);
		ec_mbox_set_value(EC_MBOX_CMD_OFFSET, EC_MBOX_READ_256_BYTES_BUFFER);

		if (ec_wait_mbox_cmd_clear() < 0)
			goto ErrorReturn;

		if (ec_mbox_get_value(EC_MBOX_STATUS_OFFSET, &uStatus) < 0)
			goto ErrorReturn;
		if (uStatus != _MBoxErr_Success)
			goto ErrorReturn;

		if (i == banknum)
		{
			addition = ReadLength % 32;
		}
		else
		{
			addition = 32;
		}

		for (j = 0; j < addition; j++)
		{
			if (ec_mbox_get_value((unsigned char)EC_MBOX_DAT_OFFSET(j), &pReadBuffer[i * 32 + j]) < 0)
				goto ErrorReturn;
		}
	}

	return 1;
ErrorReturn:
	return 0;
}

// 0xC0 - Clear buffer RAM
bool ec_mbox_clear_buffer_ram(void)
{
    if (ec_wait_mbox_cmd_clear() < 0)
        goto ErrorReturn;

	if (ec_mbox_set_value(EC_MBOX_CMD_OFFSET, EC_MBOX_CLEAR_256_BYTES_BUFFER) < 0)
		goto ErrorReturn;
	return 1;
ErrorReturn:
	return 0;
}

bool ec_mbox_smbus_i2c_set_data(unsigned char uCommand, unsigned char uWriteLength, unsigned char *pWriteBuffer, int *pReadLength, unsigned char *pReadBuffer)
{
	unsigned char WriteLen = uWriteLength;
	unsigned char ReadLen  = 0;

	unsigned char i;

//	UNREFERENCED_PARAMETER(pReadBuffer);

	if (WriteLen > 41)	// data 0x04~0x2c size
		WriteLen = 41;
	if (pReadLength != NULL)
		ReadLen = (unsigned char)*pReadLength;

	switch (uCommand & 0x7F)
	{
		case EC_MBOX_SMBUS_WRITE_WORD:
		case EC_MBOX_SMBUS_SEND_BYTE:
		case EC_MBOX_SMBUS_WRITE_BYTE:
			break;

		case EC_MBOX_I2C_READ_WRITE:
		case EC_MBOX_I2C_WRITE_READ:
			if (pReadLength != NULL && *pReadLength > 41)
				ReadLen = 41;

			if (ec_mbox_set_value((unsigned char)EC_MBOX_DAT_OFFSET(2), ReadLen) < 0)	// Set read count
				goto ErrorReturn;

		case EC_MBOX_SMBUS_WRITE_BLOCK:
			if (ec_mbox_set_value((unsigned char)EC_MBOX_DAT_OFFSET(3), WriteLen) < 0)	// Set write count
				goto ErrorReturn;
			break;

		case EC_MBOX_I2C_WRITEREAD_WITH_READ_BUFFER:
			if (ec_mbox_clear_buffer_ram() < 0)
				goto ErrorReturn;

			if (pReadLength != NULL && *pReadLength > 255)
				ReadLen = 0;

			if (ec_mbox_set_value((unsigned char)EC_MBOX_DAT_OFFSET(2), ReadLen) < 0)	// Set read count
				goto ErrorReturn;
			if (ec_mbox_set_value((unsigned char)EC_MBOX_DAT_OFFSET(3), WriteLen) < 0)	// Set write count
				goto ErrorReturn;
			break;

		case EC_MBOX_SMBUS_WRITE_QUICK:
		case EC_MBOX_SMBUS_READ_QUICK:
		case EC_MBOX_SMBUS_RECEIVE_BYTE:
		case EC_MBOX_SMBUS_READ_BYTE:
		case EC_MBOX_SMBUS_READ_WORD:
		case EC_MBOX_SMBUS_READ_BLOCK:
		default:
			break;
	}

	if (pWriteBuffer != NULL)
	{
		for (i = 0; i < uWriteLength; i++)
		{
			if (ec_mbox_set_value(EC_MBOX_DAT_OFFSET(i + 4), pWriteBuffer[i]) < 0)
				goto ErrorReturn;
		}
	}

	return true;
ErrorReturn:
	return false;
}

bool ec_mbox_smbus_i2c_get_data(unsigned char uCommand, int *pReadLength, unsigned char *pReadBuffer)
{
	unsigned char i;

	if (NULL == pReadBuffer || pReadLength == NULL)
		goto ErrorReturn;

	switch (uCommand & 0x7F)
	{
	case EC_MBOX_SMBUS_READ_WORD:
	case EC_MBOX_SMBUS_SEND_BYTE:
	case EC_MBOX_SMBUS_READ_BYTE:
		break;

	case EC_MBOX_I2C_READ_WRITE:
	case EC_MBOX_I2C_WRITE_READ:
		if (*pReadLength > 41)
			*pReadLength = 41;
		break;

	case EC_MBOX_SMBUS_READ_BLOCK:
		if (ec_mbox_get_value((unsigned char) EC_MBOX_DAT_OFFSET(2), (unsigned char *)pReadLength) < 0)
			goto ErrorReturn;
		break;

	case EC_MBOX_I2C_WRITEREAD_WITH_READ_BUFFER:
		if (*pReadLength >= 0x100)
			*pReadLength = 0x100;

		if (ec_mbox_read_buffer_ram(*pReadLength, pReadBuffer) < 0)
			goto ErrorReturn;
		goto Exit;

	case EC_MBOX_SMBUS_WRITE_QUICK:
	case EC_MBOX_SMBUS_READ_QUICK:
	case EC_MBOX_SMBUS_RECEIVE_BYTE:
	case EC_MBOX_SMBUS_WRITE_BYTE:
	case EC_MBOX_SMBUS_WRITE_WORD:
	case EC_MBOX_SMBUS_WRITE_BLOCK:
	default:
		goto Exit;
	}

	for (i = 0; i < *pReadLength; i++)
	{
		if (ec_mbox_get_value((unsigned char)EC_MBOX_DAT_OFFSET(i + 4), &pReadBuffer[i]) < 0)
			goto ErrorReturn;
	}

Exit:
	return true;
ErrorReturn:
	return false;
}

// This function wants to fixed the problem that ec_i2c_transfer() can not
// read/write more then 255 bytes.
int ec_i2c_do_transfer_new(unsigned char uDeviceID, unsigned short uSlavAddr, 
		unsigned int length, unsigned char uWriteLength, unsigned char *pWriteBuffer, 
		unsigned char uReadLength, unsigned char *pReadBuffer)
{
	unsigned int sm_ready;
	unsigned char i, uValue;
	int status = 0;
	unsigned char uHWPinNumber = 0xFF;
	unsigned char uProtocol = (uWriteLength == 0) ? SMBUS_I2C_READ_WRITE : SMBUS_I2C_WRITE_READ;

	// CHECK_PARAMETER
	if ((uReadLength > 0) && (pReadBuffer == NULL)) {
		printk(KERN_ERR "Error: Invalid Parameter, pReadBuffer == NULL \n\n");
		return -1;
	}

	if ((uWriteLength > 0) && (pWriteBuffer == NULL)) {
		printk(KERN_ERR "Error: Invalid Parameter, pWriteBuffer == NULL \n\n");
		return -1;
	}

	uHWPinNumber = get_ec_hw_pin_num_by_device_id(uDeviceID);
	if (uHWPinNumber == EC_HWPIN_NUMBER_UNUSED) {
		return -1;
	}

	// SMBUS Mutex Protection Start
	mutex_lock(&EC_SMBusMutex);

	// SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

	// Step 1: Translate HW Pin number to real SMBUS channel
	// Step 1-0. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 1-1. Send channel index command (Write 0x8A to 0x29A).
	//           EC will translate HW Pin number to real SMBUS channel
	outb(EC_SMBUS_CHANNEL_SET, EC_COMMAND_PORT);

	// Step 1-2. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 1-3. Write port number to index
	outb(uHWPinNumber, EC_STATUS_PORT);

	// Step 1-4. Wait OBF set
	if (wait_obf()) 
		goto ErrorReturn;

	uValue = inb(EC_STATUS_PORT);
	if (uValue == 0xFF) {
		printk(KERN_ERR "Error: Set HWPIN(0x%02X) Failed \n\n", uHWPinNumber);
		goto ErrorReturn;
	}

	// Step 2: Translate HW Pin number to real SMBUS channel
	// Step 2-0. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 2-1. Enable I2C block protocol command (Write 0x8C to 0x29A).
	outb(EC_SMBUS_ENABLE_I2C, EC_COMMAND_PORT);

	// Step 2-2. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 2-3. Write port number to index
	outb(uHWPinNumber, EC_STATUS_PORT);

	// Step 2-4. Wait OBF set
	if (wait_obf()) 
		goto ErrorReturn;

	uValue = inb(EC_STATUS_PORT);
	if (uValue == 0xFF) {
		printk(KERN_ERR "Error: Enable I2C block protocol Failed HWPIN(0x%02X) \n\n", uHWPinNumber);
		goto ErrorReturn;
	}

	// SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

	//
	// Step 3. Set I2C device address EX: 0xA4
	// 
	if (write_hw_ram(EC_SMBUS_SLV_ADDR, (unsigned char)uSlavAddr) < 0) {
		printk(KERN_ERR "Error: Select I2C device address(0x%02X) Failed \n\n", (unsigned char)uSlavAddr);
		goto ErrorReturnDirect;
	}

	if (write_hw_ram(EC_SMBUS_CMD, (unsigned char)(uSlavAddr >> 8)) < 0) {
		printk(KERN_ERR "Error: Select Chip Register Address(0x%02X) Failed \n\n", (unsigned char)(uSlavAddr >> 8));
		goto ErrorReturnDirect;
	}

	//
	// Step 4. Set WCount
	// 
	if (write_hw_extend_ram(EC_HW_EXTEND_RAM_ADDR_WCOUNT, uWriteLength) < 0) {
		printk(KERN_ERR "Error: Set HW Extend RAM WCOUNT Failed \n\n");
		goto ErrorReturnDirect;
	}

	// 
	// Step 5. Set Write Data to HW Extend RAM
	// 
	for (i = 0; i < uWriteLength; i++) 
	{
//		printk("EC_HW_EXTEND_RAM_ADDR_DATA(i): %d \n", i);
#ifdef QATEST
		printk("[%s] EcI2cData.WriteBuffer[%d]: 0x%X \n", __func__, i, pWriteBuffer[i]);
#endif
		if (write_hw_extend_ram(EC_HW_EXTEND_RAM_ADDR_DATA(i), pWriteBuffer[i]) < 0) {
			printk(KERN_ERR "Error: Set HW Extend RAM Data%d:[0x%X] Failed \n\n", i, pWriteBuffer[i]);
			goto ErrorReturnDirect;		
		}
	}

	// 
	// Step 6. Set Read Length
	// 
	if (write_hw_ram(EC_SMBUS_BLOCKCNT, uReadLength) < 0) {
		printk(KERN_ERR "Error: Set SMBUS HWRAM BlockCNT(%d) Failed \n\n", uReadLength);
		goto ErrorReturnDirect;
	}

	// 
	// Step 7. Start Transfer 
	// 
	if (write_hw_ram(EC_SMBUS_PROTOCOL_ADDRESS, uProtocol) < 0) {
		printk(KERN_ERR "Error: Set EC SMBUS write Byte Mode Failed \n\n");
		goto ErrorReturn;
	}

	// 
	// Step 8. Check EC Smbus/i2c states
	// 
	if (wait_smbus_protocol_finish()) {
		printk(KERN_ERR "Error: wait_smbus_protocol_finish Failed \n\n");
		goto ErrorReturnDirect;    
	}

	if (read_hw_ram(EC_SMBUS_STATUS_ADDRESS, &sm_ready)) {
		printk(KERN_ERR "Error: Check EC Smbus states Failed \n\n");
		goto ErrorReturnDirect;
	}

	// check no error
	if (sm_ready != 0x80) {
		printk(KERN_ERR "Error: SMBUS ERR:(0x%02X) \n\n", sm_ready & 0x7F);
		goto ErrorReturnDirect;
	}

	// 
	// Step 9. Get Data from HW RAM 
	// 
	if (uReadLength !=0 && pReadBuffer != NULL) 
	{
		for (i = 0; i < uReadLength; i++)
		{
			if (read_hw_ram(EC_SMBUS_DAT_OFFSET_ADDRESS(i), (unsigned int *)&pReadBuffer[i]) < 0) {
				printk(KERN_ERR "Error: Get HW RAM value Failed, offset:%d \n\n", i);
				goto ErrorReturnDirect;
			}		
		}
	}

	// SMBUS Mutex Protection End
	mutex_unlock(&EC_SMBusMutex);
	return 0;

ErrorReturn:
	// SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

ErrorReturnDirect:
	// SMBUS Mutex Protection End
	mutex_unlock(&EC_SMBusMutex);

	status = -1;
	return status;
}

int ec_i2c_do_transfer(unsigned char uDeviceID, unsigned short uSlavAddr, unsigned char uWriteLength, 
		unsigned char *pWriteBuffer, unsigned char uReadLength, unsigned char *pReadBuffer)
{
	unsigned int sm_ready;
	unsigned char i, uValue;
	int status = 0;
	unsigned char uHWPinNumber = 0xFF;
	unsigned char uProtocol = (uWriteLength == 0) ? SMBUS_I2C_READ_WRITE : SMBUS_I2C_WRITE_READ;

#ifdef QATEST
	printk("[%s] Offset: 0x%X \n", __func__, pWriteBuffer[0]);
	printk("[%s] ReadLength: %d \n", __func__, uReadLength);
	printk("[%s] WriteLength: %d \n", __func__, uWriteLength);
#endif

	// CHECK_PARAMETER
	if ((uReadLength > 0) && (pReadBuffer == NULL)) {
		printk(KERN_ERR "Error: Invalid Parameter, pReadBuffer == NULL \n\n");
		return -1;
	}

	if ((uWriteLength > 0) && (pWriteBuffer == NULL)) {
		printk(KERN_ERR "Error: Invalid Parameter, pWriteBuffer == NULL \n\n");
		return -1;
	}

	uHWPinNumber = get_ec_hw_pin_num_by_device_id(uDeviceID);
	if (uHWPinNumber == EC_HWPIN_NUMBER_UNUSED) {
		return -1;
	}

	// SMBUS Mutex Protection Start
	mutex_lock(&EC_SMBusMutex);

	// SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

	// Step 1: Translate HW Pin number to real SMBUS channel
	// Step 1-0. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 1-1. Send channel index command (Write 0x8A to 0x29A).
	//           EC will translate HW Pin number to real SMBUS channel
	outb(EC_SMBUS_CHANNEL_SET, EC_COMMAND_PORT);

	// Step 1-2. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 1-3. Write port number to index
	outb(uHWPinNumber, EC_STATUS_PORT);

	// Step 1-4. Wait OBF set
	if (wait_obf()) 
		goto ErrorReturn;

	uValue = inb(EC_STATUS_PORT);
	if (uValue == 0xFF) {
		printk(KERN_ERR "Error: Set HWPIN(0x%02X) Failed \n\n", uHWPinNumber);
		goto ErrorReturn;
	}

	// Step 2: Translate HW Pin number to real SMBUS channel
	// Step 2-0. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 2-1. Enable I2C block protocol command (Write 0x8C to 0x29A).
	outb(EC_SMBUS_ENABLE_I2C, EC_COMMAND_PORT);

	// Step 2-2. Wait IBF clear
	if (wait_ibf()) 
		goto ErrorReturn;

	// Step 2-3. Write port number to index
	outb(uHWPinNumber, EC_STATUS_PORT);

	// Step 2-4. Wait OBF set
	if (wait_obf()) 
		goto ErrorReturn;

	uValue = inb(EC_STATUS_PORT);
	if (uValue == 0xFF) {
		printk(KERN_ERR "Error: Enable I2C block protocol Failed HWPIN(0x%02X) \n\n", uHWPinNumber);
		goto ErrorReturn;
	}

	// SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

	//
	// Step 3. Set I2C device address EX: 0xA4
	// 
	if (write_hw_ram(EC_SMBUS_SLV_ADDR, (unsigned char)uSlavAddr) < 0) {
		printk(KERN_ERR "Error: Select I2C device address(0x%02X) Failed \n\n", (unsigned char)uSlavAddr);
		goto ErrorReturnDirect;
	}

	if (write_hw_ram(EC_SMBUS_CMD, (unsigned char)(uSlavAddr >> 8)) < 0) {
		printk(KERN_ERR "Error: Select Chip Register Address(0x%02X) Failed \n\n", (unsigned char)(uSlavAddr >> 8));
		goto ErrorReturnDirect;
	}

	//
	// Step 4. Set WCount
	// 
	if (write_hw_extend_ram(EC_HW_EXTEND_RAM_ADDR_WCOUNT, uWriteLength) < 0) {
		printk(KERN_ERR "Error: Set HW Extend RAM WCOUNT Failed \n\n");
		goto ErrorReturnDirect;
	}

	// 
	// Step 5. Set Write Data to HW Extend RAM
	// 
	for (i = 0; i < uWriteLength; i++) 
	{
		if (write_hw_extend_ram(EC_HW_EXTEND_RAM_ADDR_DATA(i), pWriteBuffer[i]) < 0) {
			printk(KERN_ERR "Error: Set HW Extend RAM Data%d:[0x%X] Failed \n\n", i, pWriteBuffer[i]);
			goto ErrorReturnDirect;		
		}
	}

	// 
	// Step 6. Set Read Length
	// 
	if (write_hw_ram(EC_SMBUS_BLOCKCNT, uReadLength) < 0) {
		printk(KERN_ERR "Error: Set SMBUS HWRAM BlockCNT(%d) Failed \n\n", uReadLength);
		goto ErrorReturnDirect;
	}

	// 
	// Step 7. Start Transfer 
	// 
	if (write_hw_ram(EC_SMBUS_PROTOCOL_ADDRESS, uProtocol) < 0) {
		printk(KERN_ERR "Error: Set EC SMBUS write Byte Mode Failed \n\n");
		goto ErrorReturn;
	}

	// 
	// Step 8. Check EC Smbus/i2c states
	// 
	if (wait_smbus_protocol_finish()) {
		printk(KERN_ERR "Error: wait_smbus_protocol_finish Failed \n\n");
		goto ErrorReturnDirect;    
	}

	if (read_hw_ram(EC_SMBUS_STATUS_ADDRESS, &sm_ready)) {
		printk(KERN_ERR "Error: Check EC Smbus states Failed \n\n");
		goto ErrorReturnDirect;
	}

	// check no error
	if (sm_ready != 0x80) {
		printk(KERN_ERR "Error: SMBUS ERR:(0x%02X) \n\n", sm_ready & 0x7F);
		goto ErrorReturnDirect;
	}

	// 
	// Step 9. Get Data from HW RAM 
	// 
	if (uReadLength !=0 && pReadBuffer != NULL) 
	{
		for (i = 0; i < uReadLength; i++)
		{
			if (read_hw_ram(EC_SMBUS_DAT_OFFSET_ADDRESS(i), (unsigned int *)&pReadBuffer[i]) < 0) {
				printk(KERN_ERR "Error: Get HW RAM value Failed, offset:%d \n\n", i);
				goto ErrorReturnDirect;
			}		
		}
	}

	// SMBUS Mutex Protection End
	mutex_unlock(&EC_SMBusMutex);
	return 0;

ErrorReturn:
	// SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

ErrorReturnDirect:
	// SMBUS Mutex Protection End
	mutex_unlock(&EC_SMBusMutex);

	status = -1;
	return status;
}

int ec_mbox_smbus_i2c(unsigned char uDeviceID, unsigned char uCommand, unsigned short uSlavAddr, 
		unsigned char uSMBCommand, unsigned char uWriteLength, unsigned char *pWriteBuffer, 
		int *pReadLength, unsigned char *pReadBuffer)
{
    unsigned char uStatus = _MBoxErr_Fail;
    // SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

    if (ec_wait_mbox_cmd_clear() < 0)
        goto ErrorReturn;

    if (ec_mbox_set_value(EC_MBOX_PARA_OFFSET, uDeviceID) < 0)
        goto ErrorReturn;

    if (ec_mbox_set_value((unsigned char) EC_MBOX_DAT_OFFSET(0), (unsigned char)uSlavAddr) < 0)
        goto ErrorReturn;

    if (ec_mbox_set_value((unsigned char) EC_MBOX_DAT_OFFSET(1), uSMBCommand) < 0)
        goto ErrorReturn;

	if (ec_mbox_smbus_i2c_set_data(uCommand, uWriteLength, pWriteBuffer, pReadLength, pReadBuffer) < 0)
		goto ErrorReturn;

	// Start transmit
	if (ec_mbox_set_value(EC_MBOX_CMD_OFFSET, uCommand) < 0)
		goto ErrorReturn;

    if (ec_wait_mbox_cmd_clear() < 0)
        goto ErrorReturn;

	if (ec_mbox_get_value(EC_MBOX_STATUS_OFFSET, &uStatus) < 0)
		goto ErrorReturn;

	if (uStatus != 0x80)
	{
		printk(KERN_ERR"    --> SMBusI2C status code: 0x%02X \n", uStatus & 0x7F);
		goto ErrorReturn;
	}

	if (*pReadLength !=0 && pReadBuffer != NULL) 
	{
		if (ec_mbox_smbus_i2c_get_data(uCommand, pReadLength, pReadBuffer) < 0)
			goto ErrorReturn;
	}

    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

    return 0;

ErrorReturn:
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);
	return -1;    
}

bool ec_init_eeprom(unsigned char uBlock, unsigned char uOffset)
{
    unsigned char status;
    int retry = 10;
    
    if (uBlock < EC_EEPROM_INFO_BLOCK || uBlock > EC_EEPROM_OEM_BLOCK)
    {
        printk(KERN_ERR "Invalid Input (0x%02X) \n\n", uBlock);
        return false;
    }

    // SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

    // Step 1. Read Status
    while (retry--) {
        if (wait_ibf())
			goto ErrorReturn;

		outb(EC_EEPROM_STATUS_READ, EC_COMMAND_PORT);

        if (wait_obf())
			goto ErrorReturn;

        status = inb(EC_STATUS_PORT);
        if (status & 0x3) {		// SPI Ready & EC Synced
            break;
        } else {
            if (retry == 0) {
                printk(KERN_ERR "EEPROM Init Failed status(0x%02X)!! \n\n", status);
                goto ErrorReturn;
            }
        }
    }

    // OEM Block: Set Bank
    if (uBlock == EC_EEPROM_OEM_BLOCK) {
        if (wait_ibf())
			goto ErrorReturn;

		outb(EC_EEPROM_OEM_BANK_WRITE, EC_COMMAND_PORT);

        if (wait_ibf())
			goto ErrorReturn;
		outb(uBlock, EC_STATUS_PORT);
        
		if (wait_obf())
			goto ErrorReturn;

        status = inb(EC_STATUS_PORT);
        if (status != 0x1) {
            printk(KERN_ERR "EEPROM select OEM bank Failed!! \n\n");
            goto ErrorReturn;
        }
    }
    
    // Step2: Set index
    if (wait_ibf())
		goto ErrorReturn;

    if (uBlock == EC_EEPROM_OEM_BLOCK) {
		outb(EC_EEPROM_OEM_IDX_WRITE, EC_COMMAND_PORT);
    } else {
		outb(EC_EEPROM_INFO_IDX_WRITE, EC_COMMAND_PORT);
    }

    if (wait_ibf())
		goto ErrorReturn;

	outb(uOffset, EC_STATUS_PORT);

    if (wait_obf())
		goto ErrorReturn;

    status = inb(EC_STATUS_PORT);
    if (status != 0x1) {
        printk(KERN_ERR "EEPROM Set index Failed uBlock(%d), uOffset(0x%02X) !! \n\n", uBlock, uOffset);
        goto ErrorReturn;
    }
    
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);
    
    return true;

ErrorReturn:
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

    return false;
}

/* Get EC EEPROM Value [SuperIO Mutex] */
bool ec_eeprom_get_value(unsigned char uBlock, unsigned char uOffset, unsigned char *pObjOut)
{
    if (uBlock < EC_EEPROM_INFO_BLOCK || uBlock > EC_EEPROM_OEM_BLOCK) {
        printk(KERN_ERR "Erroe: Invalid Input (0x%02X) \n\n", uBlock);
        return false;
    }
    // CHECK_PARAMETER
	if (pObjOut == NULL) {
        printk(KERN_ERR "Error: Invalid Parameter, pObjOut == NULL \n\n");
		return false;
	}

	/* Initialize EC EEPROM [SuperIO Mutex] */
    if (!ec_init_eeprom(uBlock, uOffset)) {
		return false;
	}

    // SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

    if (wait_ibf()) {
		goto ErrorReturn;
	}

    if (uBlock == EC_EEPROM_OEM_BLOCK) {
		outb(EC_EEPROM_OEM_DATA_READ, EC_COMMAND_PORT);
    } else {
		outb(EC_EEPROM_INFO_DATA_READ, EC_COMMAND_PORT);
    }

    if (wait_obf())
		goto ErrorReturn;

    *pObjOut = inb(EC_STATUS_PORT);
    
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

	/* Wait for EC EEPROM Done [SuperIO Mutex] */
    return wait_eeprom_done();

ErrorReturn:
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);
    return false;
}

/* Wait for EC EEPROM Done [SuperIO Mutex] */
bool wait_eeprom_done(void)
{
    unsigned char status;
    
	// SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

    if (wait_ibf())
		goto ErrorReturn;

	outb(EC_EEPROM_RESULT_READ, EC_COMMAND_PORT);

    if (wait_obf())
		goto ErrorReturn;

    status = inb(EC_STATUS_PORT);
    if (status != 0x80)
    {
        printk(KERN_ERR "EEPROM process Failed status(0x%02X)!! \n\n", status);
        goto ErrorReturn;
    }

    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);
	// The SMBus is low speed bus, we need to wait a while before the next operation
	udelay(1000);
    return true;

ErrorReturn:
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);
    return false;
}

int ec_i2c_transfer(unsigned char uDeviceID, unsigned short uSlavAddr, unsigned char uWriteLength, 
		unsigned char *pWriteBuffer, unsigned char uReadLength, unsigned char *pReadBuffer)
{
	int iReadLen = uReadLength;
	unsigned char i2cCmd = EC_MBOX_I2C_READ_WRITE;
	int status = 0;

	if (bIsMailBoxSupported) {
		iReadLen = uReadLength;
		i2cCmd = EC_MBOX_I2C_READ_WRITE;
		if (uWriteLength == 0) {
			i2cCmd = EC_MBOX_I2C_READ_WRITE;
		} else {
			i2cCmd = EC_MBOX_I2C_WRITE_READ;
		}
		return ec_mbox_smbus_i2c(uDeviceID, i2cCmd, (unsigned char)uSlavAddr, (unsigned char)(uSlavAddr >> 8), 
				uWriteLength, pWriteBuffer, &iReadLen, pReadBuffer);
	}

	status = ec_i2c_do_transfer(uDeviceID, uSlavAddr, uWriteLength, pWriteBuffer, uReadLength, pReadBuffer);
	return status; 
}

static int adv_get_all_hw_pin_num(void)
{
	int i, j;
	if (NULL == PDynamic_Tab) {
		printk(KERN_ERR "Error: pDynamic_Tab == NULL \n\n");
		return -ENODATA;
	}

	for (i = 0; i < EC_MAX_DEVICE_ID_NUM; i++) {
		for (j = 0; j < EC_MAX_TBL_NUM; j++) {
			if (PDynamic_Tab[j].DeviceID == i) {
				DeviceId2HWPinNum[i] = PDynamic_Tab[j].HWPinNumber;
				break;
			}
		}
	}

	return 0;
}

unsigned char get_ec_hw_pin_num_by_device_id(unsigned char uDeviceID) 
{
	if (uDeviceID < EC_MAX_DEVICE_ID_NUM) {
		return DeviceId2HWPinNum[uDeviceID];
	}

	return EC_HWPIN_NUMBER_UNUSED;
}

bool ec_eeprom_is_key_empty(unsigned char *pKey)
{
	int i = 0;

	for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
		if (pKey[i] != 0xFF)
			return false;
	}

	return true;
}

static void sha1_encrypt_decrypt(unsigned char *srcbuf, unsigned char *desbuf, 
		unsigned long datalen, unsigned char *key, unsigned long keylen)
{
	unsigned char hash_key_result_buf[64] = {0};
	int i;

	if (datalen > sizeof(hash_key_result_buf)) {
		datalen = sizeof(hash_key_result_buf);
	}

	sha1(hash_key_result_buf, key, keylen);

	for (i = 0; i < datalen; i++)
	{
		desbuf[i] = srcbuf[i] ^ hash_key_result_buf[i];
	}

	return;
}

int ec_eeprom_get_security_key(unsigned char *pKey) 
{
	int i = 0;

	if (pKey == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter: pIsLock == NULL \n\n");
		return EINVAL;
	}
	for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
		if (ec_eeprom_get_value(EC_EEPROM_OEM_BLOCK, EC_LOCK_KEY_OFFSET + i, &pKey[i]) < 0)
			break;
	}

	return 0;
}

int ec_eeprom_set_security_key(unsigned char *pKey) 
{
	int i = 0;

	if (pKey == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter: pIsLock == NULL \n\n");
		return EINVAL;
	}
	for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
		if (ec_eeprom_set_value(EC_EEPROM_OEM_BLOCK, EC_LOCK_KEY_OFFSET + i, pKey[i]) < 0)
			break;
	}

	return 0;
}

/* Set EC EEPROM Value [SuperIO Mutex] */
bool ec_eeprom_set_value(unsigned char uBlock, unsigned char uOffset, unsigned char uValue)
{
    if (uBlock < EC_EEPROM_INFO_BLOCK || uBlock > EC_EEPROM_OEM_BLOCK) {
        printk(KERN_WARNING "Invalid Input (0x%02X) \n\n", uBlock);
        return false;
    }

	/* Initialize EC EEPROM [SuperIO Mutex] */
    if (ec_init_eeprom(uBlock, uOffset) < 0) {
		return -1;
	}

    // SuperIO Mutex Protection Start
	mutex_lock(&EC_SuperIOMutex);

    if (wait_ibf()) {
		goto ErrorReturn;
	}

    if (uBlock == EC_EEPROM_OEM_BLOCK) {
    	outb(EC_EEPROM_OEM_DATA_WRITE, ec_command_port);
    } else {
    	outb(EC_EEPROM_INFO_DATA_WRITE, ec_command_port);
    }

    if (wait_ibf()) {
		goto ErrorReturn;
	}

    outb(uValue, ec_status_port);
    
	if (wait_obf()) {
		goto ErrorReturn;
	}

	if (inb(ec_status_port) != 0x1) {
		printk(KERN_ERR "Error: EEPROM Set value Failed uBlock(%d), uOffset(0x%02X), uValue(0x%02X) !! \n\n", 
				uBlock, uOffset, uValue);
        goto ErrorReturn;
    }
    
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);

	/* Wait for EC EEPROM Done [SuperIO Mutex] */
    return wait_eeprom_done();

ErrorReturn:
    // SuperIO Mutex Protection End
	mutex_unlock(&EC_SuperIOMutex);   
    return false;
}

int ec_eeprom_set_protect(peeprom_protect_data pProtectData)
{
	int i;
	unsigned char InKey[EC_LOCK_KEY_SIZE] = {0};
	unsigned char SecureKey[EC_LOCK_KEY_SIZE] = {0};
	// 0xFF -> Protect
	// 0x00 -> Unprotect
	unsigned char uLockByte;

	if (pProtectData == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter: pIsLock == NULL \n\n");
		return -1;
	}

	if (pProtectData->PwdLength > EC_LOCK_KEY_SIZE) {
		return -1;
	}
	uLockByte = pProtectData->Protected ? 0xFF : 0x00;

	memcpy(InKey, pProtectData->Password, pProtectData->PwdLength);

	sha1_encrypt_decrypt(InKey, InKey, pProtectData->PwdLength, (unsigned char *)g_asg_key, strlen(g_asg_key));

	if (ec_eeprom_get_security_key(SecureKey) < 0) {
		printk(KERN_WARNING "ec_eeprom_get_security_key failed \n");
		return -1;
	}

	if (pProtectData->Protected) {
		if (!ec_eeprom_is_key_empty(SecureKey)) {
			printk(KERN_WARNING "It was locked \n");
			return -1;
		} else {
			if (ec_eeprom_set_security_key(InKey) < 0) {
				printk(KERN_WARNING "ec_eeprom_set_security_key failed \n");
				return -1;
			}
		}
	} else {
		if (!ec_eeprom_is_key_empty(SecureKey)) {
			if (memcmp(SecureKey, InKey, EC_LOCK_KEY_SIZE) != 0) {
				printk(KERN_WARNING "Wrong Password \n");
				return -1;			
			}
			else {
				// Correct Password, Clear Key, and then unprotect
				memset(InKey, 0xFF, EC_LOCK_KEY_SIZE);
				if (ec_eeprom_set_security_key(InKey) < 0) {
					printk(KERN_WARNING "ec_eeprom_set_security_key (Unlock) failed \n");
					return -1;
				}
			}
		}
	}

	for (i = 0; i < EC_EEPROM_BYTE_PROTECT_OFFSET_COUNT; i++) {
		if (ec_eeprom_set_value(EC_EEPROM_INFO_BLOCK, EC_INFO_DATA_BYTE_PROTECT_OFFSET + i, uLockByte) < 0) {
			printk(KERN_WARNING "ec_eeprom_set_value failed offset:0x%x\n", i);
			break;
		}
	}

	return 0;
}

static __inline bool is_valid_barcode_slave_address(unsigned short SlaveAddress)
{
	if ( SlaveAddress == EC_BARCODE_EEPROM0_BANK(0) ||
			SlaveAddress == EC_BARCODE_EEPROM0_BANK(2) ||
			SlaveAddress == EC_BARCODE_EEPROM0_BANK(3) ||
			SlaveAddress == EC_BARCODE_EEPROM1_BANK(0) ||
			SlaveAddress == EC_BARCODE_EEPROM1_BANK(2) ||
			SlaveAddress == EC_BARCODE_EEPROM1_BANK(3) ) {
		return true;
	} else {
		return false;
	}
}

static __inline int get_barcode_lock_key_offset(unsigned short SlaveAddress, unsigned char *pOffset)
{
	int    status = 0;
	unsigned char offset = 0;
	if (NULL == pOffset || (!is_valid_barcode_slave_address(SlaveAddress) )) {
		printk(KERN_ERR "Error: Invalid Parameter, pIsLock == NULL \n\n");
		return -1;
	}

	switch (SlaveAddress) {
	case EC_BARCODE_EEPROM0_BANK(0):
	case EC_BARCODE_EEPROM1_BANK(0):
		offset = EC_BARCODE_LOCK_KEY_OFFSET_BANK(0);
		break;
	case EC_BARCODE_EEPROM0_BANK(2):
	case EC_BARCODE_EEPROM1_BANK(2):
		offset = EC_BARCODE_LOCK_KEY_OFFSET_BANK(1);
		break;
	case EC_BARCODE_EEPROM0_BANK(3):
	case EC_BARCODE_EEPROM1_BANK(3):
		offset = EC_BARCODE_LOCK_KEY_OFFSET_BANK(2);
		break;
	default:
		offset = 0;
		status = -1;
		break;
	}

	*pOffset = offset;
	return status;
}

int ec_barcode_eeprom_set_protect(pec_barcode_eeprom_protect_data pProtectData)
{
	int status = 0;
#ifdef QATEST
	int i = 0, tmp = 0;
#endif
	unsigned char InKey[EC_LOCK_KEY_SIZE+EC_LOCK_KEY_SIZE] = {0};
	unsigned char SecureKey[EC_LOCK_KEY_SIZE+EC_LOCK_KEY_SIZE] = {0};

	if (pProtectData == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter, pIsLock == NULL \n\n");
		return -1;
	}

	if (pProtectData->PwdLength > EC_LOCK_KEY_SIZE)
		return -1;

	memcpy((unsigned char *)InKey, pProtectData->Password, pProtectData->PwdLength);
#ifdef QATEST
			printk("[Step1] pProtectData->PwdLength: 0x%d \n", pProtectData->PwdLength);
			for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
				tmp = pProtectData->Password[i];
				printk("[Step1] pProtectData->Password[%d]: 0x%d \n", i, tmp);
				tmp = InKey[i];
				printk("[Step1] Inkey[%d]: 0x%d \n", i, tmp);
			}
#endif

	sha1_encrypt_decrypt((unsigned char *)InKey, (unsigned char *)InKey, pProtectData->PwdLength, (unsigned char *)g_asg_key, strlen(g_asg_key));

	if (ec_barcode_eeprom_get_security_key(pProtectData->SlaveAddress, (unsigned char *)SecureKey) < 0) {
		printk(KERN_ERR "ec_barcode_eeprom_get_security_key failed \n");
		goto Exit;
	}

#ifdef QATEST
			for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
				tmp = SecureKey[i];
				printk("[Step2] SecureKey[%d]: 0x%d \n", i, tmp);
				tmp = InKey[i];
				printk("[Step2] Inkey[%d]: 0x%d \n", i, tmp);
			}
#endif

	if (pProtectData->Protected) {
		// It was locked mode.
		if (!ec_eeprom_is_key_empty((unsigned char *)SecureKey)) {
			printk(KERN_ERR ": It was locked \n");
			status = -1;
			goto Exit;
		} else {
			if (ec_barcode_eeprom_set_security_key(pProtectData->SlaveAddress, (unsigned char *)InKey) < 0) {
				printk(KERN_ERR ": ec_barcode_eeprom_set_security_key failed \n");
				goto Exit;
			}		
		}
	} else {
		// It was unlocked mode.
		if (!ec_eeprom_is_key_empty((unsigned char *)SecureKey)) {
#ifndef PASSWDCLEAN
			if (memcmp((unsigned char *)SecureKey, (unsigned char *)InKey, EC_LOCK_KEY_SIZE) != 0) {
				printk(KERN_WARNING "Wrong Password \n");
				status = -1;
				goto Exit;			
			} else {
				// Correct Password, Clear Key, and then unprotect
				memset((unsigned char *)InKey, 0xFF, EC_LOCK_KEY_SIZE);
				if (ec_barcode_eeprom_set_security_key(pProtectData->SlaveAddress, (unsigned char *)InKey) < 0) {
					printk(KERN_ERR "(Unlock) failed \n");
					goto Exit;
				}
			}
#else
			// Correct Password, Clear Key, and then unprotect
			memset((unsigned char *)InKey, 0xFF, EC_LOCK_KEY_SIZE);
			if (ec_barcode_eeprom_set_security_key(pProtectData->SlaveAddress, (unsigned char *)InKey) < 0) {
				printk(KERN_ERR "(Unlock) failed \n");
				goto Exit;
			}
#endif
		}
	}

Exit:
	return status;
}

int ec_barcode_eeprom_check_is_locked(unsigned short uSlavAddr, unsigned long *pIsLock)
{
	int    status = 0;
	unsigned int SecureKey[EC_LOCK_KEY_SIZE] = {0};

	if (pIsLock == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter, pIsLock == NULL \n\n");
		return -1;
	}

	if (!is_valid_barcode_slave_address(uSlavAddr)) {
		printk(KERN_ERR "Error: Invalid Parameter, unknown slave address \n\n");
		return -1;
	}

	if (ec_barcode_eeprom_get_security_key(uSlavAddr, (unsigned char *)SecureKey) < 0) {
		printk(KERN_ERR "failed \n");
		goto Exit;
	}

	*pIsLock = ec_eeprom_is_key_empty((unsigned char *)SecureKey) ? 0 : 1; 

Exit:
	return status;
}

int ec_barcode_eeprom_get_security_key(unsigned short uSlavAddr, unsigned char *pKey) 
{
	int status = 0;
	unsigned char keyOffset = 0;
	unsigned short keybase = 0;

	if (pKey == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter, pIsLock == NULL \n\n");
		return -1;
	}

	if (!is_valid_barcode_slave_address(uSlavAddr)) {
		printk(KERN_ERR "Error: Invalid Parameter, unknown slave address \n\n");
		return -1;
	}

	if (get_barcode_lock_key_offset(uSlavAddr, &keyOffset) < 0) {
		printk(KERN_ERR "Error: status:0x%X \n\n", status);
		goto Exit;
	}

	keybase = (uSlavAddr & EC_BARCODE_EEPROM_MASK) ? EC_BARCODE_LOCK_KEY_BASE1 : EC_BARCODE_LOCK_KEY_BASE0;
	status = ec_i2c_transfer(EC_DID_SMBEEPROM, keybase, sizeof(keyOffset), &keyOffset, EC_LOCK_KEY_SIZE, pKey);
	if (status < 0) {
		printk(KERN_ERR "Error: status:0x%X \n\n", status);
		goto Exit;
	}

Exit:
	return status;
}

int ec_barcode_eeprom_set_security_key(unsigned short uSlavAddr, unsigned char *pKey) 
{
	int status = 0;
	unsigned char keyOffset = 0;
	unsigned char uBuffer[EC_LOCK_KEY_SIZE + 1] = {0};
	unsigned short keybase = 0;

	if (pKey == NULL) {
		printk(KERN_ERR "Error: Invalid Parameter, pIsLock == NULL \n\n");
		return -1;
	}

	if (!is_valid_barcode_slave_address(uSlavAddr)) {
		printk(KERN_ERR "Error: Invalid Parameter, unknown slave address \n\n");
		return -1;
	}

	if (get_barcode_lock_key_offset(uSlavAddr, &keyOffset) < 0) {
		printk(KERN_ERR "Error: status:0x%X \n\n", status);
		return status;
	}

	keybase = (uSlavAddr & EC_BARCODE_EEPROM_MASK) ? EC_BARCODE_LOCK_KEY_BASE1 : EC_BARCODE_LOCK_KEY_BASE0;

	if (keybase == EC_BARCODE_LOCK_KEY_BASE0) {
		unsigned char i;
		// 0xA2 can only be wrote by ec_eeprom_set_value / EcEepromBlockWrite
		for (i = 0; i < EC_LOCK_KEY_SIZE; i++) {
			if (ec_eeprom_set_value(EC_EEPROM_OEM_BLOCK, keyOffset + i, pKey[i]) < 0) {
				status = -1;
				break;
			}
		}
	} else {
		uBuffer[0] = keyOffset;
		memcpy(&uBuffer[1], pKey, EC_LOCK_KEY_SIZE);
		status = ec_i2c_transfer(EC_DID_SMBEEPROM, keybase, sizeof(uBuffer), 
				uBuffer, 0, NULL);	
	}

	if (status < 0) {
		printk(KERN_ERR "Error: status:0x%X \n\n", status);
	}

	return status;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ec_eeprom_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
#else
static long ec_eeprom_ioctl(struct file* filp, unsigned int cmd, unsigned long arg )
#endif
{
	ec_i2c_data EcI2cData = {0};
	ec_barcode_eeprom_protect_data EepromProtData = {0};
	eeprom_config EepromConfig = {0};
	unsigned char tmpStr[64] = {0};
	unsigned long ulIsLock = 0;
	unsigned char tmpOffset = 0;
	int i = 0, j = 0, RWCount, CurrentLength;

	switch ( cmd ) 
	{
	case IOCTL_COMMON_EC_I2C_TRANSFER:
	{
		// 1. Retrieve an I/O request's input buffer.
		if (copy_from_user(&EcI2cData, (ec_i2c_data __user *)arg, sizeof(ec_i2c_data))) {
			printk(KERN_ERR "Error: copy_from_user() \n");
			return -EFAULT;
		} else {
#ifdef QATEST
delay_1s();
printk("EcI2cData.DeviceID:0x%X \n", (int)EcI2cData.DeviceID);
printk("EcI2cData.Address:0x%X \n", (int)EcI2cData.Address);
printk("EcI2cData.ReadLength:%d \n", (int)EcI2cData.ReadLength);
printk("EcI2cData.WriteLength:%d \n", (int)EcI2cData.WriteLength);
printk("EcI2cData.WriteBuffer[0]:0x%X \n", EcI2cData.WriteBuffer[0]);
printk("EcI2cData.length:%d \n", EcI2cData.length);
delay_1s();
#endif
			// 2. Check Output buffer length
			if (EcI2cData.ReadLength > I2C_SMBUS_USE_MAX || EcI2cData.WriteLength > I2C_SMBUS_USE_MAX) {
				printk(KERN_ERR "Eeprom read/write length out of range! \n");
				return -EINVAL;
			}
			if (EcI2cData.WriteBuffer[0] > 0xFF) {
				printk(KERN_ERR "Set offset value out of range (0x00~0xFF) ! \n");
				return -EINVAL;
			}
			if ((EcI2cData.WriteBuffer[0] + EcI2cData.length) > 256) {
				printk(KERN_ERR "Set value out of range (offset+length<=256) ! \n");
				return -EINVAL;
			}

			// 3. Check if a valid DeviceID
			switch(EcI2cData.DeviceID) 
			{
			case EC_DID_SMBEEPROM:
			case EC_DID_SMBOEM0:
			case EC_DID_SMBOEM1:
			case EC_DID_SMBOEM2:
			case EC_DID_I2COEM1:
				break;
			default:
				printk(KERN_ERR "Set wrong DeviceID! \n");
				return -EINVAL;
			}

			// 4. Check permission
			// The restricted area (password zone), disable R/W access
			if (EcI2cData.DeviceID == EC_DID_SMBEEPROM) {
				if (EcI2cData.Address == EC_BARCODE_EEPROM0_BANK(1) ||
						EcI2cData.Address == EC_BARCODE_EEPROM1_BANK(1)) {
					printk(KERN_ERR "Set wrong address(address: %u)! \n", EcI2cData.Address);
					return -EFAULT;
				}
			}
			if (EcI2cData.DeviceID == EC_DID_SMBEEPROM && EcI2cData.WriteLength > 1) {
				if (EcI2cData.Address == EC_BARCODE_EEPROM0_BANK(0) ||
						EcI2cData.Address == EC_BARCODE_EEPROM0_BANK(2) ||
						EcI2cData.Address == EC_BARCODE_EEPROM0_BANK(3) ||
						EcI2cData.Address == EC_BARCODE_EEPROM1_BANK(0) ||
						EcI2cData.Address == EC_BARCODE_EEPROM1_BANK(2) ||
						EcI2cData.Address == EC_BARCODE_EEPROM1_BANK(3)) { 
					if (ec_barcode_eeprom_check_is_locked(EcI2cData.Address, &ulIsLock) < 0) {
						printk(KERN_ERR "Check bank(address: %u) error! \n", EcI2cData.Address);
						return -EFAULT;
					}
					if (ulIsLock) {
						printk(KERN_ERR "Bank(address: %u) is locked! \n", EcI2cData.Address);
						return -EACCES;
					}
				}
			}

			memset(tmpStr, 0, 64);
			CurrentLength = 0;
			if (!EcI2cData.isRead) {
				// Write eeprom
				tmpStr[0] = EcI2cData.WriteBuffer[0];
				RWCount = 1;
				if (EcI2cData.length <= I2C_SMBUS_WRITE_MAX) {
					tmpOffset = I2C_SMBUS_WRITE_MAX - (EcI2cData.WriteBuffer[0]%I2C_SMBUS_WRITE_MAX);
					if (tmpOffset > EcI2cData.length) {
						tmpOffset = EcI2cData.length;
					}
#ifdef QATEST
					printk("I2C_SMBUS_WRITE_MAX: %d \n",  I2C_SMBUS_WRITE_MAX);
					printk("EcI2cData.WriteBuffer[0]: %d \n", EcI2cData.WriteBuffer[0]);
					printk("tmpOffset: %d \n", tmpOffset);
#endif
					if (tmpOffset != 0 && tmpOffset != I2C_SMBUS_WRITE_MAX) {
						for (j = 1; j <= tmpOffset; j++) {
							tmpStr[j] = EcI2cData.WriteBuffer[RWCount];
#ifdef QATEST
							printk("RWCount:%d, tmpStr[%d]:0x%X \n", RWCount, j, tmpStr[j]);
#endif
							RWCount++;
						}
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, (tmpOffset + 1),
								tmpStr, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
						tmpStr[0] = EcI2cData.WriteBuffer[0] + tmpOffset;
						for (j = 1; j <= (EcI2cData.WriteLength - tmpOffset - 1); j++) {
							tmpStr[j] = EcI2cData.WriteBuffer[RWCount];
#ifdef QATEST
							printk("RWCount:%d, tmpStr[%d]:0x%X \n", RWCount, j, tmpStr[j]);
#endif
							RWCount++;
						}
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, (EcI2cData.WriteLength - tmpOffset),
								tmpStr, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
					} else {
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, EcI2cData.WriteLength, 
								EcI2cData.WriteBuffer, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
					}
				} else {
					tmpOffset = I2C_SMBUS_WRITE_MAX - (EcI2cData.WriteBuffer[0]%I2C_SMBUS_WRITE_MAX);
					if (tmpOffset != 0 && tmpOffset != I2C_SMBUS_WRITE_MAX) {
						for (j = 1; j <= tmpOffset; j++) {
							tmpStr[j] = EcI2cData.WriteBuffer[RWCount];
							RWCount++;
						}
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, (tmpOffset + 1),
								tmpStr, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
						tmpStr[0] = EcI2cData.WriteBuffer[0] + tmpOffset;
					} else {
						tmpOffset = 0;
					}
					CurrentLength = I2C_SMBUS_WRITE_MAX;
					for (i = 0; i < (EcI2cData.length - tmpOffset); i+=I2C_SMBUS_WRITE_MAX) {
						CurrentLength = (EcI2cData.length - tmpOffset - i) >= I2C_SMBUS_WRITE_MAX ? I2C_SMBUS_WRITE_MAX : (EcI2cData.length - tmpOffset - i);
#ifdef QATEST
						printk("CurrentLength: %d \n", CurrentLength);
#endif
						for (j = 1; j <= CurrentLength; j++) {
							tmpStr[j] = EcI2cData.WriteBuffer[RWCount];
							RWCount++;
						}
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, (CurrentLength + 1), 
								tmpStr, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
#ifdef QATEST
						for (j = 0; j <= CurrentLength; j++) {
							if (j == CurrentLength) {
								printk("%02X\n", tmpStr[j]);
							} else {
								printk("%02X ", tmpStr[j]);
							}
						}
#endif
						tmpStr[0] += CurrentLength;
					}
				}
			} else {
				// Read eeprom
				tmpStr[0] = EcI2cData.WriteBuffer[0];
				RWCount = EcI2cData.length;
				if (EcI2cData.length <= I2C_SMBUS_BLOCK_MAX) {
					ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, EcI2cData.WriteLength, 
							EcI2cData.WriteBuffer, EcI2cData.ReadLength, EcI2cData.ReadBuffer);
				} else {
					CurrentLength = I2C_SMBUS_BLOCK_MAX;
					for (i = 0; i < EcI2cData.length; i+=I2C_SMBUS_BLOCK_MAX) {
						CurrentLength = (EcI2cData.length - i) >= I2C_SMBUS_BLOCK_MAX ? I2C_SMBUS_BLOCK_MAX : (EcI2cData.length - i);
						ec_i2c_do_transfer(EcI2cData.DeviceID, EcI2cData.Address, EcI2cData.WriteLength, 
								tmpStr, CurrentLength, &(EcI2cData.ReadBuffer[i]));
#ifdef QATEST
						for (j = 0; j < CurrentLength; j++) {
							if (j == CurrentLength - 1) {
								printk("%02X\n", EcI2cData.ReadBuffer[i+j]);
							} else {
								printk("%02X ", EcI2cData.ReadBuffer[i+j]);
							}
						}
#endif
						tmpStr[0] += CurrentLength;
					}
				}
			}

			// 5. Start Transfter
			if (EcI2cData.ReadLength > 0 || EcI2cData.length > 0) {
				// 5. Retrieve an I/O request's output buffer.
				if (copy_to_user((ec_i2c_data __user *)arg, &EcI2cData, sizeof(ec_i2c_data))) {
					printk(KERN_ERR "Error: copy_to_user() \n");
					return -EFAULT;
				}
			} 
		}
		break;
	}

	case IOCTL_COMMON_EC_BARCODE_EEPROM_SET_PROTECT:
		// 1. Retrieve an I/O request's input buffer.
		if (copy_from_user(&EepromProtData, (ec_barcode_eeprom_protect_data __user *)arg, 
					sizeof(ec_barcode_eeprom_protect_data))) {
			printk(KERN_ERR "Error: copy_from_user() \n");
			return -EFAULT;
		} else {
			// 2. Check Input buffer length
			// 3. Check slave address
			if (EepromProtData.SlaveAddress != EC_BARCODE_EEPROM0_BANK(0) &&
					EepromProtData.SlaveAddress != EC_BARCODE_EEPROM0_BANK(2) &&
					EepromProtData.SlaveAddress != EC_BARCODE_EEPROM0_BANK(3) &&
					EepromProtData.SlaveAddress != EC_BARCODE_EEPROM1_BANK(0) &&
					EepromProtData.SlaveAddress != EC_BARCODE_EEPROM1_BANK(2) &&
					EepromProtData.SlaveAddress != EC_BARCODE_EEPROM1_BANK(3) ) { 
				printk(KERN_ERR "Check bank(slave address: %u) error! \n", EepromProtData.SlaveAddress);
				return -EFAULT;
			}

			// 4. Set Protect/UnProtect
			if (ec_barcode_eeprom_set_protect(&EepromProtData)) {
				printk(KERN_ERR "Error: ec_barcode_eeprom_set_protect() \n");
				return -EFAULT;
			}
		}
		break;

	case IOCTL_COMMON_EC_BARCODE_EEPROM_GET_CONFIG:
		// 1. Retrieve an I/O request's input buffer.
		if (copy_from_user(&EepromConfig, (eeprom_config __user *)arg, sizeof(eeprom_config))) {
			printk(KERN_ERR "Error: copy_from_user() \n");
			return -EFAULT;
		} else {
			// 2. Depends on ID to get value
			switch(EepromConfig.uID) {
			case EC_ID_BARCODE_PAGE_SIZE:
				EepromConfig.uValue = EC_BARCODE_EEPROM_PAGE_SIZE;
				break;
			case EC_ID_BARCODE_EEPROM0_LOCK_STATUS0:
			case EC_ID_BARCODE_EEPROM0_LOCK_STATUS1:
			case EC_ID_BARCODE_EEPROM0_LOCK_STATUS2:
			case EC_ID_BARCODE_EEPROM1_LOCK_STATUS0:
			case EC_ID_BARCODE_EEPROM1_LOCK_STATUS1:
			case EC_ID_BARCODE_EEPROM1_LOCK_STATUS2:
				if (ec_barcode_eeprom_check_is_locked((unsigned short)(EepromConfig.uID), &(EepromConfig.uValue)) < 0) {
					printk(KERN_ERR "Error: copy_to_user() \n");
					return -EACCES;
				}
				break;
			case EC_ID_BARCODE_PSW_MAX_LEN:
				EepromConfig.uValue = EC_LOCK_KEY_SIZE;
				break;
			default:
				return -EINVAL;
			}

			// 3. Retrieve an I/O request's output buffer.
			if (copy_to_user((eeprom_config __user *)arg, &EepromConfig, sizeof(eeprom_config))) {
				printk(KERN_ERR "Error: copy_to_user() \n");
				return -EFAULT;
			} 
		}
		break;

	default:
		return -EINVAL;
	}

	return 0;
}


static int ec_eeprom_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ec_eeprom_release(struct inode *inode, struct file *file)
{
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ec_eeprom_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
#else
static ssize_t ec_eeprom_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
#endif
{
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ec_eeprom_read(struct file *file, char *buf, size_t count, loff_t *ptr)
#else
static ssize_t ec_eeprom_read(struct file *file, char *buf, size_t count, loff_t *ptr)
#endif
{
	return 0;
}

static struct file_operations ec_eeprom_fops = {
	owner:		THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	ioctl:		ec_eeprom_ioctl,
#else
	unlocked_ioctl: ec_eeprom_ioctl,
#endif
	read:		ec_eeprom_read,
	write:		ec_eeprom_write,
	open:		ec_eeprom_open,
	release:	ec_eeprom_release,
};

void ec_eeprom_cleanup(void)
{
	device_destroy(eeprom_class, MKDEV(major, 0));
	class_destroy(eeprom_class);
	unregister_chrdev( major, "adveeprom" );
	printk("Advantech EC eeprom exit!\n");
}

int ec_eeprom_init (void)
{
	if ((major = register_chrdev(0, "adveeprom", &ec_eeprom_fops)) < 0) {
		printk("register chrdev failed!\n");
		return -ENODEV;
	}

	eeprom_class = class_create(THIS_MODULE, "adveeprom");
	if (IS_ERR(eeprom_class)) {
		printk(KERN_ERR "Error creating eeprom class.\n");
		return -1;
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
	device_create(eeprom_class, NULL, MKDEV(major, 0), NULL, "adveeprom");
#else
	class_device_create(eeprom_class, NULL, MKDEV(major, 0), NULL, "adveeprom");
#endif

	adv_get_all_hw_pin_num();

	printk("=====================================================\n");
	printk("     Advantech ec eeprom driver V%s [%s]\n", 
			ADVANTECH_EC_EEPROM_VER, ADVANTECH_EC_EEPROM_DATE);
	printk("=====================================================\n");
	return 0;
}

module_init( ec_eeprom_init );
module_exit( ec_eeprom_cleanup );

MODULE_DESCRIPTION("Advantech EC EEPROM Driver.");
MODULE_LICENSE("GPL");
