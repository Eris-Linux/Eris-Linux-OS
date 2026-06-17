/*
 *  ERIS LINUX API
 *
 *  Author: Christophe BLAESS.
 *
 *  (C) Logilin 2024-2026. All rights reserved
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "eris-rest-api.h"
#include "contact-rest-api.h"


// ---------------------- Private macros declarations.

#define CONTACT_PERIOD_PREFIX     "status_upload_period_seconds="
#define SERVER_CONTACT_FIFO       "/var/run/contact-eris-server"


// ---------------------- Private method declarations.

static enum MHD_Result get_contact_period   (struct MHD_Connection *connection);
static enum MHD_Result put_contact_period   (struct MHD_Connection *connection);
static enum MHD_Result post_contact_now     (struct MHD_Connection *connection);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_contact_rest_api(const char *app)
{
	(void) app;
	return 0;
}



enum MHD_Result contact_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcmp(url, "/api/contact/period") == 0) && (strcmp(method, "GET") == 0))
		return get_contact_period(connection);

	if ((strcmp(url, "/api/contact/period") == 0) && (strcmp(method, "PUT") == 0))
		return put_contact_period(connection);

	if ((strcmp(url, "/api/contact/now") == 0) && (strcmp(method, "POST") == 0))
		return post_contact_now(connection);

	return MHD_NO;
}


// ---------------------- Private methods

static enum MHD_Result get_contact_period(struct MHD_Connection *connection)
{
	char *reply = NULL;

	if (connection != NULL) {
		if (read_parameter_value(CONTACT_PERIOD_PREFIX, &reply) == 0) {
			if (reply != NULL) {
				enum MHD_Result ret = send_rest_response(connection, reply);
				free(reply);
				return ret;
			}
		}
		return send_rest_response(connection, "0");
	}
	return MHD_NO;
}



static enum MHD_Result put_contact_period(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *period_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "period");
	if (period_str == NULL)
		return send_rest_error(connection, "Missing 'period' parameter.", 400);

	long int value;
	char *end;
	errno = 0;
	value = strtol(period_str, &end, 10);
	if ((errno != 0) || (*end != '\0') || (value < 0) || (value > 86400))
		return send_rest_error(connection, "Server contact period must be in [0-86400] seconds.", 400);

	if (write_parameter_value(CONTACT_PERIOD_PREFIX, period_str) != 0)
		return send_rest_error(connection, "Unable to save server contact period.", 500);

	return send_rest_response(connection, "Ok");
}



static enum MHD_Result post_contact_now(struct MHD_Connection *connection)
{
	int fd = open(SERVER_CONTACT_FIFO, O_NONBLOCK | O_WRONLY);
	if (fd >= 0) {
		int ret = write(fd, "E", 1);
		close(fd);
		if (ret == 1)
			return send_rest_response(connection, "Ok");
	}
	return send_rest_error(connection, "Unable to trigger a server contact.", 500);
}

