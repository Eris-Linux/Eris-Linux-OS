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

#define CONTACT_PERIOD_PREFIX     "status_upload_period_seconds="
#define CONTAINER_UPDATE_POLICY   "container_update_policy="

#define SYSTEM_UPDATE_STATUS_FILE "/var/run/system-update-status"


// ---------------------- Private method declarations.

static enum MHD_Result get_update_status    (struct MHD_Connection *connection);
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

