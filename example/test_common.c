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

#define ADVANTECH_EC_COMMON_VER           "1.00"
#define ADVANTECH_EC_COMMON_DATE          "05/11/2018" 

#define COMMON_MAGIC				'p'
#define IOCTL_EC_GET_STATUS			_IO(COMMON_MAGIC, 0xB1)
#define IOCTL_EC_SET_STATUS			_IOW(COMMON_MAGIC, 0xB2, int)

#define EC_OEM_LED0					0x00
#define EC_OEM_LED1					0x01
#define EC_OEM_LED2					0x02
#define EC_OEM_LED3					0x03
#define EC_OEM_LED4					0x04
#define EC_OEM_LED5					0x05
#define EC_OEM_LED6					0x06
#define EC_OEM_LED7					0x07
#define EC_OEM_LED8					0x08
#define EC_OEM_LED9					0x09
#define EC_OEM_LED10				0x0A
#define EC_OEM_LED11				0x0B
#define EC_OEM_LED12				0x0C
#define EC_OEM_LED13				0x0D
#define EC_OEM_LED14				0x0E
#define EC_OEM_LED15				0x0F
#define EC_OEM_POWER_STATUS_VIN1	0x10
#define EC_OEM_POWER_STATUS_VIN2	0x11
#define EC_OEM_POWER_STATUS_BAT1	0x12
#define EC_OEM_POWER_STATUS_BAT2	0x13

#define POWER_LOW				0
#define	POWER_NORMAL			1
#define LED_OFF					0
#define LED_ON					1

int common_fd;
char *const short_options = "g:s:h";
struct option long_options[] = {
	{"get_device", 1, NULL, 'g'},
	{"set_device", 2, NULL, 's'},
	{"help", 0, NULL, 'h'},
	{NULL, 0, NULL, 0},
};

typedef struct __common_command {
	uint	id;
	uint	status;
} _common_command;

void usage(const char *filename)
{
	printf("Usage: #%s [OPTIONS] \n", filename);
	printf("[OPTIONS]: \n");
	printf(" -g <DEVICE_ID>: Get common gpio device status \n");
	printf(" -s <DEVICE_ID> <OPTION>: Set common gpio device status \n");
	printf(" -h show this help \n");
	printf("\n");
	printf("<DEVICE_ID>    Description \n");
	printf("0x00-0x0F      LED0~LED15 (Support get/set) \n");
	printf("0x10-0x11      Power Status VIN1 ~ VIN2, 0->Power Low, 1->Normal (Only support get) \n");
	printf("0x12-0x13      Power Status BAT1 ~ BAT2 (Reserve) \n");
	printf("\n");
	printf("<OPTION>       Description \n");
	printf("0x1            gpio status: pull up \n");
	printf("0x0            gpio status: pull down \n");
	printf("\n");
	printf("Example: ./test_common -g 0x11 (get power1 status) \n");
	printf("         ./test_common -g 0x12 (get power2 status) \n");
	printf("         ./test_common -g 0x01 (get led1 status) \n");
	printf("         ./test_common -s 0x01 0x0 (set led1 down) \n");
	printf("         ./test_common -s 0x01 0x1 (set led1 up) \n");
	printf("\n");
}

int my_ctoi(char c)
{
	unsigned int i = -1;

	if (c <= 102 && c >= 97) {	// a~f
		i = c - 97 + 10;
	} else if (c <= 70 && c >= 65) {	// A~F
		i = c - 65 + 10;
	} else if (c <= 57 && c >= 48) {	// 0~9
		i = c - 48;
	}

	return i;
}

unsigned int get_input_value(const char *str, unsigned int max)
{
	unsigned int ret = 999;

	if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X')) {
		ret = 0;
	} else if (strlen(str) == 3) {
		ret = my_ctoi(str[2]);
	} else if (strlen(str) == 4) {
		ret = my_ctoi(str[2])*16 + my_ctoi(str[3]);
	}

	//	printf("str: %s\nmax:0x%X\nret(hex):0x%X\nret(uint):%u\n", str, max, ret, ret);
	if (ret > max) {
		ret = 999;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	_common_command common_command;
	uint tmp_device_id = 0;
	uint i, c, count = 0, ret = 0;

	if(argc < 2) {
		printf("Try \"%s -h\" to get more help. \n", argv[0]);
		return 0;
	}

	common_fd = open("/dev/advcommon", O_WRONLY);		// O_WRONLY | O_RDWR
	if (common_fd < 0 ) {
		perror("/dev/advcommon");
		return -1;
	}

	memset(&common_command, 0, sizeof(common_command));
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch(c)
		{
		case 'g':
			common_command.id = get_input_value(optarg, EC_OEM_POWER_STATUS_VIN2);
			if (common_command.id != 999) {
				printf("Get gpio(0x%X) status ... \n", common_command.id);
			} else {
				printf("Use wrong id ! \n");
				goto end_exit;
			}

			if ((ret = ioctl(common_fd, IOCTL_EC_GET_STATUS, &common_command))) {
				perror("ioctl");
				goto end_exit;
			}
			break;
		case 's':
			common_command.id = get_input_value(optarg, EC_OEM_LED15);
			if (common_command.id != 999) {
				printf("Set gpio(0x%X) status ... \n", common_command.id);
			} else {
				printf("Use wrong id ! \n");
				goto end_exit;
			}

			common_command.status = get_input_value(argv[optind], LED_ON);
			if (common_command.status != 999) {
				printf("Status value => 0x%X \n", common_command.status);
			} else {
				printf("Set wrong status ! \n");
				goto end_exit;
			}

			if ((ret = ioctl(common_fd, IOCTL_EC_SET_STATUS, &common_command))) {
				perror("ioctl");
				goto end_exit;
			}
			break;
		case 'h':
			usage(argv[0]);
			ret = 0;
			goto end_exit;
		default:
			usage(argv[0]);
			ret = -1;
			goto end_exit;
		}
	}

	switch (common_command.id) 
	{
	case EC_OEM_LED0:
	case EC_OEM_LED1:
	case EC_OEM_LED2:
	case EC_OEM_LED3:
	case EC_OEM_LED4:
	case EC_OEM_LED5:
	case EC_OEM_LED6:
	case EC_OEM_LED7:
	case EC_OEM_LED8:
	case EC_OEM_LED9:
	case EC_OEM_LED10:
	case EC_OEM_LED11:
	case EC_OEM_LED12:
	case EC_OEM_LED13:
	case EC_OEM_LED14:
	case EC_OEM_LED15:
		printf("gpio: 0x%X    status: %s \n", common_command.id, common_command.status?"ON":"OFF");
		break;
	case EC_OEM_POWER_STATUS_VIN1:
		printf("Get power1 status: %s \n", common_command.status?"ON":"OFF");
		break;
	case EC_OEM_POWER_STATUS_VIN2:
		printf("Get power2 status: %s \n", common_command.status?"ON":"OFF");
		break;
	case EC_OEM_POWER_STATUS_BAT1:
		break;
	case EC_OEM_POWER_STATUS_BAT2:
		break;
	}

end_exit:
	close(common_fd);
	return ret;
}
