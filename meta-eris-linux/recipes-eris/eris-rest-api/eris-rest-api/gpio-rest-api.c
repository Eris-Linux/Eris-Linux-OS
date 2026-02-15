
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "gpio-rest-api.h"


// ---------------------- Private types and structures.

struct eris_api_gpio {
	char                      *name;
	unsigned int               offset;
	struct gpiod_chip         *chip;
	struct gpiod_line_request *request;
	int                        output;
};


// ---------------------- Private method declarations.

static int load_gpio_names (const char *app);

static int list_gpio       (struct MHD_Connection *connection, const char *url);
static int request_gpio    (struct MHD_Connection *connection, const char *url);
static int release_gpio    (struct MHD_Connection *connection, const char *url);
static int get_gpio_value  (struct MHD_Connection *connection, const char *url);
static int set_gpio_value  (struct MHD_Connection *connection, const char *url);
static int wait_gpio_edge  (struct MHD_Connection *connection, const char *url);


// ---------------------- Private variables.

static struct eris_api_gpio *Eris_gpios = NULL;
static int                   Gpio_count = 0;


// ---------------------- Public methods

int init_gpio_rest_api(const char *app)
{
	return load_gpio_names(app);
}



enum MHD_Result gpio_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcasecmp(url, "/api/gpio/list") == 0) {
		if (strcmp(method, "GET") == 0)
			return list_gpio(connection, url);
	}

	if (strcasecmp(url, "/api/gpio") == 0) {
		if (strcmp(method, "GET") == 0)
			return request_gpio(connection, url);
		if (strcmp(method, "DELETE") == 0)
			return release_gpio(connection, url);
	}

	if (strcasecmp(url, "/api/gpio/value") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_gpio_value(connection, url);
		if (strcmp(method, "PUT") == 0)
			return set_gpio_value(connection, url);
	}

	if (strcasecmp(url, "/api/gpio/edge") == 0) {
		if (strcmp(method, "GET") == 0)
			return wait_gpio_edge(connection, url);
	}

	return MHD_NO;
}


// ---------------------- Private methods

static int load_gpio_names(const char *app)
{
	// Scaning /dev and checking for gpiod chip seems not very efficient,
	// but this is the way the official `gpiodetect` tool works.
	
	DIR *dir = opendir("/dev");
	if (dir == NULL)
		return -1;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;

		char devname[512];
		snprintf(devname, 512, "/dev/%s", entry->d_name);
		devname[511] = '\0';
		if (! gpiod_is_gpiochip_device(devname))
			continue;
	
		struct gpiod_chip *chip;
		if ((chip = gpiod_chip_open(devname)) == NULL)
			continue;

		struct gpiod_chip_info *info;
		if ((info = gpiod_chip_get_info(chip)) == NULL) {
			gpiod_chip_close(chip);
			continue;
		}

		for (int i = 0; i < gpiod_chip_info_get_num_lines(info); i++) {

			struct gpiod_line_info *line = gpiod_chip_get_line_info(chip, i);

			Eris_gpios = realloc(Eris_gpios, (Gpio_count + 1) * sizeof(struct eris_api_gpio));
			if (Eris_gpios == NULL) {
				fprintf(stderr, "%s: not enough memory to initialize GPIO name table.\n", app);
				return -1;
			}

			Eris_gpios[Gpio_count].offset = gpiod_line_info_get_offset(line);
			Eris_gpios[Gpio_count].request = NULL;
			Eris_gpios[Gpio_count].chip = chip;
			Eris_gpios[Gpio_count].output = -1;
			const char *name = gpiod_line_info_get_name(line);
			if (name == NULL)
				name = "";
			Eris_gpios[Gpio_count].name = malloc(strlen(name) + 1);
			if (Eris_gpios[Gpio_count].name != NULL) {
				strcpy(Eris_gpios[Gpio_count].name, name);
				for (int j = 0; Eris_gpios[Gpio_count].name[j] != '\0'; j++)
					if (Eris_gpios[Gpio_count].name[j] == ' ')
						Eris_gpios[Gpio_count].name[j] = '_';
			}
			Gpio_count ++;
			gpiod_line_info_free(line);
		}
		gpiod_chip_info_free(info);

		// Don't close `chip`, it will be used via Eris_gpios[Gpio_count].chip to request lines.
		// gpiod_chip_close(chip);
		}
		closedir(dir);
	return 0;
}



static int list_gpio(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;
	
	for (int i = 0; i < Gpio_count; i++) {
		if (pos != 0)
		addsnprintf(&reply, &size, &pos, " ");
		addsnprintf(&reply, &size, &pos, "%s", Eris_gpios[i].name);
	}
	
	if (reply == NULL) {
			return send_rest_error(connection, "No GPIO available.", 400);
	}
	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static int request_gpio(struct MHD_Connection *connection, const char *url)
{
	int num;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
		return send_rest_error(connection, "Missing GPIO name.", 400);

	const char *direction = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "direction");
	if (direction == NULL)
		return send_rest_error(connection, "Missing GPIO direction.", 400);

	if ((strncasecmp(direction, "in", 2) != 0) && (strncasecmp(direction, "out", 3) != 0))
		return send_rest_error(connection, "Invalid direction", 400);

	const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");
	if ((value == NULL)  &&  (strncasecmp(direction, "out", 3) == 0))
		return send_rest_error(connection, "Missing GPIO value.", 400);
	if ((value != NULL) && ((value[0] != '0') && (value[0] != '1')))
		return send_rest_error(connection, "Invalid value", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcasecmp(name, Eris_gpios[num].name) == 0)
			break;
	if (num >= Gpio_count)
		return send_rest_error(connection, "Unknown GPIO name.", 404);

	if (Eris_gpios[num].request != NULL)
		return send_rest_error(connection, "GPIO line is already reserved by Eris API.", 403);

	struct gpiod_line_settings *settings;
	settings = gpiod_line_settings_new();
	if (settings == NULL)
		return send_rest_error(connection, "Memory allocation error.", 500);

	if (strncasecmp(direction, "out", 3) == 0) {
		gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
		gpiod_line_settings_set_output_value(settings, value[0] - '0');
	} else {
		gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	}

	struct gpiod_line_config *config = gpiod_line_config_new();
	if (config == NULL) {
		gpiod_line_settings_free(settings);
		return send_rest_error(connection, "Memory allocation error.", 500);
	}

	if (gpiod_line_config_add_line_settings(config, &(Eris_gpios[num].offset), 1, settings) != 0) {
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
		return send_rest_error(connection, "Unable to reserve the GPIO.", 500);
	}

	struct gpiod_request_config * rconfig = gpiod_request_config_new();
	if  (rconfig == NULL) {
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
		return send_rest_error(connection, "Memory allocation error.", 500);
	}
	gpiod_request_config_set_consumer(rconfig, "Eris API");

	Eris_gpios[num].request = gpiod_chip_request_lines(Eris_gpios[num].chip, rconfig, config);
	if (Eris_gpios[num].request == NULL) {
		gpiod_request_config_free(rconfig);
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
		return send_rest_error(connection, "The GPIO is already reserved by another application.", 403);
	}

	Eris_gpios[num].output = (strncasecmp(direction, "out", 3) == 0);

	gpiod_request_config_free(rconfig);
	gpiod_line_config_free(config);
	gpiod_line_settings_free(settings);

	return send_rest_response(connection, "Ok");
}



static int release_gpio(struct MHD_Connection *connection, const char *url)
{
	int num;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
		return send_rest_error(connection, "Missing GPIO name.", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcasecmp(name, Eris_gpios[num].name) == 0)
			break;
	if (num >= Gpio_count)
		return send_rest_error(connection, "Unknown GPIO name.", 404);

	if (Eris_gpios[num].request == NULL)
		return send_rest_error(connection, "GPIO line already free.", 404);

	gpiod_line_request_release(Eris_gpios[num].request);

	Eris_gpios[num].request = NULL;
	return send_rest_response(connection, "Ok");
}



static int get_gpio_value(struct MHD_Connection *connection, const char *url)
{
	int num;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
		return send_rest_error(connection, "Missing GPIO name.", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcasecmp(name, Eris_gpios[num].name) == 0)
			break;
	if (num >= Gpio_count)
		return send_rest_error(connection, "Unknown GPIO name.", 404);

	if (Eris_gpios[num].request == NULL)
		return send_rest_error(connection, "The GPIO line is not reserved.", 400);

	if (Eris_gpios[num].output)
		return send_rest_error(connection, "This GPIO line is not readable.", 400);

	int value = gpiod_line_request_get_value(Eris_gpios[num].request, Eris_gpios[num].offset);

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	addsnprintf(&reply, &size, &pos, "%d", value);
	int ret = send_rest_response(connection, reply);
	free(reply);

	return ret;
}



static int set_gpio_value(struct MHD_Connection *connection, const char *url)
{
	int num;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
		return send_rest_error(connection, "Missing GPIO name.", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcasecmp(name, Eris_gpios[num].name) == 0)
			break;
	if (num >= Gpio_count)
		return send_rest_error(connection, "Unknown GPIO name.", 404);

	if (Eris_gpios[num].request == NULL)
		return send_rest_error(connection, "The GPIO line is not reserved.", 400);

	if (! Eris_gpios[num].output)
		return send_rest_error(connection, "This GPIO line is not writable.", 400);

	const char *value_string = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");
	if (value_string == NULL)
		return send_rest_error(connection, "Missing value.", 400);

	int value = (value_string[0] == '0' ? 0 : 1);
	
	gpiod_line_request_set_value(Eris_gpios[num].request, Eris_gpios[num].offset, value);
	return send_rest_response(connection, "Ok");
}



static int wait_gpio_edge(struct MHD_Connection *connection, const char *url)
{
	int num;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
		return send_rest_error(connection, "Missing GPIO name.", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcasecmp(name, Eris_gpios[num].name) == 0)
			break;
	if (num >= Gpio_count)
		return send_rest_error(connection, "Unknown GPIO name.", 404);

	if (Eris_gpios[num].request == NULL)
		return send_rest_error(connection, "The GPIO line is not reserved.", 400);

	if (Eris_gpios[num].output)
		return send_rest_error(connection, "This GPIO line is not readable.", 400);


	const char *event = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "type");
	if (event == NULL)
		return send_rest_error(connection, "Missing type of event.", 400);

	enum gpiod_edge_event_type evtype;
	if (strncasecmp(event, "ris", 3) == 0)
		evtype = GPIOD_EDGE_EVENT_RISING_EDGE;
	else if (strncasecmp(event, "fal", 3) == 0)
		evtype = GPIOD_EDGE_EVENT_FALLING_EDGE;
	else
		return send_rest_error(connection, "Unknown event (must be 'rising' or 'falling').", 400);

	struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(64);
	int ret = -1;

	for (;;) {
		int n = gpiod_line_request_wait_edge_events(Eris_gpios[num].request, -1);
		if (n <= 0) {
			ret = send_rest_error(connection, "Unable to wait event on this GPIO line.", 500);
			break;
		}
		n = gpiod_line_request_read_edge_events(Eris_gpios[num].request, buffer, 64);
		if (n < 0) {
			ret = send_rest_error(connection, "Unable to read event on this GPIO line.", 500);
			break;
		}
		if (n == 0) {
			ret = send_rest_error(connection, "Timeout.", 400);
			break;
		}
		int i;
		for (i = 0; i < n; i ++) {
			struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, i);
			if (gpiod_edge_event_get_event_type(event) == evtype) {
				ret = send_rest_response(connection, "Ok");
				break;
			}
		}
		if (i < n)
			break;
	}
	gpiod_edge_event_buffer_free(buffer);

	return ret;
}

