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

#define MAX_CONTAINERS  4
#define CONTAINER_LINE  1024

// ---------------------- Private method declarations.

static int init_system_uuid(const char *app);

static enum MHD_Result get_system_model       (struct MHD_Connection *connection);
static enum MHD_Result get_system_type        (struct MHD_Connection *connection);
static enum MHD_Result get_system_uuid        (struct MHD_Connection *connection);
static enum MHD_Result get_system_version     (struct MHD_Connection *connection);
static enum MHD_Result get_system_slots       (struct MHD_Connection *connection);


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
	if ((strcmp(url, "/api/system/model") == 0) && (strcmp(method, "GET") == 0))
		return get_system_model(connection);

	if ((strcmp(url, "/api/system/type") == 0) && (strcmp(method, "GET") == 0))
		return get_system_type(connection);

	if ((strcmp(url, "/api/system/uuid") == 0) && (strcmp(method, "GET") == 0))
		return get_system_uuid(connection);

	if ((strcmp(url, "/api/system/version") == 0) && (strcmp(method, "GET") == 0))
		return get_system_version(connection);

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

