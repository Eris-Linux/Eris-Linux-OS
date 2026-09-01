#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "system-rest-api.h"


// ---------------------- Private macros declarations.

#define UUID_LENGTH     40
#define ERIS_LINE_SIZE  1024

#define SYSTEM_MODEL_FILE         "/usr/share/eris-linux/system-model"
#define SYSTEM_MODEL_TYPE         "/usr/share/eris-linux/system-type"
#define SYSTEM_VERSION_FILE       "/usr/share/eris-linux/system-version"
#define SYSTEM_UUID_PREFIX        "machine_uuid="
#define SYSTEM_UPDATE_STATUS_FILE "/var/run/system-update-status"


// ---------------------- Private method declarations.

static int init_system_uuid(const char *app);

static enum MHD_Result get_system_kernel   (struct MHD_Connection *connection);
static enum MHD_Result get_system_model    (struct MHD_Connection *connection);
static enum MHD_Result get_system_status   (struct MHD_Connection *connection);
static enum MHD_Result get_system_type     (struct MHD_Connection *connection);
static enum MHD_Result get_system_uuid     (struct MHD_Connection *connection);
static enum MHD_Result get_system_version  (struct MHD_Connection *connection);

static enum MHD_Result read_file_first_line_and_send(struct MHD_Connection *connection, const char *filename, const char *error_message);


// ---------------------- Private variables.

// ---------------------- Public methods

int init_system_rest_api(const char *app)
{
	if (app == NULL)
		app = "eris-linux-rest-api";

	if (init_system_uuid(app) != 0)
		return -1;

	return 0;
}



enum MHD_Result system_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcmp(url, "/api/system/kernel") == 0) && (strcmp(method, "GET") == 0))
		return get_system_kernel(connection);

	if ((strcmp(url, "/api/system/model") == 0) && (strcmp(method, "GET") == 0))
		return get_system_model(connection);

	if ((strcmp(url, "/api/system/status") == 0) && (strcmp(method, "GET") == 0))
		return get_system_status(connection);

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
	char *existing_uuid = NULL;
	uuid_t uuid;

	if (read_parameter_value(SYSTEM_UUID_PREFIX, &existing_uuid) == 0) {
		if (existing_uuid != NULL) {
			if (uuid_parse(existing_uuid, uuid) == 0) {
				free(existing_uuid);
				return 0;
			}
			free(existing_uuid);
		}
	}
	char new_uuid[UUID_LENGTH];
	uuid_generate_random(uuid);
	uuid_unparse(uuid, new_uuid);
	if (write_parameter_value(SYSTEM_UUID_PREFIX, new_uuid) != 0) {
		fprintf(stderr, "%s: unable to save system UUID parameter.\n", app);
		return -1;
	}
	return 0;
}



static enum MHD_Result get_system_kernel(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	FILE *pp = popen("/bin/uname -r", "r");
	if (pp == NULL)
		return send_rest_error(connection, "Unable to get kernel version.", 500);

	char line[ERIS_LINE_SIZE];
	if (fgets(line, sizeof(line) - 1, pp) == NULL) {
		pclose(pp);
		return send_rest_error(connection, "Kernel version not found.", 500);
	}
	pclose(pp);

	line[sizeof(line) - 1] = '\0';
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

	return send_rest_response(connection, line);
}



static enum MHD_Result get_system_model(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	FILE *fp = fopen(SYSTEM_MODEL_FILE, "r");
	if (fp == NULL)
		return send_rest_error(connection, "System model file not found.", 500);

	char line[ERIS_LINE_SIZE];
	if (fgets(line, sizeof(line) - 1, fp) == NULL) {
		fclose(fp);
		return send_rest_error(connection, "System model not found.", 500);
	}
	fclose(fp);

	line[sizeof(line) - 1] = '\0';
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

	return send_rest_response(connection, line);
}



static enum MHD_Result get_system_status(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	int status = 1;
	FILE *fp = NULL;
	if ((fp = fopen(SYSTEM_UPDATE_STATUS_FILE, "r")) != NULL) {
		char line[32];
		if (fgets(line, sizeof(line) - 1, fp) != NULL) {
			char *end = NULL;
			long value;
			errno = 0;
			value = strtol(line, &end, 10);
			if ((errno != 0) || ((*end != '\0') && (*end != '\n'))) {
				status = 0;
			} else {
				status = (int) value;
			}
		}
		fclose(fp);
	}

	enum MHD_Result ret = MHD_NO;

	switch (status) {
	case 1:
		ret = send_rest_response(connection, "1 System OK.");
		break;
	case 2:
		ret = send_rest_response(connection, "2 System update install in progress.");
		break;
	case 3:
		ret = send_rest_response(connection, "3 System update install Ok.");
		break;
	case 4:
		ret = send_rest_response(connection, "4 System update install failed.");
		break;
	case 5:
		ret = send_rest_response(connection, "5 System reboot in progress.");
		break;
	default:
		ret = send_rest_error(connection, "Unable read system update status.", 500);
		break;
	}

	return ret;
}



static enum MHD_Result get_system_type(struct MHD_Connection *connection)
{
	return read_file_first_line_and_send(connection, SYSTEM_MODEL_TYPE, "System type file not found.");
}



static enum MHD_Result get_system_uuid(struct MHD_Connection *connection)
{
	char *uuid;

	if (read_parameter_value(SYSTEM_UUID_PREFIX, &uuid) == 0) {
		if (uuid != NULL) {
			int ret = send_rest_response(connection, uuid);
			free(uuid);
			return ret;
		}
	}
	return send_rest_error(connection, "System UUID not found.", 500);
}



static enum MHD_Result get_system_version(struct MHD_Connection *connection)
{
	return read_file_first_line_and_send(connection, SYSTEM_VERSION_FILE, "System version file not found.");

}



static enum MHD_Result read_file_first_line_and_send(struct MHD_Connection *connection, const char *filename, const char *error_message)
{
	if (connection == NULL)
		return MHD_NO;

	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
		return send_rest_error(connection, error_message, 500);

	char line[ERIS_LINE_SIZE];
	if (fgets(line, sizeof(line), fp) == NULL) {
		fclose(fp);
		return send_rest_error(connection, error_message, 500);
	}

	fclose(fp);

	line[strcspn(line, "\n")] = '\0';

	return send_rest_response(connection, line);
}
