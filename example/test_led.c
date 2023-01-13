#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define LED_DISABLE  0x0  // disable diagnostic LED 
#define LED_ON       0x1  // Light on
#define LED_FAST     0x3  // fast flicker
#define LED_NORMAL   0x5  // normal flick
#define LED_SLOW     0x7  // slow flicker
#define LED_MANUAL   0xF  // user define 

#define LED_MAGIC					'p'
#define IOCTL_EC_HWRAM_GET_VALUE	_IO(LED_MAGIC, 0x0B)		//IOCTL Command: EC Hardware Ram
#define IOCTL_EC_HWRAM_SET_VALUE	_IO(LED_MAGIC, 0x0C)
//#define IOCTL_EC_DYNAMIC_TBL_GET	_IO(LED_MAGIC, 0x0D)
#define IOCTL_EC_ONEKEY_GET_STATUS	_IO(LED_MAGIC, 0x0D)		//IOCTL Command: EC One Key Recovery
#define IOCTL_EC_ONEKEY_SET_STATUS	_IO(LED_MAGIC, 0x0E)
#define IOCTL_EC_LED_USER_DEFINE	_IOW(LED_MAGIC, 0x0F, int)	//IOCTL Command: EC LED Control
#define IOCTL_EC_LED_CONTROL_ON		_IO(LED_MAGIC, 0xA0)
#define IOCTL_EC_LED_CONTROL_OFF	_IO(LED_MAGIC, 0xA1)
#define IOCTL_EC_LED_BLINK_ON		_IO(LED_MAGIC, 0xA2)
#define IOCTL_EC_LED_BLINK_OFF		_IO(LED_MAGIC, 0xA3)
#define IOCTL_EC_LED_BLINK_SPACE	_IO(LED_MAGIC, 0xA4)

#define ADVANTECH_EC_LED_VER           "1.00"
#define ADVANTECH_EC_LED_DATE          "06/23/2016" 

//#define USE_WRITE

int led_fd;
char *const short_options = "d:ht:r:";
struct option long_options[] = {
	{"device_id", 1, NULL, 'd'},
	{"help", 0, NULL, 'h'},
	{"type", 1, NULL, 't'},
	{"rate", 1, NULL, 'r'},
	{0, 0, 0, 0},
};

typedef struct __led_command {
	uint	device_id;
	uint	flash_type;
	uint	flash_rate;
} _led_command;

void usage(const char *filename)
{
	printf("Usage: #%s [OPTIONS] \n", filename);
	printf("OPTIONS: \n");
	printf("    \n");
//	printf(" -d [DEVICE_ID]: Set device_id for advled ( 0xE1 ~ 0xE6 ) \n");
	printf(" -d [DEVICE_ID]: Set device_id on APAX-5580 LED (0xE1:RUN_LED, 0xE2:ERR_LED) \n");
	printf("    \n");
	printf(" -h get more help \n");
	printf("    \n");
	printf(" -t [FLASH_TYPE]: Set flash type of advled \n");
	printf("    |    0    | 1  |  3   |    5   |   7  |   15   | \n");
	printf("    | DISABLE | ON | FAST | NORMAL | SLOW | MANUAL | \n");
	printf("    \n");
//	printf(" -r [FLASH_RATE]: Set flash rate of advled,depends on [FLASH_TYPE]=MANUAL \n");
//	printf("    | BIT F ~ BIT 6 (10 Bit) | BIT 5  | BIT 4  | BIT 3 ~ BIT 0 | \n");
//	printf("    |         LedBit         | IntCtl | Enable |   Polarity    | \n");
//	printf("    Polarity - 0: low active; 1: high active. \n");
//	printf("    Enable   - 0: disable control; 1: enable led control. \n");
//	printf("    IntCtl   - Internal control. 1: Control by EC, external control is not allow. \n");
//	printf("    LedBit   - Led Flash bit. Total 10 Bit. Every 100ms, EC get a bit one by one. \n");
//	printf("    If the bit is 1, led light. Otherwise led dark. \n");
	printf(" -r [FLASH_RATE]: Set flash rate of advled, depends on [FLASH_TYPE]=MANUAL \n");
	printf("    | BIT F ~ BIT 6 (10 Bit) | BIT 5  | BIT 4  | BIT 3 ~ BIT 0 | \n");
	printf("    |         LedBit         |              Default            | \n");
	printf("    LedBit  - Led Flash bit. Total 10 Bit ( 0x00 ~ 0x3FF ). Every 100ms, EC get a bit \n");
	printf("              one by one. If the bit is 1, led light. Otherwise led dark. \n");
	printf("               \n");
	printf("    Example: ./test_led -d 0xE1 -t 15 -r 0x0000 (LED disable) \n");
	printf("             ./test_led -d 0xE1 -t 15 -r 0x02AA (LED flash fast) \n");
	printf("             ./test_led -d 0xE1 -t 15 -r 0x03E0 (LED flash slow) \n");
	printf("    \n");
}

int main(int argc, char *argv[])
{
	_led_command led_command;
	uint tmp_device_id = 0;
	uint tmp_flash_type = 0;
	uint i, c, count = 0, ret = 0;

	if(argc < 2) {
		printf("Try \"%s -h\" to get more help. \n", argv[0]);
		return 0;
	}
	
	led_fd = open("/dev/advled", O_WRONLY);		// O_WRONLY | O_RDWR
	if (led_fd < 0 ) {
		perror("/dev/advled");
		return -1;
	}

	memset(&led_command, 0, sizeof(led_command));
	while((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch(c)
		{
		case 'd':
			if(!strncmp(optarg, "0xE", 3) || !strncmp(optarg, "0xe", 3) 
					|| !strncmp(optarg, "0XE", 3) || !strncmp(optarg, "0Xe", 3)) {
				tmp_device_id = (optarg[3] - 48);
				tmp_device_id += 224;
			} else if(optarg[0] == 'e' || optarg[0] == 'E') {
				tmp_device_id = (optarg[1] - 48);
				tmp_device_id += 224;
			}
			printf("led device id is %02X(%d). \n", tmp_device_id, tmp_device_id);
//			if((tmp_device_id < 225) || (tmp_device_id > 230)) {
			if((tmp_device_id < 225) || (tmp_device_id > 226)) {
				printf("Set led device ID (Hex) error. \n");
				printf("Only \"0xE1\" (RUN_LED) or \"0xE2\" (ERR_LED) is user defined on APAX-5580. \n");
				ret = -1;
				goto error_exit;
			} else {
				led_command.device_id = tmp_device_id;
			}
		break;
		case 't':
			tmp_flash_type = (uint)atoi(optarg);
			if((tmp_flash_type == LED_DISABLE) || (tmp_flash_type == LED_ON) || (tmp_flash_type == LED_FAST) 
				|| (tmp_flash_type == LED_NORMAL) || (tmp_flash_type == LED_SLOW) || (tmp_flash_type == LED_MANUAL)) {
				led_command.flash_type = tmp_flash_type;
			} else {
				printf("Set led flash type error. \n");
				ret = -1;
				goto error_exit;
			}
		break;
		case 'r':
		{
			led_command.flash_rate = 0;
			count = strlen(optarg);
			if((optarg[0] == '0') && ((optarg[1] == 'x') || (optarg[1] == 'X'))) {
				for(i = 2; i < count; i++)
				{
					led_command.flash_rate *= 16;
					if((optarg[i] >= '0') && (optarg[i] <= '9')) {
						led_command.flash_rate += (optarg[i] - 48);
					} else if((optarg[i] >= 'A') && (optarg[i] <= 'F')) {
						led_command.flash_rate += (optarg[i] - 55);
					} else if((optarg[i] >= 'a') && (optarg[i] <= 'f')) {
						led_command.flash_rate += (optarg[i] - 87);
					} else {
						printf("Set led flash rate (Hex) error. \n");
					}
				}
			} else {
				for(i = 0; i < count; i++)
				{
					led_command.flash_rate *= 16;
					if((optarg[i] >= '0') && (optarg[i] <= '9')) {
						led_command.flash_rate += (optarg[i] - 48);
					} else if((optarg[i] >= 'A') && (optarg[i] <= 'F')) {
						led_command.flash_rate += (optarg[i] - 55);
					} else if((optarg[i] >= 'a') && (optarg[i] <= 'f')) {
						led_command.flash_rate += (optarg[i] - 87);
					} else {
						printf("Set led flash rate (Hex) error. \n");
					}
				}
			}

			if(led_command.flash_rate > 0x3FF) {
				printf("This flash rate is out of range ( <= 0x3FF ). \n");
				ret = -1;
				goto error_exit;
			}
//			led_command.flash_rate = (uint)atoi(optarg);
		}
		break;
		case 'h':
			usage(argv[0]);
			ret = 0;
			goto error_exit;
		}
	}

	printf("device_id: 0x%02X. \n", led_command.device_id);
	printf("flash_type: %d. \n", led_command.flash_type);
	printf("flash_rate: 0x%04X. \n", led_command.flash_rate);

#ifdef USE_WRITE
	if(write(led_fd, &led_command, sizeof(led_command)) == -1) {
		perror("write");
	}
#else
	ret = ioctl(led_fd, IOCTL_EC_LED_USER_DEFINE, &led_command);
	if(ret) {
		perror("ioctl");
	}
#endif

	close(led_fd);
	return ret;

error_exit:
	close(led_fd);
	return ret;
}
