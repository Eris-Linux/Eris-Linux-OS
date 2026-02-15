/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "api-server.h"
#include "time-api.h"


// ---------------------- Private macros.

#define NTP_SERVER_PREFIX  "ntp_server="
#define NTP_ENABLE_PREFIX  "ntp_enable="
#define TIME_ZONE_PREFIX   "time_zone="
#define TIME_ZONE_PATH     "/usr/share/zoneinfo"

// ---------------------- Private method declarations.

static void get_and_send_value(int sock, const char *parameter);
static void set_received_value(int sock, const char *parameter, const char *value);

static void get_ntp_server(int sock, int argc, char *argv[]);
static void set_ntp_server(int sock, int argc, char *argv[]);
static void get_ntp_enable(int sock, int argc, char *argv[]);
static void set_ntp_enable(int sock, int argc, char *argv[]);
static void read_time_zone_list(void);
static void list_time_zones(int sock, int argc, char *argv[]);
static void get_time_zone(int sock, int argc, char *argv[]);
static void set_time_zone(int sock, int argc, char *argv[]);
static void get_local_time(int sock, int argc, char *argv[]);
static void get_system_time(int sock, int argc, char *argv[]);
static void set_system_time(int sock, int argc, char *argv[]);
static void set_rtc_time(struct tm *tm);

// ---------------------- Private variables.

static char **tz_names = NULL;
static int nb_tz_names = 0;


// ---------------------- Public methods

int init_time_api(void)
{
	char *tz = NULL;
	if (read_parameter_value(TIME_ZONE_PREFIX, &tz) == 0) {
		setenv("TZ", tz, 1);
		free(tz);
	} else {
		setenv("TZ", "UTC", 1);
	}

	if (register_api_command("get-ntp-server", "gnts", "Get the current NTP server URL.", get_ntp_server))
		return -1;

	if (register_api_command("set-ntp-server", "snts", "Set a new NTP server URL.", set_ntp_server))
		return -1;

	if (register_api_command("get-ntp-enable", "gnte", "Get wether NTP is enabled or not.", get_ntp_enable))
		return -1;

	if (register_api_command("set-ntp-enable", "snte", "Set wether NTP is enabled or not.", set_ntp_enable))
		return -1;

	if (register_api_command("list-time-zones", "ltmz", "List all available timezones.", list_time_zones))
		return -1;

	if (register_api_command("get-time-zone", "gtmz", "Get the current system timezone.", get_time_zone))
		return -1;

	if (register_api_command("set-time-zone", "stmz", "Set a new system timezone.", set_time_zone))
		return -1;

	if (register_api_command("get-local-time", "gltm", "Get the current system time in local timezone.", get_local_time))
		return -1;

	if (register_api_command("get-system-time", "gstm", "Get the current system time in UTC timezone.", get_system_time))
		return -1;

	if (register_api_command("set-system-time", "sstm", "Set the current system time in UTC timezone.", set_system_time))
		return -1;

	return 0;
}


// ---------------------- Private methods

static void get_and_send_value(int sock, const char *parameter)
{
	char *reply = NULL;

	if (read_parameter_value(parameter, &reply) != 0) {
		send_error(sock, errno, "Unable to read parameter.");
		return;
	}
	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void set_received_value(int sock, const char *parameter, const char *value)
{
	if (write_parameter_value(parameter, value) != 0) {
		send_error(sock, errno, "Unable to save parameter.");
		return;
	}
	send_reply(sock, 2, "Ok");
}



static void get_ntp_server(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-ntp-server doesn't take any an argument.");
		return;
	}
	get_and_send_value(sock, NTP_SERVER_PREFIX);
}



static void set_ntp_server(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-ntp-server needs an argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "setntp-server takes only one argument.");
		return;
	}
	for (int i = 0; argv[0][i] != '\0'; i++) {
		if (isalnum(argv[0][i]))
			continue;
		if ((argv[0][i] == '.')
		 || (argv[0][i] == ':')
		 || (argv[0][i] == '-')
		 || (argv[0][i] == '_'))
			continue;
		send_error(sock, EINVAL, "set-ntp-server argument must be a string of letters, digits or .:-_");
		return;
	}
	set_received_value(sock, NTP_SERVER_PREFIX, argv[0]);

	send_reply(sock, 2, "Ok");
}



static void get_ntp_enable(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-ntp-enable doesn't take any an argument.");
		return;
	}
	get_and_send_value(sock, NTP_ENABLE_PREFIX);
}



static void set_ntp_enable(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-ntp-enable needs an argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "set-ntp-enable takes only one argument.");
		return;
	}
	if ((strcasecmp(argv[0], "yes") != 0) && (strcasecmp(argv[0], "no") != 0)) {
		send_error(sock, EINVAL, "set-ntp-enable argument must be 'yes' or 'no'");
		return;
	}
	set_received_value(sock, NTP_ENABLE_PREFIX, argv[0]);

	send_reply(sock, 2, "Ok");
}



static void add_time_zone(const char *group, const char *zone)
{
	static char **new_tz_names;

	new_tz_names = realloc(tz_names, (nb_tz_names + 1) * sizeof(char *));
	if (new_tz_names == NULL)
		return;
	tz_names = new_tz_names;

	if (group != NULL) {
		tz_names[nb_tz_names] = malloc(strlen(group) + 1 + strlen(zone) + 1);
		if (tz_names[nb_tz_names] != NULL)
			sprintf(tz_names[nb_tz_names], "%s/%s", group, zone);
	} else {
		tz_names[nb_tz_names] = malloc(strlen(zone) + 1);
		if (tz_names[nb_tz_names] != NULL)
			sprintf(tz_names[nb_tz_names], "%s", zone);
	}
	nb_tz_names ++;
}



static int compare_tz(const void *tz1, const void *tz2)
{
	return strcmp(*(const char **)tz1, *(const char **)tz2);
}



static void read_time_zone_list(void)
{
	char dirname[1024];
	DIR *dir;
	DIR *subdir;
	struct dirent *entry;
	struct dirent *subentry;

	if (tz_names != NULL)
		return;

	dir = opendir(TIME_ZONE_PATH);
	if (dir == NULL)
		return;

	while ((entry = readdir(dir)) != NULL) {
		if ((! isalpha(entry->d_name[0]))
		 || (! isupper(entry->d_name[0])))
			continue;
		if (entry->d_type == DT_REG) {
			add_time_zone(NULL, entry->d_name);
			continue;
		}
		if (entry->d_type == DT_DIR) {
			snprintf(dirname, 1023, "%s/%s", TIME_ZONE_PATH, entry->d_name);
			dirname[1023] = '\0';
			subdir = opendir(dirname);
			while ((subentry = readdir(subdir)) != NULL) {
				if ((! isalpha(subentry->d_name[0]))
				 || (! isupper(subentry->d_name[0])))
					continue;
				if (subentry->d_type != DT_REG)
					continue;
				add_time_zone(entry->d_name, subentry->d_name);
			}
			closedir(subdir);
		}
	}
	closedir(dir);

	qsort(tz_names, nb_tz_names, sizeof(char *),  compare_tz);
}



static void list_time_zones(int sock, int argc, char *argv[])
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	read_time_zone_list();

	for (int i = 0; i < nb_tz_names; i++) {
		if (tz_names[i] != NULL) {
			if (pos > 0)
				addsnprintf(&reply, &size, &pos, " ");
			addsnprintf(&reply, &size, &pos, "%s", tz_names[i]);
		}
	}
	if (reply == NULL) {
		send_error(sock, ENOENT, "No timezone available.");
		return;
	}
	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void get_time_zone(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get_time_zone doesn't take any an argument.");
		return;
	}
	get_and_send_value(sock, TIME_ZONE_PREFIX);
}



static void set_time_zone(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "This command needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "This command takes only one argument.");
		return;
	}
	for (int i = 0; i < nb_tz_names; i++) {
		if (tz_names[i] != NULL) {
			if (strcasecmp(tz_names[i], argv[0]) == 0) {
				set_received_value(sock, TIME_ZONE_PREFIX, tz_names[i]);
				setenv("TZ", tz_names[i], 1);
				send_reply(sock, 2, "Ok");
				return;
			}
		}
	}
	send_error(sock, ENOENT, "Invalid time zone name.");
}



static void get_local_time(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-local-time doesn't take any an argument.");
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm *t = localtime(&(tv.tv_sec));
	char reply[128];

	snprintf(reply, 128, "%04d-%02d-%02d %02d:%02d:%02d:%06ld",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec,
		tv.tv_usec);
	send_reply(sock, strlen(reply), reply);
}



static void get_system_time(int sock, int argc, char *argv[])
{
	if (argc > 0) {
		send_error(sock, EINVAL, "get-utc-time doesn't take any an argument.");
		return;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm *t = gmtime(&(tv.tv_sec));
	char reply[128];

	snprintf(reply, 128, "%04d-%02d-%02d %02d:%02d:%02d:%06ld",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec,
		tv.tv_usec);
	send_reply(sock, strlen(reply), reply);
}



static void set_system_time(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "set-system-time needs an argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "set-system-time command takes only one argument.");
		return;
	}

	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	if ((sscanf(argv[0], "%d-%d-%dT%d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(argv[0], "%d-%d-%d %d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(argv[0], "%d/%d/%d %d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(argv[0], "%d:%d:%d:%d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)) {
		send_error(sock, EINVAL, "Wrong time format (must be yyyy/mm/ddThh:mm:ss).");
		return;
	}
	if ((tm.tm_year < 1970) || (tm.tm_year > 2999)) {
		send_error(sock, EINVAL, "Wrong year value (must be between 1970 and 2999).");
		return;
	}
	tm.tm_year -= 1900;
	if ((tm.tm_mon < 1) || (tm.tm_mon > 12)) {
		send_error(sock, EINVAL, "Wrong month value (must be between 1 and 12).");
		return;
	}
	tm.tm_mon --;
	if (tm.tm_mday < 1) {
		send_error(sock, EINVAL, "Wrong month day value (must be positive).");
		return;
	}
	if ((tm.tm_hour < 0) || (tm.tm_hour > 23)) {
		send_error(sock, EINVAL, "Wrong hour value (must be between 0 and 23).");
		return;
	}
	if ((tm.tm_min < 0) || (tm.tm_min > 59)) {
		send_error(sock, EINVAL, "Wrong minutes value (must be between 0 and 59).");
		return;
	}
	if ((tm.tm_sec < 0) || (tm.tm_sec > 60)) {
		send_error(sock, EINVAL, "Wrong seconds value (must be between 0 and 60).");
		return;
	}

	struct timeval tv;
	memset(&tv, 0, sizeof(tv));

	char timezone[64];
	memset(timezone, 0, 64);

	char *ptr = getenv("TZ");
	if (ptr != NULL)
		strncpy(timezone, ptr, 63);

	unsetenv("TZ");
	tv.tv_sec = mktime(&tm);

	if (timezone[0] != '\0')
		setenv("TZ", timezone, 1);

	if (tv.tv_sec == (time_t) -1) {
		send_error(sock, EINVAL, "Wrong date.");
		return;
	}

	settimeofday(&tv, NULL);
	set_rtc_time(&tm);

	send_reply(sock, 2, "Ok");
}



static void set_rtc_time(struct tm *tm)
{
	int fd;

	if ((fd = open("/dev/rtc", O_RDONLY)) < 0)
		return;

	struct rtc_time rtm;
	memset(&rtm, 0, sizeof(rtm));

	rtm.tm_sec = tm->tm_sec;
	rtm.tm_min = tm->tm_min;
	rtm.tm_hour = tm->tm_hour;
	rtm.tm_mday = tm->tm_mday;
	rtm.tm_mon = tm->tm_mon;
	rtm.tm_year = tm->tm_year;

	ioctl(fd, RTC_SET_TIME, &rtm);
	close(fd);
}




