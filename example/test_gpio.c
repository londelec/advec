#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define DIR_IN	0x80
#define DIR_OUT	0x40  
#define OUTPUT_LOW  0
#define OUTPUT_HIGH	1

#define EC_MAGIC        'p'
#define SETGPIODIR              _IO(EC_MAGIC, 7)
#define GETGPIODIR              _IO(EC_MAGIC, 8)
#define ECSETGPIOSTATUS         _IO(EC_MAGIC, 9)
#define ECGETGPIOSTATUS         _IO(EC_MAGIC, 0x0a)

int main(void)
{
	int ignore_ret = 0;
	int ret = 0;
	int fd;
	int status;
	int tmp_index;
	int tmp_dir;
	int tmp_status;
	unsigned int gpio_index;
	unsigned int gpio_dir;
	unsigned int gpio_status;
	unsigned int data[2];

	if ((fd = open("/dev/advgpio", O_RDWR)) < 0 )
	{
		printf("Can not open advgpio device!\n");
		return -1;
	}

	printf("Please input the gpio index.\n");
	printf("The index ragnge(0~7):");
	ignore_ret = scanf("%d",&tmp_index);
	
	if((tmp_index < 0) || (tmp_index > 7))
	{
		printf("GPIO index range: 0 to 7\n");
		ret = -1;
		goto ERROR;
	}else{
		gpio_index = (unsigned int)tmp_index;
	}

	printf("Please input the gpio direction:(0:input, 1:output)\n");	

	ignore_ret = scanf("%d",&tmp_dir);
	if(tmp_dir == 0){
		gpio_dir = DIR_IN;
	}else if(tmp_dir == 1)
		gpio_dir = DIR_OUT;
	else{
		printf("GPIO direction can only be 0:input, 1:output\n");
		ret = -1;
		goto ERROR;
	}
	
	printf("****************************************************\n");
	//Set GPIO direction
	data[0] = gpio_index;	//GPIO index
	data[1] = gpio_dir;		//GPIO direction
	printf("Set %d GPIO the dir value is: %2X\n", data[0],data[1]);
	status = ioctl(fd,SETGPIODIR,data);
	if(status != 0){
		printf("Set GPIO direction failed\n");
		ret = -1;
		goto ERROR;
	}else{
		printf("OK\n");
	}
	
	printf("****************************************************\n");
	//Get GPIO direction
	data[0] = gpio_index;	//GPIO index
	data[1] = 0;			//GPIO direction
	printf("Get %d GPIO the dir", data[0]);
	status = ioctl(fd,GETGPIODIR,data);
	if(status != 0){
		printf(" failed\n");
		ret = -1;
		goto ERROR;
	}else{
		printf(" is: 0x%2X.",data[1]);
		if(data[1] == DIR_IN)
			printf(" input\n");
		else if(data[1] == DIR_OUT)
			printf(" output\n");
		else{
			printf(" Get invalid value\n");
			ret = -1;
			goto ERROR;
		}			
	}
	
	printf("****************************************************\n");
	if(data[1] == DIR_IN)
	{
		printf("INPUT: Get %d GPIO the status\n", data[0]);
		//input mode, get GPIO value
		data[0] = gpio_index;	//GPIO index
		data[1] = 0;			//GPIO direction
		status = ioctl(fd,ECGETGPIOSTATUS,data);
		if(status != 0){
			printf(" failed\n");
			ret = -1;
			goto ERROR;
		}		
		else{
			if(data[1] == 0 || data[1] == 1)
				printf("The value is %d\n", data[1]);
			else{
				printf("Get invalid value\n");
				ret = -1;
				goto ERROR;
			}						
		}
	}else if(data[1] == DIR_OUT)
	{
		printf("OUTPUT: Set %d GPIO the status\n", data[0]);
		
		printf("Please input the GPIO status.(0:OUTPUT_LOW, 1:OUTPUT_HIGH):\n");
		ignore_ret = scanf("%d",&tmp_status);
		if(tmp_status!=OUTPUT_LOW && tmp_status!=OUTPUT_HIGH)
		{
			printf("Invalid input\n");
			ret = -1;
			goto ERROR;
		}else{
			
			printf("****************************************************\n");
			//Set GPIO output status
			data[0] = gpio_index;				//GPIO index
			data[1] = (unsigned int)tmp_status;	//GPIO direction
			printf("Set %d GPIO the status is: %d\n", data[0],data[1]);
			status = ioctl(fd,ECSETGPIOSTATUS,data);
			if(status != 0){
				printf("Set GPIO status failed\n");
				ret = -1;
				goto ERROR;
			}else{
				printf("OK\n");
			}
			
			printf("****************************************************\n");
			//Get GPIO output stauts
			data[0] = gpio_index;	//GPIO index
			data[1] = 0;			//GPIO direction
			printf("Get %d GPIO the status is:", data[0]);
			status = ioctl(fd,ECGETGPIOSTATUS,data);
			if(status != 0){
				printf(" failed\n");
			}else{
				if(data[1]!=0 && data[1]!=1){
					printf(" Get invalid value\n");
					ret = -1;
					goto ERROR;
				}else
					printf(" %d\n",data[1]);
			}
		}

	}
	
ERROR:
	close(fd);
	return ret;
}
