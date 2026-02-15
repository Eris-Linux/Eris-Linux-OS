/*
 *  ERIS LINUX REST API
 *
 *  (c) 2024-2026: Logilin
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

#include "eris-rest-api.h"
#include "wdog-rest-api.h"


// ---------------------- Private macros declarations.

#define WATCHDOG_FILE   "/dev/watchdog0"
#define WATCHDOG_DELAY_PREFIX "watchdog_delay="


// ---------------------- Private types definitions.

// ---------------------- Private method declarations.

static enum MHD_Result feed_watchdog          (struct MHD_Connection *connection);
static enum MHD_Result disable_watchdog       (struct MHD_Connection *connection);
static enum MHD_Result get_watchdog_delay     (struct MHD_Connection *connection);
static enum MHD_Result set_watchdog_delay     (struct MHD_Connection *connection);
static enum MHD_Result start_watchdog_feeder  (struct MHD_Connection *connection);
static enum MHD_Result stop_watchdog_feeder   (struct MHD_Connection *connection);
static enum MHD_Result watchdog_feeder_status (struct MHD_Connection *connection);


static void *_feeder_function(void *arg);
static int   _keep_wd_alive(void);
static int   _disable_wd(void);
static int   _get_wd_delay(int *delay);
static int   _set_wd_delay(int delay);


// ---------------------- Private variables declarations.

static int       _feeder_running = 0;
static pthread_t _feeder_thread;
static int       _watchdog_fd = -1;


// ---------------------- Public methods definitions.

int init_wdog_rest_api(const char *name)
{

	char *delay_line = NULL;
	long int delay;

	if ((_watchdog_fd = open(WATCHDOG_FILE, O_RDWR)) < 0)
		return -1;

	if ((read_parameter_value(WATCHDOG_DELAY_PREFIX, &delay_line) == 0)
	 && (delay_line != NULL)
	 && (sscanf(delay_line, "%ld", &delay) == 1))
		_set_wd_delay(delay);
	
	pthread_create(&_feeder_thread, NULL, _feeder_function, NULL);

	return 0;
}



enum MHD_Result wdog_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcasecmp(url, "/api/watchdog") == 0) {
		if (strcmp(method, "POST") == 0)
			return feed_watchdog(connection);
		if (strcmp(method, "DELETE") == 0)
			return disable_watchdog(connection);
	}
	if (strcasecmp(url, "/api/watchdog/delay") == 0) {
		if (strcmp(method, "GET") == 0)
            return get_watchdog_delay(connection);
		if (strcmp(method, "PUT") == 0)
			return set_watchdog_delay(connection);
	}

	if (strcasecmp(url, "/api/watchdog/feeder") == 0) {
		if (strcmp(method, "GET") == 0)
			return watchdog_feeder_status(connection);
		if (strcmp(method, "POST") == 0)
			return start_watchdog_feeder(connection);
		if (strcmp(method, "DELETE") == 0)
			return stop_watchdog_feeder(connection);
	}
	return MHD_NO;
}


// ---------------------- Private methods definitions.

static enum MHD_Result feed_watchdog(struct MHD_Connection *connection)
{
	if (_keep_wd_alive() == 0) 
		return send_rest_response(connection, "Ok");
	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result disable_watchdog(struct MHD_Connection *connection)
{
	if (_feeder_running) {
		pthread_cancel(_feeder_thread);
		pthread_join(_feeder_thread, NULL);
		_feeder_running = 0;
	}
	if (_disable_wd() == 0) 
		return send_rest_response(connection, "Ok");
	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result get_watchdog_delay(struct MHD_Connection *connection)
{
	int delay;
	if (_get_wd_delay(&delay) < 0) 
		return send_rest_error(connection, "No watchdog available", 500);

	char buffer[32];
	snprintf(buffer, 32, "%d", delay);
	return send_rest_response(connection, buffer);
}



static enum MHD_Result set_watchdog_delay (struct MHD_Connection *connection)
{
	const char *delay_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "delay");
	if (delay_str == NULL)
        return send_rest_error(connection, "Missing delay.", 400);

	unsigned int delay;
	if ((sscanf(delay_str, "%d", & delay) != 1)
	 || (delay < 1)
	 || (delay > 48))
		return send_rest_error(connection, "Invalid delay.", 400);

	if (_set_wd_delay(delay) == 0)
		return send_rest_response(connection, "Ok");

	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result start_watchdog_feeder(struct MHD_Connection *connection)
{
	if (_feeder_running == 0) {
		pthread_create(&_feeder_thread, NULL, _feeder_function, NULL);
		return send_rest_response(connection, "Ok");
	}
	return send_rest_error(connection, "Already running", 400);
}



static enum MHD_Result stop_watchdog_feeder(struct MHD_Connection *connection)
{
	if (_feeder_running) {
		pthread_cancel(_feeder_thread);
		pthread_join(_feeder_thread, NULL);
		_feeder_running = 0;
		return send_rest_response(connection, "Ok");
	}
	return send_rest_error(connection, "Already stopped", 400);
}



static enum MHD_Result watchdog_feeder_status(struct MHD_Connection *connection)
{
	return send_rest_response(connection, _feeder_running ? "running" : "stopped");

}



static void *_feeder_function(void *arg)
{
	_feeder_running = 1;
	for (;;) {
		_keep_wd_alive();
		sleep(1);
	}
	return NULL;
}



static int _keep_wd_alive(void)
{
	ioctl(_watchdog_fd, WDIOC_KEEPALIVE, 0);

	return 0;
}



static int _disable_wd(void)
{
	unsigned int value = WDIOS_DISABLECARD;

	ioctl(_watchdog_fd, WDIOC_SETOPTIONS, &value);

	return 0;
}



static int _get_wd_delay(int *delay)
{
	ioctl(_watchdog_fd, WDIOC_GETTIMEOUT, delay);

	return 0;
}



static int _set_wd_delay(int delay)
{
	ioctl(_watchdog_fd, WDIOC_SETTIMEOUT, &delay);
	ioctl(_watchdog_fd, WDIOC_KEEPALIVE, 0);

	return 0;
}

