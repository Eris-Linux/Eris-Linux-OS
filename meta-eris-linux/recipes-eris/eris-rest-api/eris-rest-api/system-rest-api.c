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
#include "system-rest-api.h"


// ---------------------- Private macros declarations.

#define UUID_LENGTH 40

#define SYSTEM_MODEL_FILE       "/usr/share/eris-linux/system-model"
#define SYSTEM_MODEL_TYPE       "/usr/share/eris-linux/system-type"
#define SYSTEM_VERSION_FILE     "/usr/share/eris-linux/system-version"
#define SYSTEM_UUID_PREFIX      "machine_uuid="
#define SYSTEM_CONTAINERS_FILE  "/etc/eris-linux/containers"

#define MAX_CONTAINERS  4
#define CONTAINER_LINE  1024

// ---------------------- Private method declarations.

static int init_system_uuid(const char *app);

static enum MHD_Result get_system_model       (struct MHD_Connection *connection);
static enum MHD_Result get_system_type        (struct MHD_Connection *connection);
static enum MHD_Result get_system_uuid        (struct MHD_Connection *connection);
static enum MHD_Result get_system_version     (struct MHD_Connection *connection);
static enum MHD_Result get_system_slots       (struct MHD_Connection *connection);
static enum MHD_Result get_container_name     (struct MHD_Connection *connection);
static enum MHD_Result get_container_presence (struct MHD_Connection *connection);
static enum MHD_Result get_container_status   (struct MHD_Connection *connection);
static enum MHD_Result get_container_version  (struct MHD_Connection *connection);

//static enum MHD_Result put_system_reset      (struct MHD_Connection *connection);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_system_rest_api(const char *app)
{
	if (init_system_uuid(app) != 0)
		return -1;

	return 0;
}



enum MHD_Result system_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcasecmp(url, "/api/system/model") == 0) && (strcmp(method, "GET") == 0))
		return get_system_model(connection);
	if ((strcasecmp(url, "/api/system/type") == 0) && (strcmp(method, "GET") == 0))
		return get_system_type(connection);
	if ((strcasecmp(url, "/api/system/uuid") == 0) && (strcmp(method, "GET") == 0))
		return get_system_uuid(connection);
	if ((strcasecmp(url, "/api/system/version") == 0) && (strcmp(method, "GET") == 0))
		return get_system_version(connection);
	if ((strcasecmp(url, "/api/container/count") == 0) && (strcmp(method, "GET") == 0))
		return get_system_slots(connection);
	if ((strcasecmp(url, "/api/container/name") == 0) && (strcmp(method, "GET") == 0))
		return get_container_name(connection);
	if ((strcasecmp(url, "/api/container/presence") == 0) && (strcmp(method, "GET") == 0))
		return get_container_presence(connection);
	if ((strcasecmp(url, "/api/container/status") == 0) && (strcmp(method, "GET") == 0))
		return get_container_status(connection);
	if ((strcasecmp(url, "/api/container/version") == 0) && (strcmp(method, "GET") == 0))
		return get_container_version(connection);

	return MHD_NO;
}


// ---------------------- Private methods

static int init_system_uuid(const char *app)
{
	char *uuid_string = NULL;
	uuid_t uuid;

	if (read_parameter_value(SYSTEM_UUID_PREFIX, &uuid_string) == 0) {
		if (uuid_parse(uuid_string, uuid) == 0) {
			free(uuid_string);
			return 0;
		}
		free(uuid_string);
	}
	uuid_string = malloc(64);
	if (uuid_string == NULL) {
		fprintf(stderr, "%s: not enough memory to allocate system UUID.\n", app);
		return -1;
	}
	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_string);
	if (write_parameter_value(SYSTEM_UUID_PREFIX, uuid_string) != 0) {
		fprintf(stderr, "%s: unable to save system UUID parameter.\n", app);
		free(uuid_string);
		return -1;
	}
	free(uuid_string);
	return 0;
}



static enum MHD_Result get_system_model(struct MHD_Connection *connection)
{
	FILE *fp;
	char line[CONTAINER_LINE];

	fp = fopen(SYSTEM_MODEL_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "System model file not found.", 500);

	if (fgets(line, CONTAINER_LINE - 1, fp) == NULL) {
		fclose(fp);
		send_rest_error(connection, "System model not found.", 500);
	}

	line[CONTAINER_LINE - 1] = '\0';
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
	fclose(fp);
	return send_rest_response(connection, line);
}



static enum MHD_Result get_system_type(struct MHD_Connection *connection)
{
	FILE *fp;
	char line[CONTAINER_LINE];

	fp = fopen(SYSTEM_MODEL_TYPE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "System type file not found.", 500);

	if (fgets(line, CONTAINER_LINE - 1, fp) == NULL) {
		fclose(fp);
		send_rest_error(connection, "System type not found.", 500);
	}

	line[CONTAINER_LINE - 1] = '\0';
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
	fclose(fp);
	return send_rest_response(connection, line);
}



static enum MHD_Result get_system_uuid(struct MHD_Connection *connection)
{
	char *uuid_string = NULL;

	if (read_parameter_value(SYSTEM_UUID_PREFIX, &uuid_string) != 0)
		return send_rest_error(connection, "System UUID not found.", 500);

	if (uuid_string == NULL)
		return send_rest_error(connection, "No system UUID defined.", 404);

	int ret = send_rest_response(connection, uuid_string);
	free(uuid_string);
	return ret;
}



static enum MHD_Result get_system_version(struct MHD_Connection *connection)
{
	FILE *fp;
	char line[CONTAINER_LINE];

	fp = fopen(SYSTEM_VERSION_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "System version file not found.", 500);

	if (fgets(line, CONTAINER_LINE - 1, fp) == NULL) {
		fclose(fp);
		send_rest_error(connection, "System version not found.", 500);
	}

	line[CONTAINER_LINE - 1] = '\0';
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
	fclose(fp);
	return send_rest_response(connection, line);
}



static enum MHD_Result get_system_slots(struct MHD_Connection *connection)
{
	char line[CONTAINER_LINE];

	snprintf(line, CONTAINER_LINE - 1, "%d", MAX_CONTAINERS);
	line[CONTAINER_LINE - 1] = '\0';

	return send_rest_response(connection, line);
}



static enum MHD_Result get_container_name(struct MHD_Connection *connection)
{
	int cnt;
	char line[CONTAINER_LINE];
	FILE *fp;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
		return send_rest_error(connection, "Missing slot index.", 400);

	if (sscanf(container_num, "%d", &cnt) != 1)
		return send_rest_error(connection, "Invalid slot index.", 400);

	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, CONTAINER_LINE - 1, "Slot index must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[CONTAINER_LINE - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}
	
	fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, CONTAINER_LINE - 1, fp) == NULL)
			break;
	fclose(fp);
	if (c != cnt + 1)
		return send_rest_error(connection, "Containers description is incomplete.", 500);

	if (((line[0] == '-') && (line[1] == '1')) || (line[0] == '\0'))
		return send_rest_response(connection, "");

	if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';
	int i;
	for (i = 0; (i < CONTAINER_LINE - 1) && (line[i] != '!'); i++)
		// container id
		;
	if ((i == CONTAINER_LINE - 1) || (line[i] !='!'))
		return send_rest_error(connection, "Containers description is inconsistant.", 500);

	int start = i + 1;
	for (i ++; (i < CONTAINER_LINE - 1) && (line[i] != '!'); i++)
		// container name
		;
	if ((i == CONTAINER_LINE - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';

	return send_rest_response(connection, &(line[start]));
}



static enum MHD_Result get_container_presence(struct MHD_Connection *connection)
{
	int cnt;
	char line[CONTAINER_LINE];
	FILE *fp;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	if (sscanf(container_num, "%d", &cnt) != 1)
	        return send_rest_error(connection, "Invalid container number.", 400);

	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, CONTAINER_LINE - 1, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[CONTAINER_LINE - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}
	fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, CONTAINER_LINE - 1, fp) == NULL)
			break;
	fclose(fp);
	if (c != cnt + 1)
		return send_rest_error(connection, "Containers description is incomplete.", 500);
	
	if (((line[0] == '-') && (line[1] == '1')) || (line[0] == '\0'))
		return send_rest_response(connection, "absent");
	return send_rest_response(connection, "present");
}



static enum MHD_Result get_container_status(struct MHD_Connection *connection)
{
	int cnt;
	char line[CONTAINER_LINE];
	FILE *fp;
	int found = 0;
	char slotname[16];

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	if (sscanf(container_num, "%d", &cnt) != 1)
	        return send_rest_error(connection, "Invalid container number.", 400);

	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, CONTAINER_LINE - 1, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[CONTAINER_LINE - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}

	snprintf(slotname, 15, "slot-%d", cnt + 1);

	fp = popen("docker ps", "r");
	if (fp == NULL)
		return send_rest_error(connection, "Unable to communicate with docker.", 500);
	while (fgets(line, CONTAINER_LINE - 1, fp) != NULL) {
		if (strstr(line, slotname) != NULL) {
			found = 1;
			break;
		}
	}
	fclose(fp);

	return send_rest_response(connection, found ? "running" : "stopped");
}



static enum MHD_Result get_container_version(struct MHD_Connection *connection)
{
	int cnt;
	char line[CONTAINER_LINE];
	FILE *fp;

	const char *container_num = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "index");
	if (container_num == NULL)
	        return send_rest_error(connection, "Missing container number.", 400);

	if (sscanf(container_num, "%d", &cnt) != 1)
	        return send_rest_error(connection, "Invalid container number.", 400);

	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, CONTAINER_LINE - 1, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[CONTAINER_LINE - 1] = '\0';
		return send_rest_error(connection, line, 400);
	}
	fp = fopen(SYSTEM_CONTAINERS_FILE, "r");
	if (fp == NULL)
	        return send_rest_error(connection, "Unable to open containers description.", 500);

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, CONTAINER_LINE - 1, fp) == NULL)
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
	for (i = 0; (i < CONTAINER_LINE - 1) && (line[i] != '!'); i++)
		// container id
		;
	if ((i == CONTAINER_LINE - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);

	int start = i + 1;
	for (i ++; (i < CONTAINER_LINE - 1) && (line[i] != '!'); i++)
		// container name
		;
	if ((i == CONTAINER_LINE - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';


	start = i + 1;
	for (i ++; (i < CONTAINER_LINE - 1) && (line[i] != '!'); i++)
		// container version
		;
	if ((i == CONTAINER_LINE - 1) || (line[i] !='!'))
	        return send_rest_error(connection, "Containers description is inconsistant.", 500);
	line[i] = '\0';

	return send_rest_response(connection, &(line[start]));
}

