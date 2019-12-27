/* **************************************************
 * EC Watchdog keep alive daemon (for Linux system).
 * **************************************************/

#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#define WDIOC_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT        _IOR(WATCHDOG_IOCTL_BASE, 7, int)

#define RELOAD_TIME			40

int wdt_enable(void);
int wdt_disable(void);
int keep_alive(void);

int main(int argc, const char *argv[])
{
	pid_t pid = 0, sid = 0;
	int delay = 1000000;		// 1s = 1000000 us
	int ret = 0;


	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	ret = wdt_enable();
	if (ret == -1) {
		return ret;
	}

	delay *= RELOAD_TIME;
	while (1) {
		ret = keep_alive();
		if (ret == -1) {
			return ret;
		}
		usleep(delay);
	}

	wdt_disable();
	exit(EXIT_SUCCESS);

	return 0;
}

/* **************************************************
 * This function simply sends an IOCTL to the driver, 
 * which in turn ticks the PC WDT to reset its internal 
 * timer so it doesn't trigger a computer reset.
 * **************************************************/
int keep_alive(void)
{
	int fd = 0, retval = 0;
	int i_en = WDIOS_ENABLECARD;

	fd = open("/dev/watchdog", O_WRONLY);
	if (fd == -1) {
		return -EBUSY;
	}

	ioctl(fd, WDIOC_KEEPALIVE, NULL);

	close(fd);
}

int wdt_disable(void)
{
	int fd = 0, retval = 0;
	int i_dis = WDIOS_DISABLECARD;

	fd = open("/dev/watchdog", O_WRONLY);
	if (fd == -1) {
		return -EBUSY;
	}

	retval = write(fd, "V", 1);
	ioctl(fd, WDIOC_SETOPTIONS, &i_dis);

	close(fd);
}

int wdt_enable(void)
{
	int fd = 0, retval = 0;
	int i_en = WDIOS_ENABLECARD;

	fd = open("/dev/watchdog", O_WRONLY);
	if (fd == -1) {
		return -EBUSY;
	}

	retval = ioctl(fd, WDIOC_SETOPTIONS, &i_en);
	if (retval != 0) {
		wdt_disable();
		return -1;
	}

	close(fd);
}

