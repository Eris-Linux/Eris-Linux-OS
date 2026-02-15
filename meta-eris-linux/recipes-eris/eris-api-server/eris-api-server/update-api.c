/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <uuid/uuid.h>


#include "api-server.h"
#include "update-api.h"


// ---------------------- Private constants and macros.

#define AUTOMATIC_REBOOT_PREFIX   "automatic_reboot_after_update="
#define CONTACT_PERIOD_PREFIX     "status_upload_period_seconds="
#define CONTAINER_UPDATE_POLICY   "container_update_policy="

#define REBOOT_NEEDED_FLAG_FILE   "/tmp/reboot-is-needed"
#define SYSTEM_UPDATE_STATUS_FILE "/tmp/system-update-status"

#define SERVER_CONTACT_FIFO       "/tmp/contact-eris-server"


// ---------------------- Private method declarations.


static void get_system_update_status(int sock, int argc, char *argv[]);

static void get_reboot_needed_flag(int sock, int argc, char *argv[]);
static void set_reboot_needed_flag(int sock, int argc, char *argv[]);

static void get_server_contact_period(int sock, int argc, char *argv[]);
static void set_server_contact_period(int sock, int argc, char *argv[]);
static void contact_server(int sock, int argc, char *argv[]);

static void get_automatic_reboot_flag(int sock, int argc, char *argv[]);
static void set_automatic_reboot_flag(int sock, int argc, char *argv[]);

static void get_container_update_policy(int sock, int argc, char *argv[]);
static void set_container_update_policy(int sock, int argc, char *argv[]);

static void	restore_factory_preset(int sock, int argc, char *argv[]);

static void halt_command(int sock, int argc, char *argv[]);
static void reboot_command(int sock, int argc, char *argv[]);


// ---------------------- Public methods

int init_update_api(void)
{
	if (register_api_command("get-system-update-status",    "gsus", "Get the current status of update system.", get_system_update_status))
		return -1;

	if (register_api_command("get-reboot-needed-flag",      "grnf", "Get the Reboot Needed flag.", get_reboot_needed_flag))
		return -1;
	if (register_api_command("set-reboot-needed-flag",      "srnf", "Set the Reboot Needed flag.", set_reboot_needed_flag))
		return -1;

	if (register_api_command("get-server-contact-period",   "gscp", "Get the period in seconds between server contact.", get_server_contact_period))
		return -1;
	if (register_api_command("set-server-contact-period",   "sscp", "Set the period in seconds between server contact.", set_server_contact_period))
		return -1;
	if (register_api_command("contact-server",              "csvr", "Contact the server now. Useful if period is zero.", contact_server))
		return -1;

	if (register_api_command("get-automatic-reboot-flag",   "garf", "Get the Automatic Reboot flag", get_automatic_reboot_flag))
		return -1;
	if (register_api_command("set-automatic-reboot-flag",   "sarf", "Set the Automatic Reboot flag", set_automatic_reboot_flag))
		return -1;

	if (register_api_command("get-container-update-policy", "gcup", "Get the Container Update policy", get_container_update_policy))
		return -1;
	if (register_api_command("set-container-update-policy", "scup", "Set the Container Update policy", set_container_update_policy))
		return -1;

	if (register_api_command("restore-factory-preset",      "rfac", "Erase all data and setup and resotre factory presets.", restore_factory_preset))
		return -1;

	if (register_api_command("reboot",                      "restart", "Restart the system.", reboot_command))
		return -1;
	if (register_api_command("shutdown",                    "halt", "Halt the system and power it off if possible..", halt_command))
		return -1;

	return 0;
}


// ---------------------- Private methods

static void get_system_update_status(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-system-update-status doesn't take any argument.");
		return;
	}

	FILE *fp = NULL;
	char line[32];
	int status = 0;
	if ((fp = fopen(SYSTEM_UPDATE_STATUS_FILE, "r")) != NULL) {
		if (fgets(line, 32, fp) != NULL) {
			if (sscanf(line, "%d", &status) != 1)
				status = 0;
		}
		fclose(fp);
	}
	char reply[64];
	switch (status) {
	case 1:
		strncpy(reply, "1 System OK.", 64);
		break;
	case 2:
		strncpy(reply, "2 System update install in progress.", 64);
		break;
	case 3:
		strncpy(reply, "3 System update install Ok.", 64);
		break;
	case 4:
		strncpy(reply, "4 System update install failed", 64);
		break;
	case 5:
		strncpy(reply, "5 System reboot in progress", 64);
		break;
	default:
		send_error(sock, errno, "Unable read system update status.");
		return;
	}
	send_reply(sock, strlen(reply), reply);
}



static void get_reboot_needed_flag(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-reboot-needed-flag doesn't take any argument.");
		return;
	}
	if (access(REBOOT_NEEDED_FLAG_FILE, F_OK) == 0) {
		send_reply(sock, 1, "y");
		return;
	}
	send_reply(sock, 1, "n");
}



static void set_reboot_needed_flag(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-reboot-needed-flag needs one argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "set-reboot-needed-flag doesn't take more than one argument.");
		return;
	}
	if ((argv[0][0] == 'y') || (argv[0][0] == 'Y')) {
		FILE *fp = fopen(REBOOT_NEEDED_FLAG_FILE, "w");
		if (fp)
			fclose(fp);
		send_reply(sock, 2, "Ok");
	} else if ((argv[0][0] == 'n') || (argv[0][0] == 'N')) {
		unlink(REBOOT_NEEDED_FLAG_FILE);
		send_reply(sock, 2, "Ok");
	} else {
		send_error(sock, EINVAL, "Wrong argument for set-reboot-needed-flag.");
	}
}



static void get_server_contact_period(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-server-contact-period doesn't take any argument.");
		return;
	}

	char *reply = NULL;

	if (read_parameter_value(CONTACT_PERIOD_PREFIX, &reply) != 0) {
		send_error(sock, EIO, "Error while reading contact period.");
		return;
	}
	if (reply == NULL) {
		send_error(sock, ENOENT, "No contact period defined for this machine.");
		return;
	}
	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void set_server_contact_period(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-server-contact-period needs one argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "set-server-contact-period doesn't take more than one argument.");
		return;
	}

	int i;
	if ((sscanf(argv[0], "%d", &i) != 1) || (i < 0) || (i > 86400)) {
		send_error(sock, EINVAL, "Server contact period must be in [0-86400] seconds.");
		return;
	}

	if (write_parameter_value(CONTACT_PERIOD_PREFIX, argv[0]) != 0) {
		send_error(sock, errno, "Unable to save contact period.");
		return;
	}

	send_reply(sock, 2, "Ok");
}



static void contact_server(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "contact-server doesn't take any argument.");
		return;
	}
	int fd = open(SERVER_CONTACT_FIFO, O_NONBLOCK | O_WRONLY);
	if (fd >= 0) {
		write(fd, "E", 1);
		send_reply(sock, 2, "Ok");
		close(fd);
		return;
	}
	send_error(sock, errno, "Unable to ask for a contact.");
}




static void get_automatic_reboot_flag(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-automatic-reboot-needed-flag doesn't take any argument.");
		return;
	}

	char *reply = NULL;

	if (read_parameter_value(AUTOMATIC_REBOOT_PREFIX, &reply) != 0) {
		send_error(sock, EIO, "Error while reading automatic reboot flag.");
		return;
	}
	if (reply == NULL) {
		send_error(sock, ENOENT, "No automatic reboot flag defined for this machine.");
		return;
	}
	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void set_automatic_reboot_flag(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-automatic-reboot-flag needs one argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "set-automatic-reboot-flag doesn't take more than one argument.");
		return;
	}
	if ((argv[0][0] != 'y') && (argv[0][0] != 'Y') && (argv[0][0] != 'n') && (argv[0][0] != 'N')) {
		send_error(sock, errno, "Automatic reboot flag must be 'y' or 'n'.");
		return;
	}
	if (write_parameter_value(AUTOMATIC_REBOOT_PREFIX, argv[0]) != 0) {
		send_error(sock, errno, "Unable to save automatic reboot flag.");
		return;
	}
	send_reply(sock, 2, "Ok");
}



static void get_container_update_policy(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-container-update-policy doesn't take any argument.");
		return;
	}

	char *reply = NULL;

	if ((read_parameter_value(CONTAINER_UPDATE_POLICY, &reply) != 0)
	 || (reply == NULL)) {
	 	send_reply(sock, strlen("immediate"), "immediate");
	} else {
		send_reply(sock, strlen(reply), reply);
	}
	free(reply);
}



static void set_container_update_policy(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-container-update-policy needs one argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "set-container-update-policy doesn't take more than one argument.");
		return;
	}
	if ((strcmp(argv[0], "immediate") != 0)
	 && (strcmp(argv[0], "atreboot") != 0)) {
		send_error(sock, EINVAL, "container update policy must be 'immediate' or 'atreboot'.");
		return;
	}
	if (write_parameter_value(CONTAINER_UPDATE_POLICY, argv[0]) != 0) {
		send_error(sock, errno, "Unable to save container update policy.");
		return;
	}
	send_reply(sock, 2, "Ok");
}



static void restore_factory_preset(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "restore-factory-preset doesn't take any argument.");
		return;
	}
	send_reply(sock, 16, "Not implemented.");
}



static void halt_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "halt doesn't take any argument.");
		return;
	}
	sync();
	reboot(RB_POWER_OFF);
	send_error(sock, errno, "Unable to halt the system.");
	exit(EXIT_FAILURE);
}



static void reboot_command(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "reboot doesn't take any argument.");
		return;
	}
	sync();
	reboot(RB_AUTOBOOT);
	send_error(sock, errno, "Unable to reboot the system.");
	exit(EXIT_FAILURE);
}
