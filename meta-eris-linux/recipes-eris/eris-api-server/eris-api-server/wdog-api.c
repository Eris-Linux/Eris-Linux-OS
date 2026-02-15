/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

#include "api-server.h"
#include "wdog-api.h"


// ---------------------- Private macros declarations.

#define WATCHDOG_FILE   "/dev/watchdog0"
#define WATCHDOG_DELAY_PREFIX "watchdog_delay="


// ---------------------- Private types definitions.

// ---------------------- Private method declarations.

static void initialize_wd(void);
static void *wd_thread(void *arg);
static void keep_wd_alive_command(int sock, int argc, char *argv[]);
static void disable_wd_command(int sock, int argc, char *argv[]);
static void get_wd_delay_command(int sock, int argc, char *argv[]);
static void set_wd_delay_command(int sock, int argc, char *argv[]);
static int  keep_wd_alive(void);
static int  disable_wd(void);
static int  set_wd_delay(int delay);


// ---------------------- Private variables declarations.

// ---------------------- Public methods definitions.

int init_wdog_api(void)
{

	initialize_wd();

	if (register_api_command("keep-wdog-alive", "kwda", "Keep the watchdog alive.", keep_wd_alive_command))
		return -1;

	if (register_api_command("disable-wdog",   "dswd", "Disable the watchdog alive.", disable_wd_command))
		return -1;

	if (register_api_command("get-wdog-gelay", "gwdd", "Get the watchdog delay in seconds.", get_wd_delay_command))
		return -1;

	if (register_api_command("set-wdog-gelay", "swdd", "Set the watchdog delay in seconds.", set_wd_delay_command))
		return -1;

	return 0;
}


// ---------------------- Private methods definitions.

static void initialize_wd(void)
{
	char *delay_line = NULL;
	long int delay;
	pthread_t watchdog_thread;

	if ((read_parameter_value(WATCHDOG_DELAY_PREFIX, &delay_line) == 0)
	 && (delay_line != NULL)
	 && (sscanf(delay_line, "%ld", &delay) == 1))
		set_wd_delay(delay);

	pthread_create(&watchdog_thread, NULL, wd_thread, (void *) delay);
}


static void *wd_thread(void *arg)
{
	long int delay = (long int) arg;

	pthread_detach(pthread_self());

	for (;;) {
		sleep(delay / 2);
		keep_wd_alive();
	}
	return NULL;
}



static void keep_wd_alive_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "keep-wdog-alive doesn't take any an argument.");
		return;
	}

	if (keep_wd_alive() != 0) {
		send_error(sock, errno, "Unable to feed watchdog.");
		return;
	}

	send_reply(sock, 2, "Ok");
}



static void disable_wd_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "disable-wdog doesn't take any an argument.");
		return;
	}

	if (disable_wd() != 0) {
		send_error(sock, errno, "Unable to disable watchdog.");
		return;
	}

	send_reply(sock, 2, "Ok");

}



static void get_wd_delay_command(int sock, int argc, char *argv[])
{
	int fd;
	int interval;

	if (argc > 0) {
		send_error(sock, EINVAL, "get-wdog-gelay doesn't take any an argument.");
		return;
	}

	if ((fd = open(WATCHDOG_FILE, O_RDONLY)) < 0) {
		send_error(sock, errno, "Unable to open " WATCHDOG_FILE ".");
		return;
	}
	if (ioctl(fd, WDIOC_GETTIMEOUT, &interval) == 0) {
		char *reply = NULL;
		size_t size = 0;
		size_t pos  = 0;

		addsnprintf(&reply, &size, &pos, "%d", interval);
		send_reply(sock, strlen(reply), reply);
		free(reply);
	}
	close(fd);
}



static void set_wd_delay_command(int sock, int argc, char *argv[])
{
	unsigned int value;

	if (argc < 1) {
		send_error(sock, EINVAL, "This command needs an argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "This command takes only one argument.");
		return;
	}

	if ((sscanf(argv[0], "%u", &value) != 1)
	 || (value < 1)
	 || (value > 48)) {
		send_error(sock, EINVAL, "Invalid argument for set-wd-delay command.");
		return;
	}

	if (set_wd_delay(value) != 0) {
		send_error(sock, errno, "Unable to open " WATCHDOG_FILE ".");
		return;
	}

	write_parameter_value(WATCHDOG_DELAY_PREFIX, argv[0]);

	send_reply(sock, 2, "Ok");
}



static int keep_wd_alive(void)
{
	int fd;

	if ((fd = open(WATCHDOG_FILE, O_RDONLY)) < 0)
		return -1;
	ioctl(fd, WDIOC_KEEPALIVE, 0);
	close(fd);

	return 0;
}



static int disable_wd(void)
{
	int fd;

	if ((fd = open(WATCHDOG_FILE, O_RDONLY)) < 0)
		return -1;

	unsigned int value = WDIOS_DISABLECARD;
	ioctl(fd, WDIOC_SETOPTIONS, &value);
	close(fd);

	return 0;
}



static int set_wd_delay(int delay)
{
	int fd;

	if ((fd = open(WATCHDOG_FILE, O_RDONLY)) < 0)
		return -1;

	ioctl(fd, WDIOC_SETTIMEOUT, &delay);
	ioctl(fd, WDIOC_KEEPALIVE, 0);
	close(fd);

	return 0;
}

