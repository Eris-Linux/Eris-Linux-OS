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

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "time-rest-api.h"


// ---------------------- Private macros declarations.

#define NTP_SERVER_PREFIX  "ntp_server="
#define NTP_ENABLE_PREFIX  "ntp_enable="
#define TIME_ZONE_PREFIX   "time_zone="
#define TIME_ZONE_PATH     "/usr/share/zoneinfo"


// ---------------------- Private method declarations.

static enum MHD_Result read_and_send_value  (struct MHD_Connection *connection, const char *parameter);
static enum MHD_Result store_received_value (struct MHD_Connection *connection, const char *parameter, const char *value);

static enum MHD_Result get_time_ntp_server (struct MHD_Connection *connection);
static enum MHD_Result put_time_ntp_server (struct MHD_Connection *connection);
static enum MHD_Result get_time_ntp        (struct MHD_Connection *connection);
static enum MHD_Result put_time_ntp        (struct MHD_Connection *connection);
static enum MHD_Result get_time_zone_list  (struct MHD_Connection *connection);
static enum MHD_Result get_time_zone       (struct MHD_Connection *connection);
static enum MHD_Result put_time_zone       (struct MHD_Connection *connection);
static enum MHD_Result get_time_local      (struct MHD_Connection *connection);
static enum MHD_Result get_time_system     (struct MHD_Connection *connection);
static enum MHD_Result put_time_system     (struct MHD_Connection *connection);

static void read_time_zone_list(void);
static void set_rtc_time(struct tm *tm);

// ---------------------- Private variables.

static char **tz_names = NULL;
static int nb_tz_names = 0;


// ---------------------- Public methods

int init_time_rest_api(const char *app)
{
	(void) app;

	char *tz = NULL;

	read_time_zone_list();

	if (read_parameter_value(TIME_ZONE_PREFIX, &tz) == 0) {
		setenv("TZ", tz, 1);
		free(tz);
	} else {
		setenv("TZ", "UTC", 1);
	}

	return 0;
}



enum MHD_Result time_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcasecmp(url, "/api/time/ntp/server") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_ntp_server(connection);
		if (strcmp(method, "PUT") == 0)
			return put_time_ntp_server(connection);
	}
	if (strcasecmp(url, "/api/time/ntp") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_ntp(connection);
		if (strcmp(method, "PUT") == 0)
			return put_time_ntp(connection);
	}
	if (strcasecmp(url, "/api/time/zone/list") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_zone_list(connection);
	}
	if (strcasecmp(url, "/api/time/zone") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_zone(connection);
		if (strcmp(method, "PUT") == 0)
			return put_time_zone(connection);
	}
	if (strcasecmp(url, "/api/time/local") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_local(connection);
	}
	if (strcasecmp(url, "/api/time/system") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_time_system(connection);
		if (strcmp(method, "PUT") == 0)
			return put_time_system(connection);
	}
	return MHD_NO;
}


// ---------------------- Private methods

static enum MHD_Result read_and_send_value(struct MHD_Connection *connection, const char *parameter)
{
	char *reply = NULL;

	if (read_parameter_value(parameter, &reply) != 0)
		return send_rest_error(connection, "Unable to read internal parameter.", 500);

	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result store_received_value(struct MHD_Connection *connection, const char *parameter, const char *value)
{
	if (write_parameter_value(parameter, value) != 0) {
		return send_rest_error(connection, "Unable to store internal parameter.", 500);
	}
	return send_rest_response(connection, "Ok");
}



static enum MHD_Result get_time_ntp_server(struct MHD_Connection *connection)
{
	return read_and_send_value(connection, NTP_SERVER_PREFIX);
}



static enum MHD_Result put_time_ntp_server(struct MHD_Connection *connection)
{
	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "server");
	if (name == NULL)
	        return send_rest_error(connection, "Missing server name.", 400);

	for (int i = 0; name[i] != '\0'; i++) {
		if (isalnum(name[i]))
			continue;
		if ((name[i] == '.')
		 || (name[i] == ':')
		 || (name[i] == '-')
		 || (name[i] == '_'))
			continue;
		return send_rest_error(connection, "NTP server must be a string of letters, digits or .:-_.", 400);
	}
	return store_received_value(connection, NTP_SERVER_PREFIX, name);
}



static enum MHD_Result get_time_ntp(struct MHD_Connection *connection)
{
	return read_and_send_value(connection, NTP_ENABLE_PREFIX);
}



static enum MHD_Result put_time_ntp(struct MHD_Connection *connection)
{
	const char *status = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "status");
	if (status == NULL)
	        return send_rest_error(connection, "Missing NTP status.", 400);
	if ((strcasecmp(status, "yes") != 0) && (strcasecmp(status, "no") != 0)) {
	        return send_rest_error(connection, "NTP status must be 'yes' or 'no'.", 400);
	}
	return store_received_value(connection, NTP_ENABLE_PREFIX, status);
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



static enum MHD_Result get_time_zone_list(struct MHD_Connection *connection)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	for (int i = 0; i < nb_tz_names; i++) {
		if (tz_names[i] != NULL) {
			if (pos > 0)
				addsnprintf(&reply, &size, &pos, " ");
			addsnprintf(&reply, &size, &pos, "%s", tz_names[i]);
		}
	}
	if (reply == NULL) {
		return send_rest_error(connection, "No time zone available.", 500);
	}
	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result get_time_zone(struct MHD_Connection *connection)
{
	return read_and_send_value(connection, TIME_ZONE_PREFIX);
}



static enum MHD_Result put_time_zone(struct MHD_Connection *connection)
{

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "zone");
	if (name == NULL)
	        return send_rest_error(connection, "Missing time zone name.", 400);

	for (int i = 0; i < nb_tz_names; i++) {
		if (tz_names[i] != NULL) {
			if (strcasecmp(tz_names[i], name) == 0) {
				setenv("TZ", tz_names[i], 1);
				return store_received_value(connection, TIME_ZONE_PREFIX, tz_names[i]);
			}
		}
	}
	return send_rest_error(connection, "Invalid time zone name.", 400);
}



static enum MHD_Result get_time_local(struct MHD_Connection *connection)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm *t = localtime(&(tv.tv_sec));

	char reply[128];
	snprintf(reply, 128, "%04d-%02d-%02d %02d:%02d:%02d:%06ld",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec,
		tv.tv_usec);
	int ret = send_rest_response(connection, reply);
	return ret;
}



static enum MHD_Result get_time_system(struct MHD_Connection *connection)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm *t = gmtime(&(tv.tv_sec));
	char reply[128];

	snprintf(reply, 128, "%04d-%02d-%02d %02d:%02d:%02d:%06ld",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec,
		tv.tv_usec);
	int ret = send_rest_response(connection, reply);
	return ret;
}



static enum MHD_Result put_time_system(struct MHD_Connection *connection)
{

	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	const char *timestr = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "time");
	if (timestr == NULL)
	        return send_rest_error(connection, "Missing system time.", 400);

	if ((sscanf(timestr, "%d-%d-%dT%d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(timestr, "%d-%d-%d %d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(timestr, "%d/%d/%d %d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)
	 && (sscanf(timestr, "%d:%d:%d:%d:%d:%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 6)) {
		return send_rest_error(connection, "Wrong time format (must be yyyy/mm/ddThh:mm:ss).", 400);
	}
	if ((tm.tm_year < 1970) || (tm.tm_year > 2999)) {
		return send_rest_error(connection, "Wrong year value (must be between 1970 and 2999).", 400);
	}
	tm.tm_year -= 1900;
	if ((tm.tm_mon < 1) || (tm.tm_mon > 12)) {
		return send_rest_error(connection, "Wrong month value (must be between 1 and 12).", 400);
	}
	tm.tm_mon --;
	if ((tm.tm_mday < 1) || (tm.tm_mday > 31)) {
		return send_rest_error(connection, "Wrong day value (must be between 1 and 31).", 400);
	}
	if ((tm.tm_hour < 0) || (tm.tm_hour > 23)) {
		return send_rest_error(connection, "Wrong hour value (must be between 0 and 23).", 400);
	}
	if ((tm.tm_min < 0) || (tm.tm_min > 59)) {
		return send_rest_error(connection, "Wrong minute value (must be between 0 and 59).", 400);
	}
	if ((tm.tm_sec < 0) || (tm.tm_sec > 60)) {
		return send_rest_error(connection, "Wrong second value (must be between 0 and 60).", 400);
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
	        return send_rest_error(connection, "Wrong date.", 400);
	}

	settimeofday(&tv, NULL);
	set_rtc_time(&tm);

	return send_rest_response(connection, "Ok");
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
