#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <uuid/uuid.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "update-rest-api.h"


// ---------------------- Private macros declarations.

#define AUTOMATIC_REBOOT_PREFIX   "automatic_reboot_after_update="
#define CONTACT_PERIOD_PREFIX     "status_upload_period_seconds="
#define CONTAINER_UPDATE_POLICY   "container_update_policy="

#define REBOOT_NEEDED_FLAG_FILE   "/tmp/reboot-is-needed"
#define SYSTEM_UPDATE_STATUS_FILE "/tmp/system-update-status"

#define SERVER_CONTACT_FIFO       "/tmp/contact-eris-server"


// ---------------------- Private method declarations.

static enum MHD_Result get_update_status    (struct MHD_Connection *connection);
static enum MHD_Result get_pending_reboot   (struct MHD_Connection *connection);
static enum MHD_Result set_pending_reboot   (struct MHD_Connection *connection);
static enum MHD_Result get_automatic_reboot (struct MHD_Connection *connection);
static enum MHD_Result set_automatic_reboot (struct MHD_Connection *connection);
static enum MHD_Result set_reboot_now       (struct MHD_Connection *connection);
static enum MHD_Result get_contact_period   (struct MHD_Connection *connection);
static enum MHD_Result set_contact_period   (struct MHD_Connection *connection);
static enum MHD_Result set_contact_now      (struct MHD_Connection *connection);
static enum MHD_Result rollback             (struct MHD_Connection *connection);
static enum MHD_Result back_to_factory      (struct MHD_Connection *connection);
static enum MHD_Result get_container_policy (struct MHD_Connection *connection);
static enum MHD_Result set_container_policy (struct MHD_Connection *connection);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_update_rest_api(const char *app)
{
	return 0;
}



enum MHD_Result update_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcasecmp(url, "/api/update/status") == 0) && (strcmp(method, "GET") == 0))
		return get_update_status(connection);

	if ((strcasecmp(url, "/api/update/reboot/automatic") == 0) && (strcmp(method, "GET") == 0))
		return get_automatic_reboot(connection);
	if ((strcasecmp(url, "/api/update/reboot/automatic") == 0) && (strcmp(method, "PUT") == 0))
		return set_automatic_reboot(connection);

	if ((strcasecmp(url, "/api/update/contact/period") == 0) && (strcmp(method, "GET") == 0))
		return get_contact_period(connection);
	if ((strcasecmp(url, "/api/update/contact/period") == 0) && (strcmp(method, "PUT") == 0))
		return set_contact_period(connection);

	if ((strcasecmp(url, "/api/update/contact/now") == 0) && (strcmp(method, "POST") == 0))
		return set_contact_now(connection);

	if ((strcasecmp(url, "/api/update/rollback") == 0) && (strcmp(method, "POST") == 0))
		return rollback(connection);
	if ((strcasecmp(url, "/api/update/factory") == 0) && (strcmp(method, "POST") == 0))
		return back_to_factory(connection);

	if ((strcasecmp(url, "/api/update/reboot/pending") == 0) && (strcmp(method, "GET") == 0))
		return get_pending_reboot(connection);
	if ((strcasecmp(url, "/api/update/reboot/pending") == 0) && (strcmp(method, "PUT") == 0))
		return set_pending_reboot(connection);
	if ((strcasecmp(url, "/api/update/reboot/now") == 0) && (strcmp(method, "POST") == 0))
		return set_reboot_now(connection);

	if ((strcasecmp(url, "/api/update/container/policy") == 0) && (strcmp(method, "GET") == 0))
		return get_container_policy(connection);
	if ((strcasecmp(url, "/api/update/container/policy") == 0) && (strcmp(method, "PUT") == 0))
		return set_container_policy(connection);

	return MHD_NO;
}


// ---------------------- Private methods

static enum MHD_Result get_update_status(struct MHD_Connection *connection)
{
	FILE *fp = NULL;

	int status = 0;
	if ((fp = fopen(SYSTEM_UPDATE_STATUS_FILE, "r")) != NULL) {
		char line[32];
		if (fgets(line, 32, fp) != NULL) {
			if (sscanf(line, "%d", &status) != 1)
				status = 0;
		}
		fclose(fp);
	}

	switch (status) {
	case 1:
		return send_rest_response(connection, "1 System OK.");
	case 2:
		return send_rest_response(connection, "2 System update install in progress.");
	case 3:
		return send_rest_response(connection, "3 System update install Ok.");
	case 4:
		return send_rest_response(connection, "4 System update install failed.");
	case 5:
		return send_rest_response(connection, "5 System reboot in progress.");
	default:
		break;
	}
	return send_rest_error(connection, "Unable read system update status.", 500);
}



static enum MHD_Result get_pending_reboot(struct MHD_Connection *connection)
{
	if (access(REBOOT_NEEDED_FLAG_FILE, F_OK) == 0)
		return send_rest_response(connection, "yes");
	return send_rest_response(connection, "no");
}



static enum MHD_Result set_pending_reboot(struct MHD_Connection *connection)
{
	const char *reboot_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "reboot");
	if (reboot_str == NULL)
        return send_rest_error(connection, "Missing 'reboot' parameter'.", 400);

	if ((reboot_str[0] == 'y') || (reboot_str[0] == 'Y')) {
		FILE *fp = fopen(REBOOT_NEEDED_FLAG_FILE, "w");
		if (fp)
			fclose(fp);
		return send_rest_response(connection, "Ok");
	}
	
	if ((reboot_str[0] == 'n') || (reboot_str[0] == 'N')) {
		unlink(REBOOT_NEEDED_FLAG_FILE);
		return send_rest_response(connection, "Ok");
	}

	return send_rest_error(connection, "Wrong 'reboot' parameter value.", 400);
}



static enum MHD_Result get_automatic_reboot(struct MHD_Connection *connection)
{
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



static enum MHD_Result set_automatic_reboot(struct MHD_Connection *connection)
{
	const char *auto_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "auto");
	if (auto_str == NULL)
        return send_rest_error(connection, "Missing 'auto' parameter'.", 400);

	const char *value = "no";
	if ((auto_str[0] == 'y') || (auto_str[0] == 'Y'))
		value = "y";
	else
		value = "n";

	if (write_parameter_value(AUTOMATIC_REBOOT_PREFIX, value) != 0)
		return send_rest_error(connection, "Unable to store autoreboot parameter.", 500);

	return send_rest_response(connection, "Ok");
}



static enum MHD_Result set_reboot_now(struct MHD_Connection *connection)
{
	send_rest_response(connection, "Ok");
	sync();
	reboot(RB_AUTOBOOT);
	return send_rest_error(connection, "Unable to reboot the system.", 500);
}



static enum MHD_Result get_contact_period(struct MHD_Connection *connection)
{
	char *reply = NULL;

	if ((read_parameter_value(CONTACT_PERIOD_PREFIX, &reply) != 0) || (reply == NULL))
		return send_rest_response(connection, "0");

	enum MHD_Result ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result set_contact_period(struct MHD_Connection *connection)
{
	const char *period_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "period");
	if (period_str == NULL)
        return send_rest_error(connection, "Missing 'period' parameter'.", 400);

	int i;
	if ((sscanf(period_str, "%d", &i) != 1) || (i < 0) || (i > 86400))
		return send_rest_error(connection, "Server contact period must be in [0-86400] seconds.", 400);

	if (write_parameter_value(CONTACT_PERIOD_PREFIX, period_str) != 0)
		return send_rest_error(connection, "Unable to save server contact period.", 500);

	return send_rest_response(connection, "Ok");
}



static enum MHD_Result set_contact_now(struct MHD_Connection *connection)
{
	int fd = open(SERVER_CONTACT_FIFO, O_NONBLOCK | O_WRONLY);
	if (fd >= 0) {
		write(fd, "E", 1);
		close(fd);
		return send_rest_response(connection, "Ok");
	}
	return send_rest_error(connection, "Unable to trigger a server contact.", 500);
}



static enum MHD_Result rollback(struct MHD_Connection *connection)
{
	return send_rest_error(connection, "Feature not implemented yet.", 501);
}



static enum MHD_Result back_to_factory(struct MHD_Connection *connection)
{
	return send_rest_error(connection, "Feature not implemented yet.", 501);
}



static enum MHD_Result get_container_policy(struct MHD_Connection *connection)
{
	char *reply = NULL;

	if ((read_parameter_value(CONTAINER_UPDATE_POLICY, &reply) != 0) || (reply == NULL))
	 	return send_rest_response(connection, "immediate");

	enum MHD_Result ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result set_container_policy(struct MHD_Connection *connection)
{
	const char *policy = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "policy");
	if (policy == NULL)
		return send_rest_error(connection, "Missing 'policy' parameter'.", 400);

	if ((strcmp(policy, "immediate") != 0) && (strcmp(policy, "atreboot") != 0))
		return send_rest_error(connection, "Container update policy must be 'immediate' or 'atreboot'.", 400);

	if (write_parameter_value(CONTAINER_UPDATE_POLICY, policy) != 0)
		return send_rest_error(connection, "Unable to save container update policy.", 500);

	return send_rest_response(connection, "Ok");
}

