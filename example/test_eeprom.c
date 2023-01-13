#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

// magic number refer to linux-source-4.4.0/Documentation/ioctl/ioctl-number.txt
#define EEPROM_MAGIC	'g'
#define IOCTL_COMMON_EC_I2C_GET_CONFIG				_IO(EEPROM_MAGIC, 0x30)
#define IOCTL_COMMON_EC_I2C_TRANSFER				_IOWR(EEPROM_MAGIC, 0x31, int)
#define IOCTL_COMMON_EC_I2C_GET_FREQ				_IO(EEPROM_MAGIC, 0x32)
#define IOCTL_COMMON_EC_I2C_SET_FREQ				_IO(EEPROM_MAGIC, 0x33)
#define IOCTL_COMMON_EC_BARCODE_EEPROM_GET_CONFIG	_IOWR(EEPROM_MAGIC, 0x34, int)
#define IOCTL_COMMON_EC_BARCODE_EEPROM_SET_PROTECT	_IOWR(EEPROM_MAGIC, 0x35, int)

#define EC_DID_SMBOEM0          0x28	// 0x28	SMBOEM0,           	SMBUS/I2C. Smbus channel 0, EEPROM
#define EC_DID_SMBOEM1          0x29	// 0x29	SMBOEM1,           	SMBUS/I2C. Smbus channel 1, External
#define EC_DID_SMBOEM2          0x2A	// 0x2a	SMBOEM2,           	SMBUS/I2C. Smbus channel 2, Thermal
#define EC_DID_SMBEEPROM        0x2B	// 0x2b	SMBEEPROM,         	SMBUS EEPROM
#define EC_DID_I2COEM1			0x2F	// 0x2F	I2COEM1,           	SMBUS/I2C. I2C oem channel 1
#define I2C_SMBUS_BLOCK_MAX     32      // As specified in SMBus standard
#define I2C_SMBUS_WRITE_MAX     16      // As specified in SMBus standard
#define I2C_SMBUS_USE_MAX     	257     // As specified in SMBus standard

#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS0		0xA0
#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS1		0xA4
#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS2		0xA6
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS0		0xA8
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS1		0xAC
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS2		0xAA
#define EC_ID_BARCODE_PAGE_SIZE			0x00000000
#define EC_ID_BARCODE_PSW_MAX_LEN		0x00010001
#define EC_LOCK_KEY_SIZE			8		// Maximum length
#define MAX_PASSWORD_LENGTH			8
#define CHECK_ELEMENT_NUMBER		9

#define MAX_EEPROM_OFFSET 		0xFF
#define MAX_SMBUS_LENGTH 		32

#define CLEAR()			printf("\033[2J")
#define RESET_CURSOR()	printf("\033[H")

#ifndef uchar
	#define uchar unsigned char
#else
	#undef uchar
	#define uchar unsigned char
#endif
#ifndef ushort
	#define ushort unsigned short
#else
	#undef ushort
	#define ushort unsigned short
#endif
#ifndef uint
	#define uint unsigned int
#else
	#undef uint
	#define uint unsigned int
#endif
#ifndef ulong
	#define ulong unsigned long
#else
	#undef ulong
	#define ulong unsigned long
#endif

//#define __DEBUG
#ifdef __DEBUG
#define DEBUG(format, ...)	printf("%s:%d => "format"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(info, ...)
#endif

typedef struct _ec_barcode_eeprom_protect_data {
	ushort	SlaveAddress;	// The encoded Slave device.
	uchar	Password[EC_LOCK_KEY_SIZE];	// The password buffer.
	uchar	Protected;		// 0->unprotect, 1->protect
	uchar	PwdLength;		// The password length.
} ec_barcode_eeprom_protect_data, *pec_barcode_eeprom_protect_data;

typedef struct _ec_i2c_data {
	uchar	DeviceID;
	ushort	Address;
	uchar	ReadLength;
	uchar	ReadBuffer[I2C_SMBUS_USE_MAX];
	uchar	WriteLength;
	uchar	WriteBuffer[I2C_SMBUS_USE_MAX];
	uint	length;
	uchar	isRead;
} ec_i2c_data, *pec_i2c_data;

typedef struct _eeprom_config {
	ulong uID;
	ulong uValue;
} eeprom_config, *peeprom_config;

//#define QATEST
#ifdef QATEST

#if 0
static uchar testDataText[256] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};
#else
static uchar testDataText[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 
};
#endif

static uchar testDataZero[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static uchar testDataOne[256] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

void eeprom_write_test(int fd, ulong slaveaddr, const uchar *datastr);

#endif	// #define QATEST

void usage(const char *filename);
int  input_uint(ulong *psetVal, uchar base, ulong maxVal, ulong minVal);
int  wait_enter(void);
bool eeprom_set_protect(int fd, ulong SalveAddr, bool bProtect, uchar *pBuffer, uint BufLen);
bool get_eeprom_config(int fd, ulong uID, ulong *puValue);
void eeprom_get_config(int label, uint *address, int *cmd, int *length);
bool i2c_transfer(int fd, uchar DeviceID, uchar Addr, uint length, uchar *pWBuffer, ulong WriteLen, 
		uchar *pRBuffer, ulong ReadLen, uchar isRead);
bool i2c_read_transfer(int fd, ulong Addr, ulong Cmd, uchar *pBuffer, ulong ReadLen);
bool i2c_write_transfer(int fd, ulong Addr, ulong Cmd, uchar *pBuffer, ulong BufLen);
void eeprom_read(int fd, ulong slaveaddr);
void eeprom_write(int fd, ulong slaveaddr);
int  eeprom_lock(int fd, ulong slaveaddr);
int  eeprom_unlock(int fd, ulong slaveaddr);
bool eeprom_get_lock_status(int fd, ulong slaveaddr);

int main(int argc, const char *argv[])
{
	char user_input[32] = {'\0'};
	int is_exit = 0;
	ulong slaveaddr = 0;
	int op = 0, fd = 0;

	fd = open("/dev/adveeprom", O_WRONLY);
	if (fd == -1) {
		printf("Error: EC EEPROM device not enabled. \n");
		return -1;
	}

	usage(argv[0]);

	while (!is_exit)
	{
		memset(user_input, 0, sizeof(user_input));
		slaveaddr = 0;
		op = 0;
		if (!fgets(user_input, (sizeof(user_input) - 1), stdin)) {
			printf("Input error ! \n");
			wait_enter();
			usage(argv[0]);
			continue;
		}

		op = atoi((const char *)user_input);
#ifdef QATEST
		if (op > 0 && op < 9) {
#else
		if (op > 0 && op < 6) {
#endif
			printf("7-bit Slave Address(0xA0,0xA4,0xA6): 0x");
			if (input_uint(&slaveaddr, 16, 0xFF, 0x00) || 
					(slaveaddr != 0xA0 && slaveaddr != 0xA4 && slaveaddr != 0xA6)) {
				wait_enter();
				printf("Invalid input slave address! \n");
				wait_enter();
				usage(argv[0]);
				continue;
			}
			DEBUG("Slave Address: 0x%lX", slaveaddr);
		}

		switch (op)
		{
		case 0:
			DEBUG("Exit Now.");
			is_exit = 1;
			break;
		case 1:
			DEBUG("Eeprom Read.");
			eeprom_read(fd, slaveaddr);
			wait_enter();
			break;
		case 2:
			DEBUG("Eeprom Write.");
			eeprom_write(fd, slaveaddr);
			wait_enter();
			break;
		case 3:
			DEBUG("Eeprom Lock.");
			eeprom_lock(fd, slaveaddr);
			wait_enter();
			break;
		case 4:
			DEBUG("Eeprom unlock.");
			eeprom_unlock(fd, slaveaddr);
			wait_enter();
			break;
		case 5:
			DEBUG("Get lock status.");
			eeprom_get_lock_status(fd, slaveaddr);
			wait_enter();
			break;
#ifdef QATEST
		case 6:
			DEBUG("Eeprom Write 0xFF.");
			eeprom_write_test(fd, slaveaddr, testDataOne);
			wait_enter();
			break;
		case 7:
			DEBUG("Eeprom Write 0x00.");
			eeprom_write_test(fd, slaveaddr, testDataZero);
			wait_enter();
			break;
		case 8:
			DEBUG("Eeprom Write 0x00 ~ 0xFF.");
			eeprom_write_test(fd, slaveaddr, testDataText);
			wait_enter();
			break;
#endif	// #define QATEST
		default:
			printf("Unknown choice ! \n");
			wait_enter();
			usage(argv[0]);
			break;
		}
	}
	
	close(fd);
	return 0;
}

void usage(const char *filename)
{
	CLEAR();
	RESET_CURSOR();
	printf("#Usage: %s \n", filename);
	printf("\t0) Exit\n");
	printf("\t1) Read\n");
	printf("\t2) Write\n");	
	printf("\t3) Lock\n");	
	printf("\t4) Unlock\n");
	printf("\t5) Get lock status\n");
#ifdef QATEST
	printf("\t6) Write 0xFF \n");
	printf("\t7) Write 0x00 \n");
	printf("\t8) Write 0x00 ~ 0xFF \n");
#endif	// #define QATEST
	printf("Enter your choice: ");
}

int input_uint(ulong *psetVal, uchar base, 
		ulong maxVal, ulong minVal)
{
	int ret = 0;

	if (psetVal == NULL) {
		return -1;
	}

	switch (base)
	{
	case 8:
		ret = scanf("%o", (uint *)psetVal);
		DEBUG("o-base: 0x%lx", *psetVal);
		break;
	case 10:
		ret = scanf("%u", (uint *)psetVal);
		DEBUG("u-base: 0x%lx", *psetVal);
		break;
	case 16:
		ret = scanf("%x", (uint *)psetVal);
		DEBUG("x-base: 0x%lx", *psetVal);
		break;
	default:
		return -1;
	}

	if (ret <= 0) {
		DEBUG("Scanf failed !");
		return -1;
	}

	if (*psetVal > maxVal || *psetVal < minVal) {
		DEBUG("Val:0x%lx, maxVal:0x%lx, minVal:0x%lx", *psetVal, maxVal, minVal);
		DEBUG("Value out of range!");
		return -1;
	}

	return 0;
}

int wait_enter(void)
{
	int c, i = 0;

	while ((c = getchar()) != '\n' && c != EOF) {
		i++;
	}

	return i; /* number of characters thrown off */
}

bool eeprom_set_protect(int fd, ulong SalveAddr, bool bProtect, uchar *pBuffer, uint BufLen)
{
	ulong retSize = 0;
	ulong inputSize = 0;
	pec_barcode_eeprom_protect_data pprotectData = NULL;

	DEBUG(">>>>> Enter >>>>>");

	pprotectData = (pec_barcode_eeprom_protect_data) malloc(sizeof(ec_barcode_eeprom_protect_data));
	if (pprotectData == NULL) {
		printf("Error: malloc error (size: %lu) \n", sizeof(ec_barcode_eeprom_protect_data));
		return false;
	}
	memset(pprotectData, 0, sizeof(ec_barcode_eeprom_protect_data));

	pprotectData->Protected = bProtect ? 1 : 0;
	pprotectData->PwdLength = BufLen;
	pprotectData->SlaveAddress = (ushort)SalveAddr;
	strncpy(pprotectData->Password, pBuffer, EC_LOCK_KEY_SIZE);
	DEBUG("pprotectData->Protected = %d", pprotectData->Protected);
	DEBUG("pprotectData->PwdLength = %d", pprotectData->PwdLength);
	DEBUG("pprotectData->Password = %s", pprotectData->Password);

	if (ioctl(fd, IOCTL_COMMON_EC_BARCODE_EEPROM_SET_PROTECT, pprotectData)) {
		printf("Error: failed to call IOCTL_COMMON_EC_BARCODE_EEPROM_SET_PROTECT \n");
		free(pprotectData);
		return false;
	}

	DEBUG("<<<<< Exit <<<<<");

	free(pprotectData);
	return true;
}

bool get_eeprom_config(int fd, ulong uID, ulong *puValue)
{
	DEBUG(">>>>> Enter >>>>>");
	peeprom_config pEepromConfig = NULL;

	pEepromConfig = (peeprom_config) malloc(sizeof(eeprom_config));
	if (pEepromConfig == NULL) {
		printf("Error: malloc error (size: %lu) \n", sizeof(eeprom_config));
		return false;
	}
	pEepromConfig->uID = uID;
	pEepromConfig->uValue = 0;

	if (puValue == NULL) {
		printf("Error: puValue = NULL \n");
		return false;
	}

	if (ioctl(fd, IOCTL_COMMON_EC_BARCODE_EEPROM_GET_CONFIG, pEepromConfig)) {
		printf("Error: failed to call IOCTL_COMMON_EC_BARCODE_EEPROM_GET_CONFIG \n");
		free(pEepromConfig);
		return false;
	}
	*puValue = pEepromConfig->uValue;

	DEBUG("<<<<< Exit <<<<<");

	free(pEepromConfig);
	return true;
}

void eeprom_get_config(int label, uint *address, int *cmd, int *length)
{
	DEBUG("Get config start label = %d \n", label);

	switch(label)
	{
	case 1: //Ordertext PC
		*address = 0xA4;
		*cmd = 0;
		*length = 40;
		break;
	case 2: //Ordernumber PC
		*address = 0xA4;
		*cmd = 40;
		*length = 10;
		break;
	case 3: //Index PC
		*address = 0xA4;
		*cmd = 50;
		*length = 3;
		break;
	case 4: //Serialnumber PC
		*address = 0xA4;
		*cmd = 53;
		*length = 15;
		break;
	case 5: //Ordertext? OS
		*address = 0xA4;
		*cmd = 68;
		*length = 40;
		break;
	case 6: //Ordernumber OS
		*address = 0xA4;
		*cmd = 108;
		*length = 10;
		break;
	case 7: //Version OS
		*address = 0xA4;
		*cmd = 118;
		*length = 7;
		break;
	case 8: //User #1
		*address = 0xA6;
		*cmd = 0;
		*length = 128;
		break;
	case 9: //User #2
		*address = 0xA6;
		*cmd = 128;
		*length = 128;
		break;
	case 99: //Manufacturing Date
		*address = 0xA4;
		*cmd = 248;
		*length = 8;
		break;
	default:
		break;
	}		
}

bool i2c_transfer(int fd, uchar DeviceID, uchar Addr, uint length, uchar *pWBuffer, 
		ulong WriteLen, uchar *pRBuffer, ulong ReadLen, uchar readFlag) 
{
	ulong retSize = 0;
	pec_i2c_data pi2cData = NULL;

	DEBUG("<<<<< Enter, DeviceID:0x%02X <<<<<", DeviceID);

	if (WriteLen > 0 && NULL == pWBuffer) {
		return false;
	}
	if (ReadLen > 0 && NULL == pRBuffer) {
		return false;
	}

	pi2cData = (pec_i2c_data) malloc(sizeof(ec_i2c_data));
	if (pi2cData == NULL) {
		printf("Error: malloc error (size: %lu) \n", sizeof(ec_i2c_data));
		return false;
	}
	memset(pi2cData, 0, sizeof(ec_i2c_data));

	pi2cData->DeviceID = DeviceID;
	pi2cData->Address = Addr;
	pi2cData->length = length;
	pi2cData->isRead = readFlag;

	if (WriteLen > 0) {
		memcpy(pi2cData->WriteBuffer, pWBuffer, WriteLen);
		pi2cData->WriteLength = (uchar)WriteLen;
	}
	pi2cData->ReadLength = (uchar)ReadLen;

	DEBUG("[debug] >> %s:%d \n", __func__, __LINE__);
	DEBUG("[debug] >> pi2cData->length:%d \n", pi2cData->length);

	DEBUG("[debug] >> ioctl_common_ec_i2c_transfer:0x%lX\n", IOCTL_COMMON_EC_I2C_TRANSFER);

	if (ioctl(fd, IOCTL_COMMON_EC_I2C_TRANSFER, pi2cData)) {
		printf("Error: failed to call IOCTL_COMMON_EC_I2C_TRANSFER \n");
		return false;
	}

	if (ReadLen > 0) {
		memcpy(pRBuffer, pi2cData->ReadBuffer, ReadLen);
	}
	DEBUG("<<<<< Exit, DeviceID:0x%02X <<<<<", DeviceID);

	return true;
}

bool i2c_read_transfer(int fd, ulong Addr, ulong Cmd, uchar *pBuffer, ulong ReadLen)
{
	bool status = false;
	ulong CmdLen;
	uchar wBuf[8] = {0};
	uchar DeviceID = 0xFF;
	uint length = (uint)ReadLen;

	DEBUG("[debug] >> %s:%d \n", __func__, __LINE__);
	DEBUG("[debug] >> pi2cData->length:%d \n", length);

	DEBUG(">>>>> Enter >>>>>");

	if (pBuffer == NULL || ReadLen == 0) {
		return false;
	}

	DeviceID = EC_DID_SMBEEPROM;
	CmdLen = 1;
	wBuf[0] = (uchar)Cmd;
	status = i2c_transfer(fd, DeviceID, (uchar)Addr, length, wBuf, CmdLen, pBuffer, ReadLen, 1);

	DEBUG("<<<<< Exit <<<<<");

	return status;
}

bool i2c_write_transfer(int fd, ulong Addr, ulong Cmd, uchar *pBuffer, ulong BufLen)
{
	bool status = false;
	ulong CmdLen;
	uchar combineW[512] = {0};
	uchar DeviceID = EC_DID_SMBEEPROM;
	uint length = (uint)BufLen;

	DEBUG("[debug] >> %s:%d \n", __func__, __LINE__);
	DEBUG("[debug] >> pi2cData->length:%d \n", length);

	DEBUG(">>>>> Enter >>>>>");

	if (pBuffer == NULL || BufLen == 0) {
		return false;
	}

	if (DeviceID == EC_DID_SMBEEPROM && Addr == 0xA2 && Cmd < 0xC0) {
		return false;
	}

	CmdLen = 1;
	combineW[0] = (uchar)Cmd;
	memcpy((combineW + CmdLen), pBuffer, BufLen);
	status = i2c_transfer(fd, DeviceID, (uchar)Addr, length, combineW, (BufLen + CmdLen), NULL, 0, 0);

	DEBUG("<<<<< Exit <<<<<");

	return status;
}

int input_byte_sequence(uchar *pbuffer, ulong length, uchar base, 
		uchar maxVal, uchar minVal)
{
	int pass = -1;
	ulong tmp_u32 = 0, i = 0;

	if (pbuffer == NULL || length == 0) {
		return -1;
	}

	for (i = 0; i < length; i++) {
		switch (base)
		{
		case 8:
			pass = scanf("%o", (uint *)&tmp_u32);
			break;
		case 10:
			pass = scanf("%u", (uint *)&tmp_u32);
			break;	
		case 16:
			pass = scanf("%x", (uint *)&tmp_u32);
			break;
		default:			
			return -1;
		}

		if (pass <= 0) {
			break;
		}
		if (tmp_u32 > maxVal || tmp_u32 < minVal) {
			wait_enter();
			return -1;
		}

		pbuffer[i] = (uchar)tmp_u32;
	}

	if (wait_enter()) {
		return -1;
	}
	if (pass <= 0) {
		return -1;
	}

	return 0;
}

void eeprom_read(int fd, ulong slaveaddr)
{
	ulong offset = 0, length = 0, maxLength = 0;
	ulong ResultLen = 0;
	ulong retSize = 0, i = 0;
	uchar *pEepromData = NULL;
	int ret = 0;

	printf("\nRead:\n");

	do {
		if(ret != 0) {
			return;
		}
		printf("\nOffset (0x00 to 0x%02X): 0x", MAX_EEPROM_OFFSET);
	} while ((ret = input_uint(&offset, 16, MAX_EEPROM_OFFSET, 0)) != 0);

	maxLength = I2C_SMBUS_USE_MAX - 1;
	do {
		if(ret != 0) {
			return;
		}
		printf("\nLength (1 to %lu): ", maxLength);
	} while ((ret = input_uint(&length, 10, maxLength, 1)) != 0);

	DEBUG("[debug] >> %s:%d \n", __func__, __LINE__);
	DEBUG("[debug] >> pi2cData->length:%d \n", (uint)length);

	ResultLen = length * sizeof(uchar);
	pEepromData = (uchar *)malloc(ResultLen);
	if (pEepromData == NULL) {
		printf("Error: malloc error (size: %ld) \n", ResultLen);
		return;
	}
	memset(pEepromData, 0, ResultLen);

	if (!i2c_read_transfer(fd, slaveaddr, offset, pEepromData, ResultLen)) {
		free(pEepromData);
		printf("Error: I2C read transfer failed. \n");
		return;
	}

	printf("Result: (HEX)\n");
	for (i = 0; i < length; i++) {
		if (i == length - 1) {
			printf("%02X\n", pEepromData[i]);
		} else {
			printf("%02X ", pEepromData[i]);
		}
	}

	free(pEepromData);
	printf("i2c_read_transfer succeed.\n");
}

void eeprom_write(int fd, ulong slaveaddr)
{
	ulong offset = 0;
	ulong length = 0;
	ulong ResultLen = 0;
	ulong retSize = 0, i = 0;
	ulong blockSize = 0;
	uchar data[512] = {0};
	int ret = 0;

	printf("\nWrite:\n");

	do {
		printf("\nOffset (0x00 to 0x%02X): 0x", MAX_EEPROM_OFFSET);
		if(ret != 0) {
			return;
		}
	} while ((ret = input_uint(&offset, 16, MAX_EEPROM_OFFSET, 0)) != 0);

	blockSize = I2C_SMBUS_USE_MAX - 1;
	do {
		if(ret != 0) {
			return;
		}
		printf("\nLength (1 to %lu): ", blockSize);
	} while ((ret = input_uint(&length, 10, blockSize, 1)) != 0);

	printf("\nInput %lu-byte data (Hex):\n ", length);
	if (input_byte_sequence(data, length, 16, 0xFF, 0) != 0) {
		printf("Input invalid value.\n");
		return;
	}

	printf("\nWrite Data (Hex):\n");
	for (i = 0; i < length; i++) {
		printf(" %02X", data[i]);
	}
	printf("\n\n");

	if(!i2c_write_transfer(fd, slaveaddr, offset, data, length)) {
		printf("Error: I2C write transfer failed. \n");
		return;
	}

	printf("i2c_write_transfer succeed.\n");
}

#ifdef QATEST
void eeprom_write_test(int fd, ulong slaveaddr, const uchar *datastr)
{
	ulong offset = 0, length = 0;
	ulong ResultLen = 0;
	ulong retSize = 0, i = 0;
	ulong blockSize = 0;
	uchar data[512] = {0};
	int ret = 0;

	printf("\nWrite:\n");

	do {
		if(ret != 0) {
			return;
		}
		printf("\nOffset (0x00 to 0x%02X): 0x", MAX_EEPROM_OFFSET - 1);
	} while ((ret = input_uint(&offset, 16, MAX_EEPROM_OFFSET - 1, 0)) != 0);

	blockSize = I2C_SMBUS_USE_MAX - 1;
	do {
		if(ret != 0) {
			return;
		}
		printf("\nLength (1 to %lu): ", blockSize);
	} while ((ret = input_uint(&length, 10, blockSize, 1)) != 0);

	printf("\nInput %lu-byte data (Hex): use default value ...\n ", length);
	memcpy(data, datastr, (length * sizeof(uchar)));

	printf("\nWrite Data (Hex):\n");
	for (i = 0; i < length; i++) {
		printf("%02X ", data[i]);
	}
	printf("\n\n");

	if(!i2c_write_transfer(fd, slaveaddr, offset, data, length)) {
		printf("Error: I2C write transfer failed. \n");
		return;
	}

	printf("i2c_write_transfer succeed.\n");
}
#endif	// #define QATEST

int eeprom_lock(int fd, ulong slaveaddr)
{
	int length;
	uchar passwd[32] = {0};
	int maxLength = 8;

	printf("\nLock: \n\n");
	printf("Note: maximum length of password is %u-word\n", maxLength);
	printf("Type new password: ");

	if (scanf("%s", passwd) <= 0) {
		printf("Error: input invalid value. \n");
		wait_enter();
		return -1;
	}
	wait_enter();
	printf("passwd: %s \n", passwd);

	length = (int)strlen((char *)passwd);
	if (length > maxLength) {
		printf("Error: too many wards. \n");
		return -1;
	}

	if (!eeprom_set_protect(fd, slaveaddr, true, passwd, length)) {
		printf("Error: Set eeprom protect falied. \n");
		return -1;	
	}

	printf("New password: %s\n", passwd);
	return 0;
}

int eeprom_unlock(int fd, ulong slaveaddr)
{
	int length;
	uchar passwd[32] = {0};
	int maxLength = 8;

	printf("\nUnLock: \n\n");
	printf("Note: maximum length of password is %u-word\n", maxLength);
	printf("Type password: ");

	if (scanf("%s", passwd) <= 0) {
		printf("Error: input invalid value. \n");
		wait_enter();
		return -1;
	}
	wait_enter();
	printf("passwd: %s \n", passwd);

	length = (int)strlen((char *)passwd);
	if (length > maxLength) {
		printf("Error: too many wards. \n");
		return -1;
	}

	if (!eeprom_set_protect(fd, slaveaddr, false, passwd, length)) {
		printf("Error: Set eeprom protect falied. \n");
		return -1;	
	}

	return 0;
}

bool eeprom_get_lock_status(int fd, ulong slaveaddr)
{
	ulong status = 0;

	printf("Lock Status of 0x%lX: ", slaveaddr);

	if(!get_eeprom_config(fd, slaveaddr, &status)) {
		printf("Error: get eeprom config failed. \n");
		return false;
	}
	printf("Status: %s \n", status ? "LOCKED":"UNLOCKED");
	return true;
}

