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

#define EC_MAGIC        'p'
#define SETMINBRIGHTNESS        _IO(EC_MAGIC, 1)
#define SETMAXBRIGHTNESS        _IO(EC_MAGIC, 2)
#define SETBRIGHTNESS           _IO(EC_MAGIC, 3)
#define GETMINBRIGHTNESS        _IO(EC_MAGIC, 4)
#define GETMAXBRIGHTNESS        _IO(EC_MAGIC, 5)
#define GETBRIGHTNESS           _IO(EC_MAGIC, 6)

void usage(char *filename)
{
	printf("Usage: #%s [OPTIONS]\n", filename);
	printf("OPTIONS:%s must have three parameters\n",filename);
   	printf("The first parameter is min brightness value\n");
   	printf("The second parameter is max brightness value\n");
   	printf("The last parameter is seted brightness value\n");
}

int main(int argc,char ** argv)
{
        int retval = 0;
	int fd;
        int status;
        unsigned int data1 = 0;
        unsigned int data2 = 0;
        unsigned int data3 = 0;
	
	if(argc < 4)
	{
		usage(argv[ 0 ]);
		return -1;
	}
        data1 = atoi(argv[1]);
        data2 = atoi(argv[2]);
        data3 = atoi(argv[3]);
        if ((fd = open("/dev/advbrightness", O_RDWR)) < 0 )
        {
        	printf("Can not open advbrightness device!\n");
                return -1;
        }
        status = ioctl( fd, SETMINBRIGHTNESS, &data1 );
        if(status < 0)
        {
        	printf("ioctl set min brightness failed\n");
                return -1;
        }
        printf("Set Min Brightness. Value is: %d\n", data1);

        status = ioctl( fd, SETMAXBRIGHTNESS, &data2 );
        if(status < 0)
        {
        	printf("ioctl set max brightness failed\n");
                return -1;
        }
        printf("Set MAX Brightness. Value is: %d\n", data2);

        status = ioctl( fd, SETBRIGHTNESS, &data3 );
        if(status < 0)
        {
        	printf("ioctl set brightness failed\n");
                return -1;
        }
        printf("Set Brightness Default. Value is: %d\n", data3);

        status = ioctl( fd, GETMINBRIGHTNESS, &data1 );
	if(status<0)
	{
		printf("get min brightness failed");
	}
        printf("Get Min Brightness. Value is: %d\n", data1);

        status = ioctl( fd, GETMAXBRIGHTNESS, &data2 );
	if(status<0)
	{
                printf("get min brightness failed");
	}
        printf("Get MAX Brightness. Value is: %d\n", data2);

        status = ioctl( fd, GETBRIGHTNESS, &data3 );
	if(status<0)
	{
                printf("get min brightness failed");
	}
        printf("Get Brightness Default. Value is: %d\n", data3);

        close(fd);

	return 0;
}
