/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api-server.h"
#include "leds-api.h"


// ---------------------- Private macros declarations.

#define LED_TRIGGER_SETUP_FILE "/etc/eris-linux/led-triggers"

#define TRIGGER_ALWAYS_OFF   0
#define TRIGGER_ALWAYS_ON    1
#define TRIGGER_HEARTBEAT    2
#define TRIGGER_TIMER        3


// ---------------------- Private types definitions.

typedef struct {
	char *led_name;
	int   trigger;
	int   param[2];
} led_trigger_t;

// ---------------------- Private method declarations.

static void initialize_leds(void);
static void load_led_triggers_from_setup_file(void);
static void save_led_triggers_to_setup_file(void);
static void add_led_trigger(const char *name, int trigger, int param0, int param1);
static void load_led_triggers_from_sys_directory(void);
static void list_leds_command(int sock, int argc, char *argv[]);
static void get_led_trigger_command(int sock, int argc, char *argv[]);
static void set_led_trigger_command(int sock, int argc, char *argv[]);
static const char *trigger_name(int trigger);
static int trigger_number(const char *name);
static void update_trigger(int trigger);


// ---------------------- Private variables declarations.

led_trigger_t *led_triggers = NULL;
int nb_led_triggers = 0;


// ---------------------- Public methods definitions.

int init_leds_api(void)
{
	initialize_leds();

	if (register_api_command("list-leds", "leds", "List the leds available.", list_leds_command))
		return -1;

	if (register_api_command("get-led-trigger", "gled", "Get the trigger of a led and its params.", get_led_trigger_command))
		return -1;

	if (register_api_command("set-led-trigger", "sled", "Set the trigger of a led and its params.", set_led_trigger_command))
		return -1;

	return 0;
}


// ---------------------- Private methods definitions.

static void initialize_leds(void)
{
	load_led_triggers_from_setup_file();

	if (nb_led_triggers == 0) {
		load_led_triggers_from_sys_directory();
		save_led_triggers_to_setup_file();
	}

	for (int i = 0; i < nb_led_triggers; i++)
		update_trigger(i);
}



static void load_led_triggers_from_setup_file(void)
{
	FILE *fp = fopen(LED_TRIGGER_SETUP_FILE, "r");

	if (fp == NULL)
		return;

	char line[1024];

	while (fgets(line, 1023, fp) != NULL) {

		char name[256];
		int  trigger;
		int  param[2];

		if (sscanf(line, "%s %d %d %d", name, &trigger, &(param[0]), &(param[1])) == 4) {
			add_led_trigger(name, trigger, param[0], param[1]);
			continue;
		}

		if (sscanf(line, "%s %d", name, &trigger) == 2) {
			add_led_trigger(name, trigger, 0, 0);
			continue;
		}
	}
	fclose(fp);
}



static void save_led_triggers_to_setup_file(void)
{
	FILE *fp = fopen(LED_TRIGGER_SETUP_FILE, "w");

	if (fp == NULL)
		return;

	for (int i = 0; i < nb_led_triggers; i++)
		fprintf(fp, "%s %d %d %d\n", led_triggers[i].led_name, led_triggers[i].trigger, led_triggers[i].param[0], led_triggers[i].param[1]);

	fclose(fp);
}



static void add_led_trigger(const char *name, int trigger, int param0, int param1)
{
	led_triggers = realloc(led_triggers, sizeof(led_trigger_t) * (nb_led_triggers + 1));
	if (led_triggers == NULL)
		return;

	memset(&(led_triggers[nb_led_triggers]), 0, sizeof(led_trigger_t));

	led_triggers[nb_led_triggers].led_name = malloc(strlen(name) + 1);
	if (led_triggers[nb_led_triggers].led_name == NULL)
		return;
	strcpy(led_triggers[nb_led_triggers].led_name, name);

	led_triggers[nb_led_triggers].trigger = trigger;
	led_triggers[nb_led_triggers].param[0] = param0;
	led_triggers[nb_led_triggers].param[1] = param1;

	nb_led_triggers ++;
}



static void load_led_triggers_from_sys_directory(void)
{
	char filename[512];

	DIR *dir = opendir("/sys/class/leds");
	if (dir == NULL)
		return;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {

		if (entry->d_name[0] == '.')
			continue;

		snprintf(filename, 511, "/sys/class/leds/%s/device", entry->d_name);
		if (access(filename, F_OK) != 0)
			continue;

		snprintf(filename, 511, "/sys/class/leds/%s/trigger", entry->d_name);
		if (access(filename, F_OK) != 0)
			continue;

		FILE *fp = fopen(filename, "r");
		if (fp == NULL)
			return;

		char line[4096];
		if (fgets(line, 4095, fp) == NULL) {
			fclose (fp);
			continue;
		}
		fclose(fp);	
		line[4095] = '\0';

		int i = 0;
		while (line[i] != '\0') {
			if (line[i] == '[')
				break;
			i++;
		}
		if (line[i] != '[')
			continue;
		int j = i + 1;
		while (line[j] != '\0') {
			if (line[j] == ']')
				break;
			j ++;
		}
		if (line[j] != ']')
			continue;
		line[j] = '\0';

fprintf(stderr, "--> %s %s\n", entry->d_name, &(line[i+1]));

		if (strcmp(&(line[i+1]), "default-on") == 0) {
			add_led_trigger(entry->d_name, TRIGGER_ALWAYS_ON, 0, 0);
			continue;
		}

		if (strcmp(&(line[i+1]), "heartbeat") == 0) {
			add_led_trigger(entry->d_name, TRIGGER_HEARTBEAT, 0, 0);
			continue;
		}

		if (strcmp(&(line[i+1]), "timer") == 0) {

			int param[2];

			snprintf(filename, 511, "/sys/class/leds/%s/delay_on", entry->d_name);
			fp = fopen(filename, "r");
			if (fp == NULL)
				continue;
			if (fgets(line, 4095, fp) == NULL) {
				fclose(fp);
				continue;
			}
			fclose(fp);
			if (sscanf(line, "%d", &(param[0])) != 1)
				continue;

			snprintf(filename, 511, "/sys/class/leds/%s/delay_off", entry->d_name);
			fp = fopen(filename, "r");
			if (fp == NULL)
				continue;
			if (fgets(line, 4095, fp) == NULL) {
				fclose(fp);
				continue;
			}
			fclose(fp);
			if (sscanf(line, "%d", &(param[1])) != 1)
				continue;
			
			add_led_trigger(entry->d_name, TRIGGER_TIMER, param[0], param[1]);
			continue;
		}

		add_led_trigger(entry->d_name, TRIGGER_ALWAYS_OFF, 0, 0);
	}
	closedir(dir);
}



static void list_leds_command(int sock, int argc, char *argv[])
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	if (argc > 0) {
		send_error(sock, EINVAL, "list-leds doesn't take any argument.");
		return;
	}

	if (nb_led_triggers == 0) {
		send_error(sock, ENODEV, "No LED available.");
		return;	
	}

	for (int i = 0; i < nb_led_triggers; i++) {

		if (pos != 0)
			addsnprintf(&reply, &size, &pos, "%s", " ");

		addsnprintf(&reply, &size, &pos, "%s", led_triggers[i].led_name);
	}

	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void get_led_trigger_command(int sock, int argc, char *argv[])
{
	if (argc < 1) {
		send_error(sock, EINVAL, "get-led-trigger needs one argument.");
		return;
	}

	if (argc > 1) {
		send_error(sock, EINVAL, "get-led-trigger takes only one argument.");
		return;
	}

	for (int i = 0; i < nb_led_triggers; i++) {

		if (strcasecmp(led_triggers[i].led_name, argv[0]) == 0) {
			const char *trigger = trigger_name(led_triggers[i].trigger);
			send_reply(sock, strlen(trigger), trigger);
			return;
		}
	}
	send_error(sock, EINVAL, "This led doesn't exist.");
}



static void set_led_trigger_command(int sock, int argc, char *argv[])
{
	if (argc < 2) {
		send_error(sock, EINVAL, "set-led-trigger needs two arguments.");
		return;
	}

	if (argc > 4) {
		send_error(sock, EINVAL, "too many arguments for set-led-trigger.");
		return;
	}

	for (int i = 0; i < nb_led_triggers; i++) {

		if (strcasecmp(led_triggers[i].led_name, argv[0]) == 0) {
			led_triggers[i].trigger = trigger_number(argv[1]);
			if (argc >= 3)
				led_triggers[i].param[0] = atoi(argv[2]);
			if (argc >= 4)
				led_triggers[i].param[1] = atoi(argv[3]);
			save_led_triggers_to_setup_file();
			update_trigger(i);
			send_reply(sock, 2, "Ok");
			return;
		}
	}
	send_error(sock, EINVAL, "This led doesn't exist.");
}



static const char *trigger_name(int trigger)
{
	switch(trigger) {
		case TRIGGER_ALWAYS_OFF:
			return "none";
		case TRIGGER_ALWAYS_ON:
			return "default-on";
		case TRIGGER_TIMER:
			return "timer";
		case TRIGGER_HEARTBEAT:
			return "heartbeat";
		default:
			break;
	}
	return "none";
}



static int trigger_number(const char *name)
{
	if (strcasecmp(name, "default-on") == 0)
		return TRIGGER_ALWAYS_ON;
	if (strcasecmp(name, "timer") == 0)
		return TRIGGER_TIMER;
	if (strcasecmp(name, "heartbeat") == 0)
		return TRIGGER_HEARTBEAT;
	return TRIGGER_ALWAYS_OFF;
}



static void update_trigger(int trigger)
{
	char filename[512];
	snprintf(filename, 511, "/sys/class/leds/%s/trigger", led_triggers[trigger].led_name);

	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		return;
	fprintf(fp, "%s\n", trigger_name(led_triggers[trigger].trigger));
	fclose(fp);

	if (led_triggers[trigger].trigger == TRIGGER_TIMER) {
		snprintf(filename, 511, "/sys/class/leds/%s/delay_on", led_triggers[trigger].led_name);
		FILE *fp = fopen(filename, "w");
		if (fp == NULL)
			return;
		fprintf(fp, "%d\n", led_triggers[trigger].param[0]);
		fclose(fp);

		snprintf(filename, 511, "/sys/class/leds/%s/delay_off", led_triggers[trigger].led_name);
		fp = fopen(filename, "w");
		if (fp == NULL)
			return;
		fprintf(fp, "%d\n", led_triggers[trigger].param[1]);
		fclose(fp);
	}
}
