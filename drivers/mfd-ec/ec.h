#ifndef _EC_H
#define _EC_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS
#endif
#define uchar unsigned int

#define EC_COMMAND_PORT             0x29A
#define EC_STATUS_PORT              0x299

#define EC_UDELAY_TIME              200
#define EC_MAX_TIMEOUT_COUNT        5000

#define EC_MBOX_IDERR_FAIL				0x0
#define EC_MBOX_IDERR_SUCCESS			0x1
#define EC_MBOX_IDERR_UNDEFINED_ITEM	0x2
#define EC_MBOX_IDERR_ID_UNDEFINED		0x3
#define EC_MBOX_IDERR_PIN_TYPE_ERR		0x4

#define EC_MBOX_CMD_OFFSET				0x0
#define EC_MBOX_STATUS_OFFSET			0x1
#define EC_MBOX_PARA_OFFSET				0x2
#define EC_MBOX_DAT00_OFFSET			0x3
#define EC_MBOX_DAT01_OFFSET			0x4
#define EC_MBOX_DAT02_OFFSET			0x5
#define EC_MBOX_DAT03_OFFSET			0x6
#define EC_MBOX_DAT04_OFFSET			0x7
#define EC_MBOX_READ_START_OFFSET		0xA0
#define EC_MBOX_WRITE_START_OFFSET		0x50

#define EC_MBOX_READ_STATE_OFFSET		EC_MBOX_READ_START_OFFSET+EC_MBOX_STATUS_OFFSET
#define EC_MBOX_WRITE_STATE_OFFSET		EC_MBOX_WRITE_START_OFFSET+EC_MBOX_STATUS_OFFSET
#define EC_MBOX_READ_PARA_OFFSET		EC_MBOX_READ_START_OFFSET+EC_MBOX_PARA_OFFSET
#define EC_MBOX_WRITE_PARA_OFFSET		EC_MBOX_WRITE_START_OFFSET+EC_MBOX_PARA_OFFSET


#define EC_MBOX_READ_S0_STATE_OFFSET			EC_MBOX_READ_START_OFFSET+EC_MBOX_DAT00_OFFSET
#define EC_MBOX_READ_S3_STATE_OFFSET			EC_MBOX_READ_START_OFFSET+EC_MBOX_DAT01_OFFSET
#define EC_MBOX_READ_S5_STATE_OFFSET			EC_MBOX_READ_START_OFFSET+EC_MBOX_DAT02_OFFSET
#define EC_MBOX_WRITE_S0_STATE_OFFSET			EC_MBOX_WRITE_START_OFFSET+EC_MBOX_DAT00_OFFSET
#define EC_MBOX_WRITE_S3_STATE_OFFSET			EC_MBOX_WRITE_START_OFFSET+EC_MBOX_DAT01_OFFSET
#define EC_MBOX_WRITE_S5_STATE_OFFSET			EC_MBOX_WRITE_START_OFFSET+EC_MBOX_DAT02_OFFSET
#define EC_MBOX_S_STATE_INOUT_TYPE		0x0
#define EC_MBOX_S_STATE_HIGHLOW_TYPE	0x1

#define EC_MBOX_S_STATE_INPUT			0x80
#define EC_MBOX_S_STATE_OUTPUT			0x40
#define EC_MBOX_S_STATE_PULLUP			0x04
#define EC_MBOX_S_STATE_PULLDN			0x02
#define EC_MBOX_S_STATE_HIGH			0x01

#define EC_S0_STATE						0x0
#define EC_S3_STATE						0x3
#define EC_S5_STATE						0x5

// AD command
#define EC_AD_INDEX_WRITE	 0x15    // Write AD port number into index
#define EC_AD_LSB_READ		 0x16    // Read AD LSB value from AD port
#define EC_AD_MSB_READ       0x1F    // Read AD MSB value from AD port

//voltage device id
#define EC_DID_CMOSBAT              0x50    // 0x50 ADCCMOSBAT,           CMOS coin battery voltage
#define EC_DID_CMOSBAT_X2           0x51    // 0x51 ADCCMOSBATx2,         CMOS coin battery voltage*2
#define EC_DID_CMOSBAT_X10        	0x52    // 0x52 ADCCMOSBATx10,        CMOS coin battery voltage*10
#define EC_DID_5VS0                 0x56    // 0x56 ADC5VS0,              5VS0 voltage
#define EC_DID_5VS0_X2              0x57    // 0x57 ADC5VS0x2,            5VS0 voltage*2
#define EC_DID_5VS0_X10             0x58    // 0x58 ADC5VS0x10,           5VS0 voltage*10
#define EC_DID_5VS5                 0x59    // 0x59 ADC5VS5,              5VS5 voltage
#define EC_DID_5VS5_X2              0x5A    // 0x5A ADC5VS5x2,            5VS5 voltage*2
#define EC_DID_5VS5_X10             0x5B    // 0x5B ADC5VS5x10,           5VS5 voltage*10
#define EC_DID_12VS0               	0x62    // 0x62 ADC12VS0,             12VS0 voltage
#define EC_DID_12VS0_X2             0x63    // 0x63 ADC12VS0x2,           12VS0 voltage*2
#define EC_DID_12VS0_X10            0x64    // 0x64 ADC12VS0x10,          12VS0 voltage*10
#define EC_DID_VCOREA               0x65    // 0x65 ADCVCOREEA,           CPU A core voltage
#define EC_DID_VCOREA_X2            0x66    // 0x66 ADCVCOREEAx2,         CPU A core voltage*2
#define EC_DID_VCOREA_X10           0x67    // 0x67 ADCVCOREEAx10,        CPU A core voltage*10
#define EC_DID_VCOREB               0x68    // 0x68 ADCVCOREEB,           CPU B core voltage
#define EC_DID_VCOREB_X2            0x69    // 0x69 ADCVCOREEBx2,         CPU B core voltage*2
#define EC_DID_VCOREB_X10           0x6A    // 0x6A ADCVCOREEBx10,        CPU B core voltage*10
#define EC_DID_DC                 	0x6B    // 0x6B ADCDC,                ADC. onboard voltage
#define EC_DID_DC_X2            	0x6C    // 0x6C ADCDCx2,              ADC. onboard voltage*2
#define EC_DID_DC_X10           	0x6D    // 0x6D ADCDCx10,          	  ADC. onboard voltage*10
#define EC_DID_SMBOEM0          	0x28	// 0x28	SMBOEM0,           	  SMBUS/I2C. Smbus channel 0

//Current device id
#define EC_DID_CURRENT              0x74	// 0x74	ADCCURRENT    Current

//ACPI commands
#define EC_ACPI_RAM_READ            0x80    //Read ACPI ram space
#define EC_ACPI_RAM_WRITE           0x81    //Write ACPI ram space

//Dynamic control table commands
// The table includes HW pin number,Device ID,and Pin polarity
#define EC_TBL_WRITE_ITEM           0x20    // 0x20 Write item number into index
#define EC_TBL_GET_PIN              0x21    // 0x21 Get HW pin number
#define EC_TBL_GET_DEVID            0x22    // 0x22 Get device ID
#define EC_MAX_TBL_NUM              32


//FAN
#define EC_FAN0_SPEED_ADDR     0x70
#define EC_FAN0_HW_RAM_ADDR    0xD0
#define EC_FAN0_CTL_ADDR       0xD2

#define FAN_TACHO_MODE         0x02
#define FAN0_TACHO_SOURCE      0x10    
#define FAN0_PULSE_TYPE        0x04 

//PWD commands
#define EC_PWM_WRITE_INDEX       0x17
#define EC_PWM_READ_DATA         0x18
#define EC_PWM_WRITE_DATA        0x19
#define EC_PWM_WRITE_FREQUENCY   0x1A




// LED Device ID table
#define EC_DID_LED_RUN              0xE1
#define EC_DID_LED_ERR              0xE2
#define EC_DID_LED_SYS_RECOVERY     0xE3
#define EC_DID_LED_D105_G           0xE4
#define EC_DID_LED_D106_G           0xE5
#define EC_DID_LED_D107_G           0xE6

// LED control HW ram address 0xA0-0xAF
#define EC_HWRAM_LED_BASE_ADDR          0xA0
#define EC_HWRAM_LED_PIN(N)             (EC_HWRAM_LED_BASE_ADDR + (4 * (N))) // N:0-3
#define EC_HWRAM_LED_CTRL_HIBYTE(N)     (EC_HWRAM_LED_BASE_ADDR + (4 * (N)) + 1)
#define EC_HWRAM_LED_CTRL_LOBYTE(N)     (EC_HWRAM_LED_BASE_ADDR + (4 * (N)) + 2)
#define EC_HWRAM_LED_DEVICE_ID(N)       (EC_HWRAM_LED_BASE_ADDR + (4 * (N)) + 3)

// LED control bit
#define LED_CTRL_ENABLE_BIT             (1 << 4)
#define LED_CTRL_INTCTL_BIT             (1 << 5)
#define LED_CTRL_LEDBIT_MASK            (0x03FF << 6)
#define LED_CTRL_POLARITY_MASK          (0x000F << 0)
#define LED_CTRL_INTCTL_EXTERNAL        0
#define LED_CTRL_INTCTL_INTERNAL        1

#define LED_DISABLE  0x0  // disable diagnostic LED 
#define LED_ON       0x1  // Light on
#define LED_FAST     0x3  // fast flicker
#define LED_NORMAL   0x5  // normal flick
#define LED_SLOW     0x7  // slow flicker
#define LED_MANUAL   0xF  // user define 

#define LED_CTRL_LEDBIT_DISABLE	(0x0000)
#define LED_CTRL_LEDBIT_ON		(0x03FF)
#define LED_CTRL_LEDBIT_FAST	(0x02AA)
#define LED_CTRL_LEDBIT_NORMAL	(0x0333)
#define LED_CTRL_LEDBIT_SLOW	(0x03E0)

//Get the device name
#define AMI_BIOS_NAME                               "AMIBIOS"
#define AMI_BIOS_NAME_ADDRESS                       0x000FF400
#define AMI_BIOS_NAME_LENGTH                        strlen(AMI_BIOS_NAME)
#define AMI_ADVANTECH_BOARD_ID_ADDRESS              0x000FE840
#define AMI_ADVANTECH_BOARD_ID_LENGTH               32
#define AMI_ADVANTECH_BOARD_NAME_ADDRESS            0x000FF550
#define AMI_ADVANTECH_BOARD_NAME_LENGTH             _ADVANTECH_BOARD_NAME_LENGTH
#define AMI_UEFI_ADVANTECH_BOARD_NAME_ADDRESS       0x000F0000
#define AMI_UEFI_ADVANTECH_BOARD_NAME_LENGTH        0xFFFF

// EC WDT commands
// // EC can send multistage watchdog event. System can setup watchdog event
// // independently to makeup event sequence.
#define	EC_WDT_START			    0x28	// Start watchdog
#define	EC_WDT_STOP			        0x29	// Stop watchdog
#define	EC_WDT_RESET			    0x2A	// Reset watchdog
#define	EC_WDT_BOOTTMEWDT_STOP		0x2B	// Stop boot time watchdog
#define EC_HW_RAM                   0x89
#define EC_EVENT_FLAG               0x57
#define EC_RESET_EVENT              0x04
#define EC_COMMANS_PORT_IBF_MASK    0x02
#define EC_ENABLE_DELAY_L           0x59
#define EC_ENABLE_DELAY_H           0x58
#define EC_POWER_BTN_TIME_L         0x5B
#define EC_POWER_BTN_TIME_H         0x5A
#define EC_RESET_DELAY_TIME_L       0x5F
#define EC_RESET_DELAY_TIME_H       0x5E
#define EC_PIN_DELAY_TIME_L		    0x61
#define EC_PIN_DELAY_TIME_H		    0x60
#define EC_SCI_DELAY_TIME_H		    0x62
#define EC_SCI_DELAY_TIME_L		    0x63

// EC ACPI commands
#define EC_ACPI_DATA_READ		    0x80	// Read ACPI ram space.
#define EC_ACPI_DATA_WRITE		    0x81	// Write ACPI ram space.
//Brightness ACPI Addr
#define BRIGHTNESS_ACPI_ADDR        0x50

// EC HW Ram commands
#define EC_HW_EXTEND_RAM_READ		0x86	// Read EC HW extend ram
#define EC_HW_EXTEND_RAM_WRITE		0x87	// Write EC HW extend ram
#define	EC_HW_RAM_READ			    0x88	// Read EC HW ram
#define EC_HW_RAM_WRITE			    0x89	// Write EC HW ram

// EC Smbus commands
#define EC_SMBUS_CHANNEL_SET	0x8A	// Set selector number (SMBUS channel)
#define EC_SMBUS_ENABLE_I2C		0x8C	// Enable channel I2C
#define EC_SMBUS_DISABLE_I2C	0x8D	// Disable channel I2C

// Smbus transmit protocol
#define EC_SMBUS_PROTOCOL		0x00

// 0x01		Status				SMBUS status 
#define EC_SMBUS_STATUS 		0x01

// 0x02		Address				SMBUS device slave address (8bit£¬bit0 must be 0)
#define EC_SMBUS_SLV_ADDR 		0x02

// 0x03		Command				SMBUS device command
#define EC_SMBUS_CMD  			0x03

// 0x04 -0x24	Data	In read process, return data are stored in this address.
#define EC_SMBUS_DATA  			0x04

#define EC_SMBUS_DAT_OFFSET(n)		(EC_SMBUS_DATA + (n))	// DATA1~32, n:0~31

// 0x2B		SMBUS channel selector	Select SMBUS channel. (0-4)
#define EC_SMBUS_CHANNEL		0x2B

// EC SMBUS transmit Protocol code:
#define SMBUS_QUICK_WRITE		0x02	// 0x02 Write Quick Command
#define SMBUS_QUICK_READ		0x03	// 0x03 Read Quick Command
#define SMBUS_BYTE_SEND			0x04	// 0x04 Send Byte
#define SMBUS_BYTE_RECEIVE		0x05	// 0x05 Receive Byte
#define SMBUS_BYTE_WRITE		0x06	// 0x06 Write Byte
#define SMBUS_BYTE_READ			0x07	// 0x07 Read Byte
#define SMBUS_WORD_WRITE		0x08	// 0x08 Write Word
#define SMBUS_WORD_READ			0x09	// 0x09 Read Word
#define SMBUS_BLOCK_WRITE		0x0A	// 0x0A Write Block
#define SMBUS_BLOCK_READ		0x0B	// 0x0B Read Block
#define SMBUS_PROC_CALL			0x0C	// 0x0C Process Call
#define SMBUS_BLOCK_PROC_CALL	0x0D	// 0x0D Write Block-Read Block Process Call
#define SMBUS_I2C_READ_WRITE	0x0E	// 0x0E I2C block Read-Write
#define SMBUS_I2C_WRITE_READ	0x0F	// 0x0F I2C block Write-Read

// GPIO control commands
#define EC_GPIO_INDEX_WRITE		0x10	// Write Pin number into index
#define EC_GPIO_STATUS_READ		0x11	// According index, get GPIO pin status. 1-GPIO is high, 0-GPIO is low, 0xFF-fail.
#define EC_GPIO_STATUS_WRITE	0x12	// According index, change GPIO pin status. 1-GPIO is high, 0-GPIO is low, 0xFF-fail.
#define EC_GPIO_DIR_READ		0x1D	// According index, get GPIO input/output type. 1-input, 0-output, 0xFF-fail.
#define EC_GPIO_DIR_WRITE		0x1E	// According index, change GPIO input/output type. 1-input, 0-output.

#define EC_GPIO_SSTATE_READ		0x38	// According index, Read AltGpio S State control 
#define EC_GPIO_SSTATE_WRITE	0x39	// According index, Write AltGpio S State control 

// One Key Recovery commands
#define EC_ONE_KEY_FLAG         0x9C    // Get or clear EC One Key Recovery flag

// ASG OEM commands
#define EC_ASG_OEM				0xEA	// ASG OEM Command
#define EC_ASG_OEM_READ			0x00
#define EC_ASG_OEM_WRITE		0x01
#define EC_OEM_POWER_STATUS_VIN1	0X10
#define EC_OEM_POWER_STATUS_VIN2	0X11
#define EC_OEM_POWER_STATUS_BAT1	0X12
#define EC_OEM_POWER_STATUS_BAT2	0X13

// GPIO DEVICE ID
#define EC_DID_ALTGPIO_0        0x10    // 0x10 AltGpio0,               User define gpio
#define EC_DID_ALTGPIO_1        0x11    // 0x11 AltGpio1,               User define gpio
#define EC_DID_ALTGPIO_2        0x12    // 0x12 AltGpio2,               User define gpio
#define EC_DID_ALTGPIO_3        0x13    // 0x13 AltGpio3,               User define gpio
#define EC_DID_ALTGPIO_4        0x14    // 0x14 AltGpio4,               User define gpio
#define EC_DID_ALTGPIO_5        0x15    // 0x15 AltGpio5,               User define gpio
#define EC_DID_ALTGPIO_6        0x16    // 0x16 AltGpio6,               User define gpio
#define EC_DID_ALTGPIO_7        0x17    // 0x17 AltGpio7,               User define gpio

//
// Lmsensor Chip Register
//
#define NSLM96163_CHANNEL		0x02
// NS_LM96163 address 0x98
#define NSLM96163_ADDR			0x98

// LM96163 index(0x00) Local Temperature (Signed MSB)
#define NSLM96163_LOC_TEMP 		0x00

//
#define F75387_REG_R_MANU_ID	0x5D
#define F75387_REG_R_CHIP_ID	0x5A

//
#define LMF75387_MANU_ID_FINTEK			0x1934 //VENDOR ID
#define LMF75387_CHIP_ID_F75387			0x0410 //CHIPID

// Lmsensor Chip SMUBS Slave Address
#define	LMF75387_SMBUS_SLAVE_ADDRESS_5C		0x5c
#define	LMF75387_SMBUS_SLAVE_ADDRESS_5A		0x5A
#define	INA266_SMBUS_SLAVE_ADDRESS_8A		0x8A

//
// Temperature
// Temperature 1 reading (MSB).
#define F75387_REG_R_TEMP0_MSB      0x14    /* 1 degree */
//Temperature 1 reading (LSB).
#define F75387_REG_R_TEMP0_LSB      0x1A    /* 1/256 degree */

//
#define F75387_REG_R_TEMP1_MSB      0x15    /* 1 degree */
#define F75387_REG_R_TEMP1_LSB      0x1B    /* 1/256 degree */

// LOCAL Temperature
#define F75387_REG_R_TEMP2_MSB      0x1C    /* local temp., 1 degree */
#define F75387_REG_R_TEMP2_LSB      0x1D    /*              1/256 degree */

// Voltage
#define F75387_REG_R_V1             0x11    /* 8mV */
#define F75387_REG_R_V2             0x12    /* 8mV */
#define F75387_REG_R_V3             0x13    /* 8mV */

#define INA266_REG_VOLTAGE          0x02    /* 1.25mV */

// Current
#define INA266_REG_CURRENT          0x04    /* 1mA */

// Power
#define INA266_REG_POWER            0x03    /* 25mW */

typedef struct _adv_bios_info {
	int eps_table;
	unsigned short eps_length;
	unsigned int *eps_address;
	unsigned short eps_types;
} adv_bios_info;

struct HW_PIN_TBL
{
	uchar vbat[2];
	uchar v5[2];
	uchar v12[2];
	uchar vcore[2];
	uchar vdc[2];
	uchar ec_current[2];
	uchar power[2];
};

struct Dynamic_Tab
{
        uchar DeviceID;
        uchar HWPinNumber;
};

struct EC_SMBOEM0 {
        uchar HWPinNumber;
};

struct EC_HWMON_DATA{
	struct i2c_client *client;
	struct device *hwmon_dev;
	uchar temperature[3];
	int voltage[7];
	uchar  ec_current[5];
	struct HW_PIN_TBL pin_tbl;
	uchar power[5];
};

struct EC_READ_HW_RAM{
	unsigned int addr;			//IN
	unsigned int data;			//OUT
};

struct EC_WRITE_HW_RAM{
	unsigned int addr;			//IN
	unsigned int data;			//IN
};

struct EC_SMBUS_WORD_DATA{
    unsigned char   Channel;
    unsigned char   Address;
    unsigned char   Register;
    unsigned short  Value;
};

struct EC_SMBUS_READ_BYTE{
	unsigned char Channel;	
	unsigned char Address;
	unsigned char Register;
	unsigned char Data;	
};

struct EC_SMBUS_WRITE_BYTE{
	unsigned char Channel;	
	unsigned char Address;
	unsigned char Register;
	unsigned char Data;	
};

typedef struct _PLED_HW_PIN_TBL{
	uchar pled[6];
} pled_hw_pin_tbl, *ppled_hw_pin_tbl;

extern struct Dynamic_Tab *PDynamic_Tab;
extern char *BIOS_Product_Name;

extern int wait_ibf(void);
extern int wait_obf(void);
extern int wait_smbus_protocol_finish(void);
extern int read_ad_value(uchar hwpin,uchar multi);
extern int read_acpi_value(uchar addr,uchar *pvalue);
extern int write_acpi_value(uchar addr,uchar value);
extern int read_gpio_status(uchar PinNumber,uchar *pvalue);
extern int write_gpio_status(uchar PinNumber,uchar value);

extern int read_gpio_sstatus(uchar hwpin, uchar sstate, uchar *value);
extern int write_gpio_sstatus(uchar hwpin, uchar sstate, uchar value);

extern int read_gpio_dir(uchar PinNumber,uchar *pvalue);
extern int write_gpio_dir(uchar PinNumber,uchar value);
extern int read_hw_ram(uchar addr, uchar *data);
extern int write_hw_ram(uchar addr,uchar data);
extern int write_hw_extend_ram(uchar addr,uchar data);
extern int write_hwram_command(uchar data);
extern int smbus_read_word(struct EC_SMBUS_WORD_DATA *ptr_ec_smbus_word_data);
extern int smbus_read_byte(struct EC_SMBUS_READ_BYTE *ptr_ec_smbus_read_byte);
extern int smbus_write_byte(struct EC_SMBUS_WRITE_BYTE *ptr_ec_smbus_write_byte);
extern int read_onekey_status(uchar addr, uchar *pdata);
extern int write_onekey_status(uchar addr);
extern int ec_oem_get_status(uchar addr, uchar *pdata);
extern int ec_oem_set_status(uchar addr, uchar pdata);
extern int get_productname(char *product);
int adv_get_led(void);
uint check_hw_pin_number(uint led_index);
int set_led_control_value(uint device_id, uint value);
int get_led_control_value(uint device_id, uint *pvalue);
int is_led_controllable(uint device_id);
extern int set_led_flash_type(uint device_id, uint flash_type, uint flash_rate);
extern int set_led_flash_rate(uint device_id, uint led_bit);

#endif
