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
#include "container-rest-api.h"


// ---------------------- Private macros declarations.

#define UUID_LENGTH 40

#define SYSTEM_CONTAINERS_FILE    "/etc/eris-linux/containers"
#define CONTAINER_UPDATE_POLICY   "container_update_policy="

#define MAX_CONTAINERS  4
#define CONTAINER_LINE  1024

// ---------------------- Private method declarations.

static enum MHD_Result get_container_count    (struct MHD_Connection *connection);
static enum MHD_Result get_container_name     (struct MHD_Connection *connection);
static enum MHD_Result get_container_presence (struct MHD_Connection *connection);
static enum MHD_Result get_container_status   (struct MHD_Connection *connection);
static enum MHD_Result get_container_version  (struct MHD_Connection *connection);
static enum MHD_Result get_container_policy   (struct MHD_Connection *connection);
static enum MHD_Result put_container_policy   (struct MHD_Connection *connection);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_container_rest_api(const char *app)
{
	(void) app;

	return 0;
}



enum MHD_Result container_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcmp(url, "/api/container/count") == 0) && (strcmp(method, "GET") == 0))
		return get_container_count(connection);

	if ((strcmp(url, "/api/container/name") == 0) && (strcmp(method, "GET") == 0))
		return get_container_name(connection);

	if ((strcmp(url, "/api/container/presence") == 0) && (strcmp(method, "GET") == 0))
		return get_container_presence(connection);

	if ((strcmp(url, "/api/container/status") == 0) && (strcmp(method, "GET") == 0))
		return get_container_status(connection);

	if ((strcmp(url, "/api/container/version") == 0) && (strcmp(method, "GET") == 0))
		return get_container_version(connection);

	if ((strcmp(url, "/api/container/policy") == 0) && (strcmp(method, "GET") == 0))
		return get_container_policy(connection);

	if ((strcmp(url, "/api/container/policy") == 0) && (strcmp(method, "PUT") == 0))
		return put_container_policy(connection);


	return MHD_NO;
}


// ---------------------- Private methods

static enum MHD_Result get_container_count(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	char line[CONTAINER_LINE];
	snprintf(line, sizeof(line) - 1, "%d", MAX_CONTAINERS);
	line[sizeof(line) - 1] = '\0';

	return send_rest_response(connection, line);
}



static enum MHD_Result get_container_name(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
		return send_rest_error(connection, "Missing slot index.", 400);

	errno = 0;
	char *ptr;
	long int cnt = strtol(container_num, &ptr, 10);
	if ((ptr == container_num) || (errno != 0))
		return send_rest_error(connection, "Invalid slot index.", 400);

	char line[CONTAINER_LINE];
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, sizeof(line) - 1, "Slot index must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[sizeof(line) - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}

	FILE *fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, sizeof(line) - 1, fp) == NULL)
			break;
	fclose(fp);

	if (c != cnt + 1)
		return send_rest_error(connection, "Containers description is incomplete.", 500);

	if ((line[0] == '\0') || ((line[0] == '-') && (line[1] == '1')))
		return send_rest_response(connection, "");

	if (line[strlen(line) - 1] == '\n')
		line[strlen(line) - 1] = '\0';
	int i;
	for (i = 0; (i < sizeof(line) - 1) && (line[i] != '!'); i++)
		; // container id
	if ((i == sizeof(line) - 1) || (line[i] !='!'))
		return send_rest_error(connection, "Containers description is inconsistant.", 500);

	int start = i + 1;
	for (i ++; (i < sizeof(line) - 1) && (line[i] != '!'); i++)
		; // container name

	if ((i == sizeof(line) - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';

	return send_rest_response(connection, &(line[start]));
}



static enum MHD_Result get_container_presence(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	errno = 0;
	char *ptr;
	long int cnt = strtol(container_num, &ptr, 10);
	if ((ptr == container_num) || (errno != 0))
		return send_rest_error(connection, "Invalid slot index.", 400);

	char line[CONTAINER_LINE];
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, sizeof(line) - 1, "Slot index must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[sizeof(line) - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}

	FILE *fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, CONTAINER_LINE - 1, fp) == NULL)
			break;
	fclose(fp);
	if (c != cnt + 1)
		return send_rest_error(connection, "Containers description is incomplete.", 500);

	if ((line[0] == '\0') || ((line[0] == '-') && (line[1] == '1')))
		return send_rest_response(connection, "absent");

	return send_rest_response(connection, "present");
}



static enum MHD_Result get_container_status(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	errno = 0;
	char *ptr;
	long int cnt = strtol(container_num, &ptr, 10);
	if ((ptr == container_num) || (errno != 0))
		return send_rest_error(connection, "Invalid slot index.", 400);

	char line[CONTAINER_LINE];
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, sizeof(line) - 1, "Slot index must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[sizeof(line) - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}

	char slotname[16];
	snprintf(slotname, sizeof(slotname) - 1, "slot-%ld", cnt + 1);

	FILE *fp = popen("docker ps", "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to communicate with docker.", 500);

	int found = 0;
	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		if (strstr(line, slotname) != NULL) {
			found = 1;
			break;
		}
	}
	pclose(fp);

	return send_rest_response(connection, found ? "running" : "stopped");
}



static enum MHD_Result get_container_version(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	errno = 0;
	char *ptr;
	long int cnt = strtol(container_num, &ptr, 10);
	if ((ptr == container_num) || (errno != 0))
		return send_rest_error(connection, "Invalid slot index.", 400);

	char line[CONTAINER_LINE];
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, sizeof(line) - 1, "Slot index must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[sizeof(line) - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}

	FILE *fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
	        return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, sizeof(line) - 1, fp) == NULL)
			break;
	fclose(fp);
	if (c != cnt + 1)
		return send_rest_error(connection, "Containers description is incomplete.", 500);

	if (((line[0] == '-') && (line[1] == '1')) || (line[0] == '\0'))
		return send_rest_response(connection, "");

	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

	int i;
	for (i = 0; (i < sizeof(line) - 1) && (line[i] != '!'); i++)
		; // container id

	if ((i == sizeof(line) - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);

	int start = i + 1;
	for (i ++; (i < sizeof(line) - 1) && (line[i] != '!'); i++)
		; // container name

	if ((i == sizeof(line) - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';


	start = i + 1;
	for (i ++; (i < sizeof(line) - 1) && (line[i] != '!'); i++)
		// container version
		;
	if ((i == sizeof(line) - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';

	return send_rest_response(connection, &(line[start]));
}



static enum MHD_Result get_container_policy(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	char *reply = NULL;

	if ((read_parameter_value(CONTAINER_UPDATE_POLICY, &reply) != 0) || (reply == NULL))
	 	return send_rest_response(connection, "immediate");

	enum MHD_Result ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result put_container_policy(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	const char *policy = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "policy");
	if (policy == NULL)
		return send_rest_error(connection, "Missing 'policy' parameter'.", 400);

	if ((strcmp(policy, "immediate") != 0) && (strcmp(policy, "atreboot") != 0))
		return send_rest_error(connection, "Container update policy must be 'immediate' or 'atreboot'.", 400);

	if (write_parameter_value(CONTAINER_UPDATE_POLICY, policy) != 0)
		return send_rest_error(connection, "Unable to save container update policy.", 500);

	return send_rest_response(connection, "Ok");
}
