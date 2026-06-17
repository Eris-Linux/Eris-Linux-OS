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
#include "watchdog-rest-api.h"


// ---------------------- Private macros declarations.

#define WATCHDOG_FILE   "/dev/watchdog0"
#define WATCHDOG_DELAY_PREFIX "watchdog_delay="


// ---------------------- Private types definitions.

// ---------------------- Private method declarations.

static enum MHD_Result post_watchdog          (struct MHD_Connection *connection);
static enum MHD_Result delete_watchdog        (struct MHD_Connection *connection);
static enum MHD_Result get_watchdog_delay     (struct MHD_Connection *connection);
static enum MHD_Result put_watchdog_delay     (struct MHD_Connection *connection);
static enum MHD_Result post_watchdog_feeder   (struct MHD_Connection *connection);
static enum MHD_Result delete_watchdog_feeder (struct MHD_Connection *connection);
static enum MHD_Result get_watchdog_feeder    (struct MHD_Connection *connection);


static void *_feeder_function(void *arg);
static int   _keep_wd_alive(void);
static int   _disable_wd(void);
static int   _get_wd_delay(int *delay);
static int   _set_wd_delay(int delay);


// ---------------------- Private variables declarations.

static int       _feeder_running = 0;
static pthread_t _feeder_thread;
static int       _feeder_has_to_stop = 0;
static int       _watchdog_fd = -1;
static pthread_mutex_t _watchdog_mtx = PTHREAD_MUTEX_INITIALIZER;


// ---------------------- Public methods definitions.

int init_watchdog_rest_api(const char *name)
{
	(void) name;
	char *delay_line = NULL;
	long int delay;

	pthread_mutex_lock(&_watchdog_mtx);
	_watchdog_fd = open(WATCHDOG_FILE, O_RDWR);
	if (_watchdog_fd < 0) {
		pthread_mutex_unlock(&_watchdog_mtx);
		return -1;
	}
	pthread_mutex_unlock(&_watchdog_mtx);

	if (read_parameter_value(WATCHDOG_DELAY_PREFIX, &delay_line) == 0) {
		if (delay_line != NULL) {
			if (sscanf(delay_line, "%ld", &delay) == 1) {
				_set_wd_delay(delay);
			}
			free(delay_line);
		}
	}

	pthread_mutex_lock(&_watchdog_mtx);
	_feeder_has_to_stop = 0;
	_feeder_running = 1;
	if (pthread_create(&_feeder_thread, NULL, _feeder_function, NULL) != 0) {
		_feeder_running = 0;
		close(_watchdog_fd);
		_watchdog_fd = -1;
		pthread_mutex_unlock(&_watchdog_mtx);
		return -1;
	}
	pthread_mutex_unlock(&_watchdog_mtx);

	return 0;
}



enum MHD_Result watchdog_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcmp(url, "/api/watchdog") == 0) {
		if (strcmp(method, "POST") == 0)
			return post_watchdog(connection);
		if (strcmp(method, "DELETE") == 0)
			return delete_watchdog(connection);
	}

	if (strcmp(url, "/api/watchdog/delay") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_watchdog_delay(connection);
		if (strcmp(method, "PUT") == 0)
			return put_watchdog_delay(connection);
	}

	if (strcmp(url, "/api/watchdog/feeder") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_watchdog_feeder(connection);
		if (strcmp(method, "POST") == 0)
			return post_watchdog_feeder(connection);
		if (strcmp(method, "DELETE") == 0)
			return delete_watchdog_feeder(connection);
	}

	return MHD_NO;
}


// ---------------------- Private methods definitions.

static enum MHD_Result post_watchdog(struct MHD_Connection *connection)
{
	// Feed the watchdog.
	//
	if (_keep_wd_alive() == 0)
		return send_rest_response(connection, "Ok");
	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result delete_watchdog(struct MHD_Connection *connection)
{
	// Disable the watchdog.
	//
	pthread_mutex_lock(&_watchdog_mtx);
	if (_feeder_running) {
		_feeder_has_to_stop = 1;
		pthread_mutex_unlock(&_watchdog_mtx);
		pthread_join(_feeder_thread, NULL);
	} else {
		pthread_mutex_unlock(&_watchdog_mtx);
	}

	if (_disable_wd() == 0)
		return send_rest_response(connection, "Ok");
	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result get_watchdog_delay(struct MHD_Connection *connection)
{
	// Read the watchdog delay.
	//
	int delay;
	if (_get_wd_delay(&delay) < 0)
		return send_rest_error(connection, "No watchdog available", 500);

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%d", delay);
	return send_rest_response(connection, buffer);
}



static enum MHD_Result put_watchdog_delay (struct MHD_Connection *connection)
{
	// Set the watchdog delay.
	//
	const char *delay_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "delay");
	if (delay_str == NULL)
        return send_rest_error(connection, "Missing delay.", 400);

	int delay;
	if ((sscanf(delay_str, "%d", &delay) != 1)
	 || (delay < 1)
	 || (delay > 48))
		return send_rest_error(connection, "Invalid delay.", 400);

	if (_set_wd_delay(delay) == 0)
		return send_rest_response(connection, "Ok");

	return send_rest_error(connection, "No watchdog available", 500);
}



static enum MHD_Result post_watchdog_feeder(struct MHD_Connection *connection)
{
	// Start the watchdog feeder thread.
	//
	pthread_mutex_lock(&_watchdog_mtx);
	if (_feeder_running) {
		pthread_mutex_unlock(&_watchdog_mtx);
		return send_rest_error(connection, "Already running", 400);
	}

	_feeder_has_to_stop = 0;
	_feeder_running = 1;
	if (pthread_create(&_feeder_thread, NULL, _feeder_function, NULL) != 0) {
		_feeder_running = 0;
		pthread_mutex_unlock(&_watchdog_mtx);
		return send_rest_error(connection, "Thread error", 500);
	}

	pthread_mutex_unlock(&_watchdog_mtx);
	return send_rest_response(connection, "Ok");
}



static enum MHD_Result delete_watchdog_feeder(struct MHD_Connection *connection)
{
	// Stop the watchdog feeder thread.
	//
	pthread_mutex_lock(&_watchdog_mtx);
	if (_feeder_running) {
		_feeder_has_to_stop = 1;
		pthread_mutex_unlock(&_watchdog_mtx);
		pthread_join(_feeder_thread, NULL);
		return send_rest_response(connection, "Ok");
	}
	pthread_mutex_unlock(&_watchdog_mtx);

	return send_rest_error(connection, "Already stopped", 400);
}



static enum MHD_Result get_watchdog_feeder(struct MHD_Connection *connection)
{
	// Get watchdog feeder status.
	//
	pthread_mutex_lock(&_watchdog_mtx);
	if (_feeder_running) {
		pthread_mutex_unlock(&_watchdog_mtx);
		return send_rest_response(connection, "running");
	}
	pthread_mutex_unlock(&_watchdog_mtx);
	return send_rest_response(connection, "stopped");
}



static void *_feeder_function(void *arg)
{
	// Thread to feed the watchdog every seconds.
	//
	for (;;) {
		pthread_mutex_lock(&_watchdog_mtx);
		if (_feeder_has_to_stop) {
			_feeder_running = 0;
			pthread_mutex_unlock(&_watchdog_mtx);
			break;
		}
		pthread_mutex_unlock(&_watchdog_mtx);

		_keep_wd_alive();

		sleep(1);
	}
	return NULL;
}



static int _keep_wd_alive(void)
{
	// Low-level command to feed the watchdog.
	//
	int ret = -1;

	pthread_mutex_lock(&_watchdog_mtx);

	if (_watchdog_fd < 0)
		goto out_unlock;

	if (ioctl(_watchdog_fd, WDIOC_KEEPALIVE, 0) < 0)
		goto out_unlock;

	ret = 0;

	out_unlock:

	pthread_mutex_unlock(&_watchdog_mtx);

	return ret;
}



static int _disable_wd(void)
{
	// Low-level command to disable the watchdog.
	//
	int ret = -1;
	unsigned int value = WDIOS_DISABLECARD;

	pthread_mutex_lock(&_watchdog_mtx);

	if (_watchdog_fd < 0)
		goto out_unlock;

	if (ioctl(_watchdog_fd, WDIOC_SETOPTIONS, &value) < 0)
		goto out_unlock;

	ret = 0;

	out_unlock:

	pthread_mutex_unlock(&_watchdog_mtx);

	return ret;
}



static int _get_wd_delay(int *delay)
{
	// Low-level command to get the watchdog delay.
	//
	int ret = -1;

	pthread_mutex_lock(&_watchdog_mtx);

	if (_watchdog_fd < 0)
		goto out_unlock;

	if (ioctl(_watchdog_fd, WDIOC_GETTIMEOUT, delay) < 0)
		goto out_unlock;

	ret = 0;

	out_unlock:

	pthread_mutex_unlock(&_watchdog_mtx);

	return ret;
}



static int _set_wd_delay(int delay)
{
	// Low-level command to set the watchdog delay.
	//
	int ret = -1;

	pthread_mutex_lock(&_watchdog_mtx);

	if (_watchdog_fd < 0)
		goto out_unlock;

	if (ioctl(_watchdog_fd, WDIOC_SETTIMEOUT, &delay) < 0)
		goto out_unlock;

	if (ioctl(_watchdog_fd, WDIOC_KEEPALIVE, 0) < 0)
		goto out_unlock;
	ret = 0;

	out_unlock:

	pthread_mutex_unlock(&_watchdog_mtx);

	return ret;
}

