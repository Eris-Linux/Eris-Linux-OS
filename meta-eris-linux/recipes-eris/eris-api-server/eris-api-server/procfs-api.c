/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-server.h"
#include "procfs-api.h"


static void get_global_pid_cmd(int sock, int argc, char *argv[]);

#define MAX_CONTAINERS  4


int init_procfs_api(void)
{
	if (register_api_command("get-global-pid", "gpid", "Get the global PID of a process hosted in a container.", get_global_pid_cmd))
		return -1;
	return 0;
}



static void get_global_pid_cmd(int sock, int argc, char *argv[])
{
	long int cont_ns, cont_pid;

	if ((argc != 3)
	 || (sscanf(argv[1], "%ld", &cont_pid) != 1)
	 || (sscanf(argv[2], "%ld", &cont_ns) != 1)) {
		send_error(sock, 400, "usage: get-global-pid <pid> <ns>");
		return;
	}

	DIR *dir = opendir("/proc");

	for (;;) {
		struct dirent *entry;
		entry = readdir(dir);
		if (entry == NULL)
			break;
		int i;
		for (i = 0; entry->d_name[i] != '\0'; i++)
			if (! isdigit(entry->d_name[i]))
				break;
		if (entry->d_name[i] != '\0')
			continue;

		char status[128];
		char line[128];
		FILE *fp;

		long int ns, pid;
		snprintf(status, 127, "/proc/%.12s/status", entry->d_name);
		if ((fp = fopen(status, "r")) != NULL) {
			int found = 0;
			while (fgets(line, 127, fp) != NULL) {
				if (sscanf(line, "NSpid: %ld %ld", &ns, &pid) == 2) {
					if ((ns == cont_ns) && (pid == cont_pid)) {
						found = 1;
						send_reply(sock, strlen(entry->d_name), entry->d_name);
						break;
					}
				}
			}
			if (found)
				break;
		}
	}
	closedir(dir);
}
