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

#define FAN_MAGIC       'f'
#define SETFANSPEED     _IO(FAN_MAGIC, 0)
#define GETFANSPEED     _IO(FAN_MAGIC, 1)

void usage(char *filename)
{
	printf("Usage: #%s [OPTIONS]\n", filename);
	printf("OPTIONS:\n");
	printf(" -r: Get fan speed\n");
	printf(" -w x (x is 0~100): Set the percent of fan speed to x\n");
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	int status;
	int func;
	unsigned int data[3];
	
	if ((fd = open("/dev/advfan", O_RDWR)) < 0 )
	{
		printf("Can not open advfan device!\n");
		return -1;
	}

	if (argc < 2) 
	{
		usage(argv[ 0 ]);
		return 0;
	}
	
	if (!strncasecmp(argv[1], "-r", 2)) 
	{	
		status = ioctl(fd,GETFANSPEED,data);
		
		if(status != 0)
			printf(" failed\n");
		else
			printf(" %d\n",data[0]);
	}
	else if (!strncasecmp(argv[1], "-w", 2))
	{
		data[0] = atol(argv[ 2 ]);
		
		status = ioctl(fd,SETFANSPEED,data);
		
		if(status != 0)
		{
			printf("Set GPIO status failed\n");
			ret = -1;
			goto ERROR;
		}else
		{
			printf("OK\n");
		}
		
		status = ioctl(fd,GETFANSPEED,data);
		
		if(status != 0)
			printf(" failed\n");
		else
			printf(" %d\n",data[0]);			
	}

ERROR:
	close(fd);
	return ret;	
}
