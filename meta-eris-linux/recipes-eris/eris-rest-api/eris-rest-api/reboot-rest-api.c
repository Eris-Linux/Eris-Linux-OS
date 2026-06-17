/*
 *  ERIS LINUX API
 *
 *  Author: Christophe BLAESS.
 *
 *  (C) Logilin 2024-2026. All rights reserved
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>

#include "eris-rest-api.h"
#include "reboot-rest-api.h"


// ---------------------- Private macros declarations.

#define AUTOMATIC_REBOOT_PREFIX   "automatic_reboot_after_update="
#define REBOOT_NEEDED_FLAG_FILE   "/var/run/reboot-is-needed"


// ---------------------- Private method declarations.

static enum MHD_Result post_reboot_now      (struct MHD_Connection *connection);
static enum MHD_Result get_reboot_pending   (struct MHD_Connection *connection);
static enum MHD_Result post_reboot_pending  (struct MHD_Connection *connection);
static enum MHD_Result get_reboot_automatic (struct MHD_Connection *connection);
static enum MHD_Result put_reboot_automatic (struct MHD_Connection *connection);
static enum MHD_Result post_reboot_rollback (struct MHD_Connection *connection);
static enum MHD_Result post_reboot_factory  (struct MHD_Connection *connection);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_reboot_rest_api(const char *app)
{
	(void) app;

	return 0;
}



enum MHD_Result reboot_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcmp(url, "/api/reboot/now") == 0) && (strcmp(method, "POST") == 0))
		return post_reboot_now(connection);

	if ((strcmp(url, "/api/reboot/pending") == 0) && (strcmp(method, "GET") == 0))
		return get_reboot_pending(connection);
	if ((strcmp(url, "/api/reboot/pending") == 0) && (strcmp(method, "POST") == 0))
		return post_reboot_pending(connection);

	if ((strcmp(url, "/api/reboot/automatic") == 0) && (strcmp(method, "GET") == 0))
		return get_reboot_automatic(connection);
	if ((strcmp(url, "/api/reboot/automatic") == 0) && (strcmp(method, "PUT") == 0))
		return put_reboot_automatic(connection);

	if ((strcmp(url, "/api/reboot/rollback") == 0) && (strcmp(method, "POST") == 0))
		return post_reboot_rollback(connection);
	if ((strcmp(url, "/api/reboot/factory") == 0) && (strcmp(method, "POST") == 0))
		return post_reboot_factory(connection);

	return MHD_NO;
}


// ---------------------- Private methods

static enum MHD_Result post_reboot_now(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	send_rest_response(connection, "Ok");
	sync();
	reboot(RB_AUTOBOOT);

	return send_rest_error(connection, "Unable to reboot the system.", 500);
}



static enum MHD_Result get_reboot_pending(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	if (access(REBOOT_NEEDED_FLAG_FILE, F_OK) == 0)
		return send_rest_response(connection, "yes");

	return send_rest_response(connection, "no");
}



static enum MHD_Result post_reboot_pending(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *reboot_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "reboot");
	if (reboot_str == NULL)
		return send_rest_error(connection, "Missing 'reboot' parameter.", 400);

	if ((reboot_str[0] == 'y') || (reboot_str[0] == 'Y')) {
		int fd = open(REBOOT_NEEDED_FLAG_FILE, O_WRONLY | O_TRUNC | O_CREAT);
		if (fd >= 0) {
			close(fd);
			return send_rest_response(connection, "Ok");
		}
		return send_rest_error(connection, "Unable to program a reboot.", 500);
	}

	if ((reboot_str[0] == 'n') || (reboot_str[0] == 'N')) {
		if (unlink(REBOOT_NEEDED_FLAG_FILE) != 0) {
			if (errno != ENOENT)
				return send_rest_error(connection, "Unable to cancel programmed reboot.", 500);
		}
		return send_rest_response(connection, "Ok");
	}

	return send_rest_error(connection, "Wrong 'reboot' parameter value.", 400);
}



static enum MHD_Result get_reboot_automatic(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	char *reply = NULL;
	if (read_parameter_value(AUTOMATIC_REBOOT_PREFIX, &reply) != 0)
		return send_rest_response(connection, "no");

	if (reply == NULL)
		return send_rest_response(connection, "no");

	enum MHD_Result ret;
	if ((reply[0] == 'y') || (reply[0] == 'Y'))
		ret = send_rest_response(connection, "yes");
	else
		ret = send_rest_response(connection, "no");
	free(reply);

	return ret;
}



static enum MHD_Result put_reboot_automatic(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *auto_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "auto");
	if (auto_str == NULL)
		return send_rest_error(connection, "Missing 'auto' parameter.", 400);

	const char *value;
	if ((auto_str[0] == 'y') || (auto_str[0] == 'Y'))
		value = "y";
	else
		value = "n";

	if (write_parameter_value(AUTOMATIC_REBOOT_PREFIX, value) != 0)
		return send_rest_error(connection, "Unable to store autoreboot parameter.", 500);

	return send_rest_response(connection, "Ok");
}



static enum MHD_Result post_reboot_rollback(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	return send_rest_error(connection, "Feature not implemented yet.", 501);
}



static enum MHD_Result post_reboot_factory(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	return send_rest_error(connection, "Feature not implemented yet.", 501);
}

