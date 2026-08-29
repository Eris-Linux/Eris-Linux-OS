/*
 *  Eris Linux Rest API
 *
 *  (c) 2026: Logilin
 *  All rights reserved
 */


#include <ctype.h>
#include <fcntl.h>
#include <microhttpd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "addsnprintf.h"
#include "contact-rest-api.h"
#include "container-rest-api.h"
#include "eris-rest-api.h"
#include "gpio-rest-api.h"
#include "net-rest-api.h"
#include "reboot-rest-api.h"
#include "sbom-rest-api.h"
#include "system-rest-api.h"
#include "time-rest-api.h"
#include "watchdog-rest-api.h"


// ---------------------- Private macros declarations.

#define ERIS_PARAMETERS_FILE  "/etc/eris-linux/parameters"


// ---------------------- Private types and structures.

// ---------------------- Private method declarations.

static enum MHD_Result eris_rest_api_handler(void *, struct MHD_Connection *, const char *, const char *, const char *, const char *a, size_t *, void **);

static int eris_rest_api_init(int argc, char *argv[]);

static enum MHD_Result eris_rest_api(struct MHD_Connection *connection, const char *url, const char *method);


// ---------------------- Private variables.

// ---------------------- Public methods

int main(int argc, char *argv[])
{
	struct MHD_Daemon *daemon;

	if (eris_rest_api_init(argc, argv) != 0)
		exit(EXIT_FAILURE);

	struct sockaddr_in loopback_addr;
	memset(&loopback_addr, 0, sizeof(loopback_addr));
	loopback_addr.sin_family = AF_INET;
	loopback_addr.sin_port = htons(8080);
	loopback_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	daemon = MHD_start_daemon(
		MHD_USE_SELECT_INTERNALLY,  // flags.
		8080,                       // port.
		NULL,                       // apc: callback to check authorized clients. NULL = all IP.
		NULL,                       // apc_cls: extra argument to apc.
		&eris_rest_api_handler,     // dh: default handler for all URI.
		NULL,                       // dh_cls: extra argument to dh.
		MHD_OPTION_SOCK_ADDR,       // export server only on loopback.
		(struct sockaddr *)&loopback_addr,
		MHD_OPTION_END              // End of arguments.
	);

	if (!daemon)
		exit(EXIT_FAILURE);

	pause();

	MHD_stop_daemon(daemon);

	return 0;
}



enum MHD_Result send_rest_error(struct MHD_Connection *connection, const char *err_message, unsigned int err_code)
{
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen(err_message), (void *)err_message, MHD_RESPMEM_PERSISTENT);
	enum MHD_Result ret = MHD_queue_response(connection, err_code, response);
	MHD_destroy_response(response);

	return ret;
}



enum MHD_Result send_rest_response(struct MHD_Connection *connection, const char *reply_message)
{
	struct MHD_Response *response;

	response = MHD_create_response_from_buffer(strlen(reply_message), (void *)reply_message, MHD_RESPMEM_MUST_COPY);
	enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}



int read_parameter_value(const char *parameter, char **value)
{
	FILE *fp;
	int found = 0;
	size_t size = 0;
	size_t pos  = 0;

	if ((fp = fopen(ERIS_PARAMETERS_FILE, "r")) == NULL)
		return -1;

	char line[1024];
	while (fgets(line, 1024, fp) != NULL) {
		line[1023] = '\0';
		if (line[0] != '\0')
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
		if (strncmp(line, parameter, strlen(parameter)) == 0) {
			addsnprintf(value, &size, &pos, "%s", &(line[strlen(parameter)]));
			found = 1;
			break;
		}
	}
	fclose(fp);

	if (! found)
		return -1;
	return 0;
}



int write_parameter_value(const char *parameter, const char *value)
{
	FILE *fp;
	if ((fp = fopen(ERIS_PARAMETERS_FILE, "r")) == NULL) {
		return -1;
	}

	FILE *fptmp;
	if ((fptmp = tmpfile()) == NULL) {
		return -1;
	}

	char line[1024];

	int found = 0;

	while (fgets(line, 1023, fp) != NULL) {
		line[1023] = '\0';
		if (strncmp(line, parameter, strlen(parameter)) == 0) {
			snprintf(line, 1023, "%s%s\n", parameter, value);
			found = 1;
		}
		fputs(line, fptmp);
	}
	if (! found) {
		snprintf(line, 1023, "%s%s\n", parameter, value);
		fputs(line, fptmp);
	}
	fclose(fp);

	if ((fp = fopen(ERIS_PARAMETERS_FILE, "w")) == NULL) {
		fclose(fptmp);
		return -1;
	}

	rewind(fptmp);
	while (fgets(line, 1023, fptmp) != NULL) {
		fputs(line, fp);
	}

	fclose(fp);
	fclose(fptmp);

	return 0;
}


// ---------------------- Private methods

static enum MHD_Result eris_rest_api_handler(
                                 void *cls, struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **ptr)
{
	(void) cls;
	(void) version;
	(void) upload_data;
	(void) upload_data_size;
	(void) ptr;

	if (strcmp(url, "/api") == 0)
		return eris_rest_api(connection, url, method);

	if (strncmp(url, "/api/contact", 12) == 0)
		return contact_rest_api(connection, url, method);

	if (strncmp(url, "/api/container", 14) == 0)
		return container_rest_api(connection, url, method);

	if (strncmp(url, "/api/gpio", 9) == 0)
		return gpio_rest_api(connection, url, method);

	if (strncmp(url, "/api/network", 12) == 0)
		return net_rest_api(connection, url, method);

	if (strncmp(url, "/api/reboot", 11) == 0)
		return reboot_rest_api(connection, url, method);

	if (strncmp(url, "/api/sbom", 9) == 0)
		return sbom_rest_api(connection, url, method);

	if (strncmp(url, "/api/system", 11) == 0)
		return system_rest_api(connection, url, method);

	if (strncmp(url, "/api/time", 9) == 0)
		return time_rest_api(connection, url, method);

	if (strncmp(url, "/api/watchdog", 13) == 0)
		return watchdog_rest_api(connection, url, method);

	return MHD_NO;
}



static int eris_rest_api_init(int argc, char *argv[])
{
	if (init_contact_rest_api(argv[0]) != 0)
		return -1;

	if (init_container_rest_api(argv[0]) != 0)
		return -1;

	if (init_gpio_rest_api(argv[0]) != 0)
		return -1;

	if (init_net_rest_api(argv[0]) != 0)
		return -1;

	if (init_reboot_rest_api(argv[0]) != 0)
		return -1;

	if (init_sbom_rest_api(argv[0]) != 0)
		return -1;

	if (init_system_rest_api(argv[0]) != 0)
		return -1;

	if (init_time_rest_api(argv[0]) != 0)
		return -1;

	if (init_watchdog_rest_api(argv[0]) != 0)
		return -1;

	return 0;
}



static enum MHD_Result eris_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if ((strcasecmp(url, "/api") == 0) && (strcmp(method, "GET") == 0)) {
		char * message =
			"Welcome on the Eris-Linux REST API.\n"
			"Here are some API modules endpoints:\n"
			"  /api/contact    parameters to contact the device manager,\n"
			"  /api/container  container informations,\n"
			"  /api/gpio       GPIO-based features,\n"
			"  /api/network    access to network setup functions,\n"
			"  /api/reboot     restart related features,\n"
			"  /api/sbom       S-BOM (package and license details),\n"
			"  /api/system     system and update parameters,\n"
			"  /api/time       time-handling features,\n"
			"  /api/watchdog   watchdog setup.\n"
			"";
		return send_rest_response(connection, message);
	}
	return MHD_NO;
}
