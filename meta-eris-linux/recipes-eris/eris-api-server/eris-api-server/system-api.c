/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <uuid/uuid.h>


#include "api-server.h"
#include "system-api.h"


#define MACHINE_TOKEN_PREFIX  "machine_token="
#define MACHINE_UUID_PREFIX   "machine_uuid="

#define MAX_CONTAINERS  4


// ---------------------- Private method declarations.

static void init_machine_uuid(void);

static int  store_machine_uuid(int sock, uuid_t uuid);
static int  read_machine_uuid(int sock, char *uuid_string, size_t size);

static void get_machine_uuid_command(int sock, int argc, char *argv[]);
static void create_machine_uuid_command(int sock, int argc, char *argv[]);
static void set_machine_uuid_command(int sock, int argc, char *argv[]);
static void get_system_type_command(int sock, int argc, char *argv[]);
static void get_system_model_command(int sock, int argc, char *argv[]);
static void get_system_version_command(int sock, int argc, char *argv[]);
static void get_number_of_slots_command(int sock, int argc, char *argv[]);
static void get_container_present_command(int sock, int argc, char *argv[]);
static void get_container_name_command(int sock, int argc, char *argv[]);
static void get_container_version_command(int sock, int argc, char *argv[]);


// ---------------------- Public methods

int init_system_api(void)
{
	init_machine_uuid();

	if (register_api_command("get-machine-uuid",    "guuid", "Get the Universally Unique Identifier of the machine", get_machine_uuid_command))
		return -1;
	if (register_api_command("create-machine-uuid", "cuuid", "Create and store a Universally Unique Identifier for the machine", create_machine_uuid_command))
		return -1;
	if (register_api_command("set-machine-uuid",    "suuid", "Store the Universally Unique Identifier of the machine", set_machine_uuid_command))
		return -1;
	if (register_api_command("get-system-type", "syst", "Get the type of the system image.", get_system_type_command))
		return -1;
	if (register_api_command("get-system-model", "sysm", "Get the current system image model.", get_system_model_command))
		return -1;
	if (register_api_command("get-system-version", "sysv", "Get the current system image version.", get_system_version_command))
		return -1;
	if (register_api_command("get-number-of-slots", "nslt", "Get the number of container slots.", get_number_of_slots_command))
		return -1;
	if (register_api_command("get-container-present", "cntp", "Return 1 if the container is present, 9 otherwise.", get_container_present_command))
		return -1;
	if (register_api_command("get-container-name", "cntn", "Get the container image name.", get_container_name_command))
		return -1;
	if (register_api_command("get-container-version", "cntv", "Get the container image version.", get_container_version_command))
		return -1;

	return 0;
}


// ---------------------- Private methods

static void init_machine_uuid(void)
{
	char *uuid_string = NULL;
	uuid_t uuid;

	if (read_parameter_value(MACHINE_UUID_PREFIX, &uuid_string) == 0) {
		if (uuid_parse(uuid_string, uuid) == 0) {
			free(uuid_string);
			return;
		}
		free(uuid_string);
	}
	uuid_string = malloc(64);
	if (uuid_string == NULL)
		return;
	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_string);
	write_parameter_value(MACHINE_UUID_PREFIX, uuid_string);
	free(uuid_string);
}



static int store_machine_uuid(int sock, uuid_t uuid)
{
	char out[40];

	uuid_unparse(uuid, out);

	return write_parameter_value(MACHINE_UUID_PREFIX, out);
}



static int read_machine_uuid(int sock, char *uuid_string, size_t size)
{
	char *value = NULL;

	if (read_parameter_value(MACHINE_UUID_PREFIX, &value) != 0) {
		return -1;
	}

	if (value != NULL) {
		strncpy(uuid_string, value, size);
		uuid_string[size - 1] = '\0';
		free(value);
	}

	return 0;
}



#define UUID_LENGTH 40

static void get_machine_uuid_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-machine-uuid doesn't take any argument.");
		return;
	}
	char uuid_string[UUID_LENGTH + 1];

	int ret = read_machine_uuid(sock, uuid_string, UUID_LENGTH + 1);
	if (ret < 0) {
		send_error(sock, EINVAL, "unable to read machine uuid.");
		return;
	}
	send_reply(sock, strlen(uuid_string), uuid_string);
}



static void create_machine_uuid_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "create-machine-uuid doesn't take any argument.");
		return;
	}

	uuid_t uuid;
	uuid_generate(uuid);
	if (store_machine_uuid(sock, uuid) != 0) {
		send_error(sock, errno, "Unable to store UUID.");
		return;
	}
	send_reply(sock, 2, "Ok");
}



static void set_machine_uuid_command(int sock, int argc, char *argv[])
{
	uuid_t  uuid;

	if (argc < 1) {
		send_error(sock, EINVAL, "set-machine-uuid needs an argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "set-machine-uuid takes only one argument.");
		return;
	}

	if (uuid_parse(argv[0], uuid) != 0) {
		send_error(sock, errno, "Invalid uuid.");
		return;
	}
	if (store_machine_uuid(sock, uuid) != 0) {
		send_error(sock, errno, "Unable to store UUID.");
		return;
	}
	send_reply(sock, 2, "Ok");
}



static void get_system_version_command(int sock, int argc, char *argv[])
{
	char line[128];
	FILE *fp;
	int found = 0;

	if (argc > 0) {
		send_error(sock, EINVAL, "get-system-version doesn't take any argument.");
		return;
	}
	fp = fopen("/usr/share/eris-linux/system-version", "r");
	if (fp != NULL) {
		if (fgets(line, 127, fp) != NULL) {
			line[127] = '\0';
			if (line[0] != '\0')
				if (line[strlen(line) - 1] == '\n')
					line[strlen(line) - 1] = '\0';
			send_reply(sock, strlen(line), line);
			found = 1;
		}
		fclose(fp);
	}
	if (! found) {
		send_error(sock, ENOSYS, "No system version available.");
	}
}



static void get_system_model_command(int sock, int argc, char *argv[])
{
	char line[128];
	FILE *fp;
	int found = 0;

	if (argc > 0) {
		send_error(sock, EINVAL, "get-system-model doesn't take any argument.");
		return;
	}
	fp = fopen("/usr/share/eris-linux/system-model", "r");
	if (fp != NULL) {
		if (fgets(line, 127, fp) != NULL) {
			line[127] = '\0';
			if (line[0] != '\0')
				if (line[strlen(line) - 1] == '\n')
					line[strlen(line) - 1] = '\0';
			send_reply(sock, strlen(line), line);
			found = 1;
		}
		fclose(fp);
	}
	if (! found) {
		send_error(sock, ENOSYS, "No system model available.");
	}
}



static void get_system_type_command(int sock, int argc, char *argv[])
{
	char line[128];
	FILE *fp;
	int found = 0;

	if (argc > 0) {
		send_error(sock, EINVAL, "get-system-type doesn't take any argument.");
		return;
	}
	fp = fopen("/usr/share/eris-linux/system-type", "r");
	if (fp != NULL) {
		if (fgets(line, 127, fp) != NULL) {
			line[127] = '\0';
			if (line[0] != '\0')
				if (line[strlen(line) - 1] == '\n')
					line[strlen(line) - 1] = '\0';
			send_reply(sock, strlen(line), line);
			found = 1;
		}
		fclose(fp);
	}
	if (! found) {
		send_error(sock, ENOSYS, "No system type available.");
	}
}



static void get_number_of_slots_command(int sock, int argc, char *argv[])
{
	char line[128];

	if (argc > 0) {
		send_error(sock, EINVAL, "get-number-of-slots doesn't take any argument.");
		return;
	}
	snprintf(line, 127, "%d", MAX_CONTAINERS);
	line[127] = '\0';

	send_reply(sock, strlen(line), line);
}



static void get_container_present_command(int sock, int argc, char *argv[])
{
	int cnt;
	char line[128];
	FILE *fp;

	if (argc < 1) {
		send_error(sock, EINVAL, "get-container-present needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "get-container-present doesn't take any argument.");
		return;
	}
	if (sscanf(argv[0], "%d", &cnt) != 1) {
		send_error(sock, EINVAL, "Wrong container number.");
		return;
	}
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, 128, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[127] = '\0';
		send_error(sock, EINVAL, line);
		return;
	}

	fp = fopen("/etc/eris-linux/containers", "r");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to open containers description.");
		return;
	}

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, 127, fp) == NULL)
			break;
	fclose(fp);

	if (c != cnt + 1) {
		send_error(sock, EIO, "Containers description is incomplete.");
		return;
	}
	line[strlen(line) - 1] = '\0';

	if ((line[0] == '-') && (line[1] == '1')) {
		send_reply(sock, 1, "0");
		return;
	}
	send_reply(sock, 1, "1");
}



static void get_container_name_command(int sock, int argc, char *argv[])
{
	int cnt;
	char line[128];
	FILE *fp;

	if (argc < 1) {
		send_error(sock, EINVAL, "get-container-name needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "get-container-name takes only one argument.");
		return;
	}
	if (sscanf(argv[0], "%d", &cnt) != 1) {
		send_error(sock, EINVAL, "Wrong container number.");
		return;
	}
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, 128, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[127] = '\0';
		send_error(sock, EINVAL, line);
		return;
	}

	fp = fopen("/etc/eris-linux/containers", "r");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to open containers description.");
		return;
	}

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, 127, fp) == NULL)
			break;
	fclose(fp);

	if (c != cnt + 1) {
		send_error(sock, EIO, "Containers description is incomplete.");
		return;
	}
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

	int i;

	for (i = 0; (i < 127) && (line[i] != '!'); i++)
		// container id
		;
	if ((i == 127) || (line[i] !='!')) {
		send_error(sock, EIO, "Containers description is inconsistant.");
		return;
	}

	int start = i + 1;
	for (i ++; (i < 127) && (line[i] != '!'); i++)
		// container name
		;
	if ((i == 127) || (line[i] !='!')) {
		send_error(sock, EIO, "Containers description is inconsistant.\n");
		return;
	}
	line[i] = '\0';

	send_reply(sock, i - start, &(line[start]));
}



static void get_container_version_command(int sock, int argc, char *argv[])
{
	int cnt;
	char line[128];
	FILE *fp;

	if (argc < 1) {
		send_error(sock, EINVAL, "get-container-version needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "get-container-version takes only one argument.");
		return;
	}
	if (sscanf(argv[0], "%d", &cnt) != 1) {
		send_error(sock, EINVAL, "wrong container number.");
		return;
	}
	if ((cnt < 0) || (cnt >= MAX_CONTAINERS)) {
		snprintf(line, 128, "Container number must be between 0 and %d.", MAX_CONTAINERS - 1);
		line[127] = '\0';
		send_error(sock, EINVAL, line);
		return;
	}

	fp = fopen("/etc/eris-linux/containers", "r");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to open containers description.\n");
		return;
	}

	int c;
	for (c = 0; c <= cnt; c++)
		if (fgets(line, 127, fp) == NULL)
			break;
	fclose(fp);

	if (c != cnt + 1) {
		send_error(sock, EIO, "Containers description is incomplete.\n");
		return;
	}
	if (line[0] != '\0')
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

	int i;

	for (i = 0; (i < 127) && (line[i] != '!'); i++)
		// container id
		;
	if ((i == 127) || (line[i] !='!')) {
		send_error(sock, EIO, "Containers description is inconsistant.\n");
		return;
	}
	for (i ++; (i < 127) && (line[i] != '!'); i++)
		// container name
		;
	if ((i == 127) || (line[i] !='!')) {
		send_error(sock, EIO, "Containers description is inconsistant.\n");
		return;
	}

	int start = i + 1;
	for (i ++; (i < 127) && (line[i] != '!'); i++)
		// container version
		;
	if ((i == 127) || (line[i] !='!')) {
		send_error(sock, EIO, "Containers description is inconsistant.\n");
		return;
	}
	line[i] = '\0';

	send_reply(sock, i - start, &(line[start]));
}

