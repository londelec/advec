#ifndef _ec_EEPROM_DRV_H
#define _ec_EEPROM_DRV_H

/* Information Data Definition (EEPROM), Slave Address : 0xA2
|  Offset   |  Name             |  Description
----------------------------------------------------------------------------------------
| 0x00~0x03 | Power On counter  |  Every time power on system, 
|           |                   |  BIOS will send command 0x2F to step power on counter. 
|           |                   |  0x00-0x03 : MSB-LSB 
-----------------------------------------------------------------------------------------
| 0x04~0x07 | Run time counter  |  When system is running, the counter will step one every hour 
|           |                   |  and EC will write back to eeprom automatically. 
|           |                   |  0x00-0x03 : MSB-LSB 
----------------------------------------------------------------------------------------
| 0x08~0x0F | Byte protect      |  These 64 bits can protect oem eeprom Bank1 0xC0~0xFF. 
|           |                   |  Each one bit can protect one byte from writing. 0x08 to 0x0F is LSB to MSB. 
|           |                   |  For example, 
|           |                   |  if you want protect bank1 offset 0xC8, you should write 1 to 0x09 bit0. 
|           |                   |  If you want to protect bank1 offset 0xD5, you should write 1 to 0x0A bit5. 
----------------------------------------------------------------------------------------
| 0x10~0x1C | Fan0 variable     |  Mapping Fan control HW Ram Address offset 0x00~0x0C 
----------------------------------------------------------------------------------------
| 0x1D~0x29 | Fan1 variable     |  Mapping Fan control HW Ram Address offset 0x00~0x0C 
----------------------------------------------------------------------------------------
| 0x2A~0x36 | Fan2 variable     |  Mapping Fan control HW Ram Address offset 0x00~0x0C 
----------------------------------------------------------------------------------------
| 0x37~0x3B | Thermal0 variable |  Mapping SMBUS thermal source HW ram offset 0x00~0x04 
----------------------------------------------------------------------------------------
| 0x3C~0x40 | Thermal1 variable |  Mapping SMBUS thermal source HW ram offset 0x00~0x04 
----------------------------------------------------------------------------------------
| 0x41~0x45 | Thermal2 variable |  Mapping SMBUS thermal source HW ram offset 0x00~0x04 
----------------------------------------------------------------------------------------
| 0x46~0x4A | Thermal3 variable |  Mapping SMBUS thermal source HW ram offset 0x00~0x04 
----------------------------------------------------------------------------------------
| 0x4B~0x4D | AltGPIO0 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x4E~0x50 | AltGPIO1 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x51~0x53 | AltGPIO2 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x54~0x56 | AltGPIO3 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x57~0x59 | AltGPIO4 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x5A~0x5C | AltGPIO5 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x5D~0x5F | AltGPIO6 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x60~0x62 | AltGPIO7 variable |  Mapping Oem Alt GPIO default table offset 0x00~0x02 
----------------------------------------------------------------------------------------
| 0x63~0x6C | Brightness Table  |  Mapping HW Ram offset 0x37-0x40 
----------------------------------------------------------------------------------------
| 0x6D~0x76 | Speaker table     |  Mapping HW Ram offset 0x41-0x4A 
---------------------------------------------------------------------------------------- */

// |  Device Type Id.  | Chip En Addr | RW |
// | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
// |  1 |  0 |  1 |  0 |  0 |  0 |  0 | RW | (0) 0xA0
// |  1 |  0 |  1 |  0 |  0 |  0 |  1 | RW | (1) 0xA2   (Lock Key storage) offset 0xA0, 0xA8, 0xB0
// |  1 |  0 |  1 |  0 |  0 |  1 |  0 | RW | (2) 0xA4
// |  1 |  0 |  1 |  0 |  0 |  1 |  1 | RW | (3) 0xA6
// ================================================
// |  1 |  0 |  1 |  0 |  1 |  0 |  0 | RW | (0) 0xA8
// |  1 |  0 |  1 |  0 |  1 |  0 |  1 | RW | (1) 0xAA   (Lock Key storage) offsest 0xA0, 0xA8, 0xB0
// |  1 |  0 |  1 |  0 |  1 |  1 |  0 | RW | (2) 0xAC
// |  1 |  0 |  1 |  0 |  1 |  1 |  1 | RW | (3) 0xAE   

#define EC_BARCODE_EEPROM_MASK				(1 << 3)
#define EC_BARCODE_EEPROM_PAGE_SIZE			16
#define EC_BARCODE_EEPROM0_BASE				0xA0
#define EC_BARCODE_EEPROM0_BANK(N)			(EC_BARCODE_EEPROM0_BASE | (N << 1))
#define EC_BARCODE_EEPROM1_BASE				0xA8
#define EC_BARCODE_EEPROM1_BANK(N)			(EC_BARCODE_EEPROM1_BASE | (N << 1))

#define EC_BARCODE_LOCK_KEY_BASE0				EC_BARCODE_EEPROM0_BANK(1) 		// 0xA2
#define EC_BARCODE_LOCK_KEY_BASE1				EC_BARCODE_EEPROM1_BANK(1) 		// 0xAA
#define EC_BARCODE_LOCK_KEY_OFFSET_BANK(N)		(0xA0 | (N << 3)) 				// 0xA0, 0xA8, 0xB0
//#define EC_BARCODE_LOCK_KEY_OFFSET0_BANK(N)	(0xA0 | (N << 3))//(N << 3)   	// 0x00, 0x08, 0x10
//#define EC_BARCODE_LOCK_KEY_OFFSET1_BANK(N)	(EC_EEPROM_OEM_BASE | (N << 3))	// 0xC0, 0xC8, 0xD0

#define EC_DID_SMBOEM0          0x28	// 0x28	SMBOEM0,           	SMBUS/I2C. Smbus channel 0, EEPROM
#define EC_DID_SMBOEM1          0x29	// 0x29	SMBOEM1,           	SMBUS/I2C. Smbus channel 1, External
#define EC_DID_SMBOEM2          0x2A	// 0x2a	SMBOEM2,           	SMBUS/I2C. Smbus channel 2, Thermal
#define EC_DID_SMBEEPROM        0x2B	// 0x2b	SMBEEPROM,         	SMBUS EEPROM
#define EC_DID_I2COEM1			0x2F	// 0x2F	I2COEM1,           	SMBUS/I2C. I2C oem channel 1

#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS0		0xA0
#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS1		0xA4
#define EC_ID_BARCODE_EEPROM0_LOCK_STATUS2		0xA6
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS0		0xA8
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS1		0xAC
#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS2		0xAA
//#define EC_ID_BARCODE_EEPROM1_LOCK_STATUS2		0xAE
#define EC_ID_BARCODE_PAGE_SIZE			0x00000000
#define EC_ID_BARCODE_PSW_MAX_LEN		0x00010001

#define EC_MAX_DEVICE_ID_NUM	0xFF

/* EC EEPROM */
#define EC_EEPROM_INFO_BLOCK		0		// Advantech Information Data
#define EC_EEPROM_OEM_BLOCK			1		// OEM Data
#define EC_EEPROM_MAX_SIZE			0x100
#define EC_EEPROM_OEM_SIZE			0x40	// user's space is 64bytes
#define EC_EEPROM_BLOCK_SIZE		32
#define EC_EEPROM_OEM_BASE			(EC_EEPROM_MAX_SIZE - EC_EEPROM_OEM_SIZE)
#define EC_EEPROM_BYTE_PROTECT_OFFSET_COUNT		8

/* EC EEPROM Command */
#define EC_EEPROM_STATUS_READ			0x90	// Get eeprom status.
#define EC_EEPROM_WRITE_CTRL_REGISTER	0x91	// write control register
#define EC_EEPROM_RESULT_READ			0x92	// Get process result.
#define EC_EEPROM_INFO_IDX_WRITE		0x93	// Set information data index
#define EC_EEPROM_INFO_DATA_READ		0x94	// Read information data from buffer
#define EC_EEPROM_INFO_DATA_WRITE		0x95	// Write data to information buffer
#define EC_EEPROM_OEM_BANK_WRITE		0x96	// Set oem eeprom bank
#define EC_EEPROM_OEM_IDX_WRITE			0x97	// Set oem eeprom index
#define EC_EEPROM_OEM_DATA_READ			0x98	// Read oem eeprom data
#define EC_EEPROM_OEM_DATA_WRITE		0x99	// Write oem eeprom data

#define EC_INFO_DATA_BYTE_PROTECT_OFFSET 0x08  // 0x08 ~ 0x0F (64bits)
#define EC_LOCK_KEY_SIZE			8		// Maximum length
#define EC_LOCK_KEY_OFFSET			0xB8
#define EC_HWPIN_NUMBER_UNUSED		0xFF
#define EC_HW_EXTEND_RAM_ADDR_DATA(N)	((N))	// N: 0~31
#define EC_HW_EXTEND_RAM_ADDR_WCOUNT	0x20
#define I2C_SMBUS_BLOCK_MAX     	32      /* As specified in SMBus standard */
#define I2C_SMBUS_WRITE_MAX     	16      /* As specified in SMBus standard */
#define I2C_SMBUS_USE_MAX     		257     /* As specified in SMBus standard */
#define COMMON_DEVICE_BASE			FILE_DEVICE_UNKNOWN

/* SMBus/I2C */
// SMBus
#define EC_SMBUS_BLOCKCNT  			0x24
#define EC_SMBUS_PROTOCOL_ADDRESS	0x00	// Smbus transmit protocol
#define EC_SMBUS_STATUS_ADDRESS 	0x01	// 0x01		Status				SMBUS status 
#define EC_SMBUS_SLV_ADDRESS 		0x02	// 0x02		Address				SMBUS device slave address (8bitㄛbit0 must be 0)
#define EC_SMBUS_CMD_ADDRESS  		0x03	// 0x03		Command				SMBUS device command
#define EC_SMBUS_DATA_ADDRESS  		0x04	// 0x04 -0x24	Data	In read process, return data are stored in this address.
#define EC_SMBUS_DAT_OFFSET_ADDRESS(n)		(EC_SMBUS_DATA_ADDRESS + (n))	// DATA1~32, n:0~31
#define EC_SMBUS_BLOCKCNT  			0x24
#define EC_SMBUS_CHANNEL_ADDRESS	0x2B	// 0x2B		SMBUS channel selector	Select SMBUS channel. (0-4)

#define EC_MBOX_GET_SMBUS_FREQUENCY						0x34
#define EC_MBOX_SET_SMBUS_FREQUENCY						0x35

#define EC_MBOX_SMBUS_WRITE_QUICK						0x02
#define EC_MBOX_SMBUS_READ_QUICK						0x03
#define EC_MBOX_SMBUS_SEND_BYTE							0x04
#define EC_MBOX_SMBUS_RECEIVE_BYTE						0x05
#define EC_MBOX_SMBUS_WRITE_BYTE						0x06
#define EC_MBOX_SMBUS_READ_BYTE							0x07
#define EC_MBOX_SMBUS_WRITE_WORD						0x08
#define EC_MBOX_SMBUS_READ_WORD							0x09
#define EC_MBOX_SMBUS_WRITE_BLOCK						0x0A
#define EC_MBOX_SMBUS_READ_BLOCK						0x0B

// Storage
// Define in ec.h
//#define EC_MBOX_CMD_OFFSET			0x00							// Mailbox command
//#define EC_MBOX_STATUS_OFFSET		(EC_MBOX_CMD_OFFSET + 1)		// Command return status
//#define EC_MBOX_PARA_OFFSET			(EC_MBOX_CMD_OFFSET + 2)		// Parameter for command

#define EC_MBOX_DAT_OFFSET(n)		(EC_MBOX_CMD_OFFSET + 3 + (n))	// Data area. Dat00 ~ Dat2C
#define EC_MBOX_CLEAR_256_BYTES_BUFFER					0xC0
#define EC_MBOX_READ_256_BYTES_BUFFER					0xC1
#define EC_MBOX_WRITE_256_BYTES_BUFFER					0xC2
#define EC_MBOX_READ_EEPROM_DATA_FROM_256_BYTES_BUFFER	0xC3		// read from EEPROM and write into 256 bytes RAM buffer

// I2C
#define EC_MBOX_I2C_READ_WRITE							0x0E
#define EC_MBOX_I2C_WRITE_READ							0x0F
#define EC_MBOX_I2C_WRITEREAD_WITH_READ_BUFFER			0x01

//------------------------------------------------------------------------------
// The Definition of EC Common Register's Address and Value 
//------------------------------------------------------------------------------
// PM2 IO port description
// PM2 channel includes one command/status port and one data port. 
// System can use command port to send command to EC or get current port status. 
// System can send command parameter or get EC return data through data port. 
// Normally, 0x6C is command/status port and 0x68 is data port. 
// Some platforms use 0x29A as command/status port and use 0x299 as data port.
#define EC_COMMAND_PORT1		0x29A
#define EC_STATUS_PORT1			0x299

#define EC_COMMAND_PORT2		0x6C
#define EC_STATUS_PORT2			0x68

#define EC_CMD_AUTHENTICATION	0x30

/* EC MailBox I/O */
#define EC_MBOX_OFFSET_PORT		0x29E
#define EC_MBOX_DATA_PORT		0x29F

#define EC_MAX_DEVICE_ID_NUM	0xFF
#define EC_HWPIN_NUMBER_UNUSED	0xFF

//------------------------------------------------------------------------------
// The Definition of F75111 Common Register's Address and Value 
//------------------------------------------------------------------------------
/* Vendor ID */ 
#define    F75111_REG_VENDOR1			0x5D    // VENDOR ID(1) Register
#define    F75111_REG_VENDOR2			0x5E    // VENDOR ID(2) Register
#define    F75111_VENDOR1				0x19    // VENDOR ID(1)
#define    F75111_VENDOR2				0x34    // VENDOR ID(2)

/* Chip ID */ 
#define    F75111_REG_CHIPID_H			0x5A    // Chip ID High Byte Register
#define    F75111_REG_CHIPID_L			0x5B    // Chip ID Low Byte Register
#define    F75111_CHIPID_H				0x03    // Chip ID High Byte
#define    F75111_CHIPID_L				0x00    // Chip ID Low Byte

/* For IOCTL_COMMON_EEPROM_SET_PROTECT */
typedef struct _eeprom_protect_data {
	unsigned char Protected;									// 0 -> unprotect, 1 -> protect
	unsigned char PwdLength;                                    // The password length.
	unsigned char Password[EC_LOCK_KEY_SIZE];                   // The password buffer.
} eeprom_protect_data, *peeprom_protect_data;

typedef struct _ec_i2c_data {
	unsigned char	DeviceID;
	unsigned short	Address;
	unsigned char	ReadLength;
	unsigned char	ReadBuffer[I2C_SMBUS_USE_MAX];
	unsigned char	WriteLength;
	unsigned char	WriteBuffer[I2C_SMBUS_USE_MAX];
	unsigned int	length;
	unsigned char	isRead;
} ec_i2c_data, *pec_i2c_data;

typedef struct _ec_barcode_eeprom_protect_data {
    unsigned short	SlaveAddress;					            // The encoded Slave device.
	unsigned char	Password[EC_LOCK_KEY_SIZE];                 // The password buffer.
	unsigned char	Protected;									// 0 -> unprotect, 1 -> protect
	unsigned char	PwdLength;                                  // The password length.
} ec_barcode_eeprom_protect_data, *pec_barcode_eeprom_protect_data;

typedef struct _eeprom_config {
    unsigned long	uID;
	unsigned long	uValue;
} eeprom_config, *peeprom_config;

typedef enum _chipset_id {
	CHIPSET_NCT6776		= 0,
	CHIPSET_EC			= 1,
	CHIPSET_SCH311		= 2,
	CHIPSET_F75111		= 3,
	CHIPSET_MAXIMUM
} chipset_id, *pchipset_id;

// The supportted ambient light sensor light
typedef enum _ambient_lignt_chip_id {
	ALS_CHIPID_ISL29023	= 0,
	ALS_CHIPID_MAXIMUM
} ambient_lignt_chip_id, *pambient_lignt_chip_id;

// EC Mailbox
// Mailbox error code
typedef enum _MBoxErrorCode {
    _MBoxErr_Fail,             //0, fail
    _MBoxErr_Success,          //1, success
    _MBoxErr_Undefined_Item,   //2, undefined item
    _MBoxErr_ID_Undefined,     //3, undefined device id
    _MBoxErr_Pin_Type_Err,     //4, device type error
} mbox_error_code, *pmbox_error_code;

//
// The device context performs the same job as
// a WDM _DEVICE_CONTEXT in the driver frameworks
//
typedef struct _device_context {
	char 					*ProductName;				// Save the product name ( Ex: IPPC-61?2A(Haswell) )
	chipset_id				ChipsetID;					// Save the Brightness chipset ID
	ambient_lignt_chip_id	AlsChipID;					// Save the Ambient Light Sensor chipset ID
	unsigned char			uPWMHWPinNumber;			// EC uses the PWM HWPinNumber to control the PWM.
	unsigned char			uSMBusHWPinNumber;			// EC uses the SMBus HWPinNumber to control the SMBus.
	unsigned long			uCurrentBrightnessCount;	// Save the Brightness count
	unsigned long			uCurrentLightSensorCount;	// Save the Ambient Light Sensor count
	unsigned char			uBrightnessValue;			// The Brightness level or PWM value.
	unsigned char			uBrightnessMax;				// The maximum of Brightness level or PWM value.
	unsigned char			uBrightnessMin;				// The minimum of Brightness level or PWM value.
	unsigned long			uUpdateRate;				// The Light Sensor polling update rate (Unit: second).
	bool					bEnableAutoBrightness;		// Enable auto-dimming function or not.
	bool					bLoadSetting;				// Load the values from the user settings.
	void					*pvBriObject;				// Point to XXX_BRIGHTNESS_OBJECT 
	void					*pvBrightnessContext;		// Point to XXX_BRIGHTNESS_CONTEXT
	void					*pvLightObject;				// Point to XXX_LIGHTSENSOR_OBJECT
	void					*pvLightSensorContext;		// Point to XXX_LIGHTSENSOR_CONTEXT
	const struct _BRIGHTNESS_FUNC	*pBrightnessFunc;	// Point to callback function table 'BrightnessFuncTable[X]'
	const struct _LIGHT_SENSOR_FUNC	*pLightSensorFunc;	// Point to callback function table 'LightSensorFuncTable[X]'
	bool					bDeinitializeLightSensor;	// If the Light Sensor don't initialize, this value is TRUE.
} device_context, *pdevice_context;

bool ec_wait_mbox_cmd_clear(void);
bool ec_mbox_get_value(unsigned char uCmd, unsigned char *puValue);
bool ec_mbox_set_value(unsigned char uCmd, unsigned char uValue);
bool ec_mbox_read_buffer_ram(int ReadLength, unsigned char *pReadBuffer);
bool ec_mbox_clear_buffer_ram(void);

int ec_mbox_smbus_i2c(unsigned char uDeviceID, unsigned char uCommand, unsigned short uSlavAddr, 
		unsigned char uSMBCommand, unsigned char uWriteLength, unsigned char *pWriteBuffer, 
		int *pReadLength, unsigned char *pReadBuffer);
bool ec_mbox_smbus_i2c_set_data(unsigned char uCommand, unsigned char uWriteLength, 
		unsigned char *pWriteBuffer, int *pReadLength, unsigned char *pReadBuffer);
bool ec_mbox_smbus_i2c_get_data(unsigned char uCommand, int *pReadLength, unsigned char *pReadBuffer);

int ec_i2c_transfer(unsigned char uDeviceID, unsigned short uSlavAddr, 
		unsigned char uWriteLength, unsigned char *pWriteBuffer, 
		unsigned char uReadLength, unsigned char *pReadBuffer);
int ec_i2c_do_transfer(unsigned char uDeviceID, unsigned short uSlavAddr, 
		unsigned char uWriteLength, unsigned char *pWriteBuffer, unsigned char uReadLength, 
		unsigned char *pReadBuffer);
int ec_i2c_do_transfer_new(unsigned char uDeviceID, unsigned short uSlavAddr, 
		unsigned int length, unsigned char uWriteLength, unsigned char *pWriteBuffer, unsigned char uReadLength, 
		unsigned char *pReadBuffer);

bool ec_init_eeprom(unsigned char uBlock, unsigned char uOffset);
bool ec_eeprom_get_value(unsigned char uBlock, unsigned char uOffset, unsigned char *pObjOut);
bool wait_eeprom_done(void);

bool ec_eeprom_is_key_empty(unsigned char *pKey);
int ec_eeprom_get_security_key(unsigned char *pKey);
int ec_eeprom_set_security_key(unsigned char *pKey);
bool ec_eeprom_set_value(unsigned char uBlock, unsigned char uOffset, unsigned char uValue);
int ec_eeprom_set_protect(peeprom_protect_data pProtectData);

int ec_barcode_eeprom_set_protect(pec_barcode_eeprom_protect_data pProtectData);
int ec_barcode_eeprom_check_is_locked(unsigned short uSlavAddr, unsigned long *pIsLock);
int ec_barcode_eeprom_get_security_key(unsigned short uSlavAddr, unsigned char *pKey);
int ec_barcode_eeprom_set_security_key(unsigned short uSlavAddr, unsigned char *pKey);

void sha1(unsigned char hval[], const unsigned char data[], unsigned int len);
static void sha1_encrypt_decrypt(unsigned char *srcbuf, unsigned char *desbuf, 
		unsigned long datalen, unsigned char *key, unsigned long keylen);
static int adv_get_all_hw_pin_num(void);
unsigned char get_ec_hw_pin_num_by_device_id(unsigned char uDeviceID);
static __inline bool is_valid_barcode_slave_address(unsigned short SlaveAddress);
static __inline int get_barcode_lock_key_offset(unsigned short SlaveAddress, unsigned char *pOffset);

#endif	/* ec_eeprom_drv.h */
