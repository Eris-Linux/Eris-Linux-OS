/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include "api-server.h"
#include "gpio-api.h"
#include "leds-api.h"
#include "net-api.h"
#include "procfs-api.h"
#include "sbom-api.h"
#include "system-api.h"
#include "time-api.h"
#include "update-api.h"
#include "wdog-api.h"


// ---------------------- Private macros declarations.

#define ERIS_PARAMETERS_FILE  "/etc/eris-linux/parameters"
#define ERIS_PORT_NUMBER 31215
#define COMMAND_LINE_SIZE   128
#define MAX_COMMAND_LINE_ARGS   64

#define REQUEST_PREFIX "REQ "
#define REPLY_PREFIX   "REP"
#define ERROR_PREFIX   "ERR"
#define BYE_COMMAND    "BYE"
#define QUIT_COMMAND   "QUIT"


// ---------------------- Private types definitions.

typedef struct {
	char *command;
	char *abbreviation;
	char *help;
	void (*function)(int sock, int argc, char *argv[]);
} api_command_t;


// ---------------------- Private method declarations.

static int   start_api_server(void);
static void  communicate_with_client(int);
static void  call_command(int sock, const char *subcommand, int argc, char *argv[], int arglen[]);
static void  handler_sigchld(int unused);
static void  help_command(int sock, int argc, char *argv[]);


// ---------------------- Private variables declarations.

api_command_t *Api_commands = NULL;
int Nb_api_commands = 0;


// ---------------------- Public methods definitions.

int main(int argc, char *argv[])
{
	register_api_command("help", "?", NULL, help_command);

	init_gpio_api();
	init_leds_api();
	init_net_api();
	init_procfs_api();
	init_sbom_api();
	init_system_api();
	init_time_api();
	init_update_api();
	init_wdog_api();

	start_api_server();
}



int register_api_command(const char *command, const char *abbreviation, const char *help, void (*function)(int sock, int argc, char *argv[]))
{
	api_command_t *new_ptr;

	new_ptr = realloc(Api_commands, (Nb_api_commands + 1) * sizeof(api_command_t));
	if (new_ptr == NULL) {
		perror("realloc");
		return -1;
	}
	Api_commands = new_ptr;

	Api_commands[Nb_api_commands].command = malloc(strlen(command) + 1);
	if (Api_commands[Nb_api_commands].command == NULL) {
		perror("malloc");
		return -1;
	}
	strcpy(Api_commands[Nb_api_commands].command, command);

	if (abbreviation != NULL) {
		Api_commands[Nb_api_commands].abbreviation = malloc(strlen(abbreviation) + 1);
		if (Api_commands[Nb_api_commands].abbreviation == NULL) {
			perror("malloc");
			return -1;
		}
		strcpy(Api_commands[Nb_api_commands].abbreviation, abbreviation);
	} else {
		Api_commands[Nb_api_commands].abbreviation = NULL;
	}

	if (help != NULL) {
		Api_commands[Nb_api_commands].help = malloc(strlen(help) + 1);
		if (Api_commands[Nb_api_commands].help == NULL) {
			perror("malloc");
			return -1;
		}
		strcpy(Api_commands[Nb_api_commands].help, help);
	} else {
		Api_commands[Nb_api_commands].help = NULL;
	}

	Api_commands[Nb_api_commands].function = function;

	Nb_api_commands++;

	return 0;
}



void send_reply(int sock, size_t length, const void *data)
{
	char prefix[128];
	int prefix_length;
	char *newline = "\n";

	if (length == 0) {
		prefix_length = snprintf(prefix, 128, "%s 0\n", REPLY_PREFIX);
		prefix[127] = '\0';
		write(sock, prefix, strlen(prefix));
		return;
	}

	prefix_length = snprintf(prefix, 128, "%s %lu ", REPLY_PREFIX, length);
	prefix[127] = '\0';

	struct iovec iov[3] = {
		{ .iov_base = prefix,  .iov_len = prefix_length },
		{ .iov_base = data,    .iov_len = length        },
		{ .iov_base = newline, .iov_len = 1             }
	};

	writev(sock, iov, 3);
}



void send_error(int sock, int code, const char *label)
{
	char *message = NULL;
	size_t size = 0;
	size_t pos = 0;

	addsnprintf(&message, &size, &pos, "%s %d %lu %s\n", ERROR_PREFIX, code, strlen(label), label);

	write(sock, message, size);

	free(message);
}



int addsnprintf(char **string, size_t *size, size_t *pos, const char *format, ...)
{
	ssize_t length;
	va_list list;
	char *newstring;

	va_start(list, format);
	length = vsnprintf(NULL, 0, format, list);
	va_end(list);
	if (length < 0)
		return -1;

	if ((length + (*pos)) >= *size) {
		newstring = realloc(*string, length + (*pos) + 1);
		if (newstring == NULL)
			return -1;
		*string = newstring;
		*size = length + (*pos) + 1;
	}

	va_start(list, format);
	length = vsnprintf(*string + *pos, *size, format, list);
	va_end(list);
	if (length < 0)
		return -1;

	*pos += length;
	(*string)[*pos] = '\0';
	return 0;
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


// ---------------------- Private methods definitions.

static int start_api_server(void)
{
	int server_sock;
	struct sockaddr_in address;
	int reuseaddr = 1;

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(ERIS_PORT_NUMBER);

	if (bind(server_sock, (struct sockaddr *) &address, sizeof(address)) == -1) {
		perror("bind");
		return -1;
	}

	if (listen(server_sock, 4096) == -1) {
		perror("listen");
		return -1;
	}

	signal(SIGCHLD, handler_sigchld);

	for (;;) {
		long int connected_sock;
		connected_sock = accept(server_sock, NULL, 0);
		if (connected_sock < 0)
			continue;
		switch (fork()) {
			case -1:
				continue;
			case 0:
				close(server_sock);
				communicate_with_client(connected_sock);
				exit(EXIT_SUCCESS);
			default:
				close(connected_sock);
				break;
		}
	}
	return 0;
}



static void communicate_with_client(int sock)
{
	char   buffer[4096];
	int    lg;
	int    pos;
	int    start;
	int    nb;
	char  *command = NULL;
	int   *arglen  = NULL;
	int   *newarglen = NULL;
	char **argv = NULL;
	char **newargv = NULL;
	int    argc = 0;

	for (;;) {

		lg = read(sock, buffer, 4096);
		if (lg <= 0) {
			break;
		}

		pos = 0;

		if ((strncasecmp(& buffer[pos], BYE_COMMAND, strlen(BYE_COMMAND)) == 0)
		 || (strncasecmp(& buffer[pos], QUIT_COMMAND, strlen(QUIT_COMMAND)) == 0))
			break;

		if ((lg <= 4)
		 || (strncasecmp(& buffer[pos], REQUEST_PREFIX, strlen(REQUEST_PREFIX)) != 0)) {
			send_error(sock, EPROTO,  "Request must start by `REQ`.");
			continue;
		}

		pos += strlen(REQUEST_PREFIX);

		while ((pos < lg) && (isspace(buffer[pos])))
			pos ++;
		if (pos == lg) {
			send_error(sock, EPROTO, "Missing request.");
			continue;
		}

		start = pos;
		while ((pos < lg) && (! isspace(buffer[pos])))
			pos ++;

		if (pos == start) {
			send_error(sock, EPROTO, "Missing request.");
			continue;
		}

		assert(command == NULL);
		command = malloc(pos - start + 1);
		if (command == NULL) {
			send_error(sock, errno, "Not enough memory. Please retry later.");
			break;
		}
		memcpy(command, &(buffer[start]), pos - start);
		command[pos - start] = '\0';

		while ((pos < lg) && (isspace(buffer[pos])))
			pos ++;

		argc = 0;
		argv = NULL;
		arglen = NULL;

		while (pos < lg) {

			newarglen = realloc(arglen, (argc + 1) * sizeof(int));
			if (newarglen == NULL)
				break;
			arglen = newarglen;

			if (sscanf(&(buffer[pos]), "%d%n", &(arglen[argc]), &nb) != 1) {
				send_error(sock, EPROTO, "Argument must be preceded by its length.");
				break;
			}

			newargv = realloc(argv, (argc + 1) * sizeof(char *));
			if (newargv == NULL)
				break;
			argv = newargv;

			pos += nb;
			while ((pos < lg) && (isspace(buffer[pos])))
				pos++;
			if (pos >= lg)
				break;

			if (arglen[argc] < 0) {
				arglen[argc] = 0;
				while (((pos + arglen[argc]) < lg) && (! isspace(buffer[pos + arglen[argc]])))
					arglen[argc]++;
			}

			if ((pos + arglen[argc]) >= lg)
				break;

			argv[argc] = malloc(arglen[argc] + 1);
			if (argv[argc] == NULL)
				break;

			memcpy(argv[argc], &(buffer[pos]), arglen[argc]);
			argv[argc][arglen[argc]] = '\0';

			pos += arglen[argc];

			while ((pos < lg) && (isspace(buffer[pos])))
				pos ++;

			argc ++;
		}

		call_command(sock, command, argc, argv, arglen);

		free(command);
		command = NULL;

		while (--argc >= 0) {
			free(argv[argc]);
			argv[argc] = NULL;
		}

		free(argv);
		argv = NULL;

		free(arglen);
		arglen = NULL;
		argc = 0;
	}

	close(sock);
}



static void call_command(int sock, const char *command, int argc, char *argv[], int arglen[])
{
	for (int i = 0; i < Nb_api_commands; i++) {
		if (Api_commands[i].function == NULL)
			continue;
		if (Api_commands[i].command != NULL) {
			if (strcasecmp(command, Api_commands[i].command) == 0) {
				Api_commands[i].function(sock, argc, argv);
				return;
			}
		}
		if (Api_commands[i].abbreviation != NULL) {
			if (strcasecmp(command, Api_commands[i].abbreviation) == 0) {
				Api_commands[i].function(sock, argc, argv);
				return;
			}
		}
	}
	send_error(sock, ENOSYS, "Unknown command.");
}



static void  handler_sigchld(int unused)
{
	(void) unused;

	while(waitpid(-1, 0, WNOHANG) > 0)
		;
}



static void  help_command(int sock, int argc, char *argv[])
{
	for (int i = 0; i < Nb_api_commands; i++) {
		if (Api_commands[i].help != NULL) {
			if (Api_commands[i].command != NULL) {
				write(sock, Api_commands[i].command, strlen(Api_commands[i].command));
				write(sock, " ", 1);
			}
			if (Api_commands[i].abbreviation != NULL) {
				write(sock, "(", 1);
				write(sock, Api_commands[i].abbreviation, strlen(Api_commands[i].abbreviation));
				write(sock, ") ", 2);
			}
			write(sock, "- ", 2);
			write(sock, Api_commands[i].help, strlen(Api_commands[i].help));
			write(sock, "\n", 1);
		}
	}
}

