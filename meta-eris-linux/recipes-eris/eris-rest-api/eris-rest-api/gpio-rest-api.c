
#include <dirent.h>
#include <gpiod.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "gpio-rest-api.h"

#define GPIO_IDENTIFIER_SIZE  64
#define GPIO_FULLNAME_SIZE    256


// ---------------------- Private types and structures.

struct eris_api_gpio {
	char                       id[GPIO_IDENTIFIER_SIZE];
	char                       name[GPIO_FULLNAME_SIZE];
	unsigned int               offset;
	struct gpiod_chip         *chip;
	struct gpiod_line_request *request;
	int                        output;
};


// ---------------------- Private method declarations.

static int load_gpio_names (const char *app);

static enum MHD_Result get_gpio_list   (struct MHD_Connection *connection);
static enum MHD_Result post_gpio       (struct MHD_Connection *connection);
static enum MHD_Result delete_gpio     (struct MHD_Connection *connection);
static enum MHD_Result get_gpio_value  (struct MHD_Connection *connection);
static enum MHD_Result put_gpio_value  (struct MHD_Connection *connection);
//static enum MHD_Result get_gpio_edge   (struct MHD_Connection *connection);


// ---------------------- Private variables.

static struct eris_api_gpio *Eris_gpios = NULL;
static int                   Gpio_count = 0;
static pthread_mutex_t       Gpio_mutex = PTHREAD_MUTEX_INITIALIZER;


// ---------------------- Public methods

int init_gpio_rest_api(const char *app)
{
	if (app == NULL)
		return -1;

	return load_gpio_names(app);
}



enum MHD_Result gpio_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcmp(url, "/api/gpio/list") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_gpio_list(connection);
	}

	if (strcmp(url, "/api/gpio") == 0) {
		if (strcmp(method, "POST") == 0)
			return post_gpio(connection);
		if (strcmp(method, "DELETE") == 0)
			return delete_gpio(connection);
	}

	if (strcmp(url, "/api/gpio/value") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_gpio_value(connection);
		if (strcmp(method, "PUT") == 0)
			return put_gpio_value(connection);
	}
/*
	if (strcmp(url, "/api/gpio/edge") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_gpio_edge(connection);
	}
*/
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
		snprintf(devname, sizeof(devname), "/dev/%s", entry->d_name);
		devname[sizeof(devname) - 1] = '\0';
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
			if (line == NULL)
				continue;

			struct eris_api_gpio *new_eris_gpios = realloc(Eris_gpios, (Gpio_count + 1) * sizeof(struct eris_api_gpio));
			if (new_eris_gpios == NULL) {
				fprintf(stderr, "%s: not enough memory to initialize GPIO name table.\n", app);
				closedir(dir);
				gpiod_line_info_free(line);
				gpiod_chip_info_free(info);
				return -1;
			}
			Eris_gpios = new_eris_gpios;

			Eris_gpios[Gpio_count].offset = gpiod_line_info_get_offset(line);
			Eris_gpios[Gpio_count].request = NULL;
			Eris_gpios[Gpio_count].chip = chip;
			Eris_gpios[Gpio_count].output = -1;

			int nb = snprintf(Eris_gpios[Gpio_count].id, GPIO_IDENTIFIER_SIZE, "%s:%d", entry->d_name, Eris_gpios[Gpio_count].offset);
			if ((nb < 0) || ((size_t)nb >= sizeof(Eris_gpios[Gpio_count].id))) {
				gpiod_line_info_free(line);
				continue;
			}

			const char *name = gpiod_line_info_get_name(line);
			if (name == NULL)
				name = "";

			snprintf(Eris_gpios[Gpio_count].name, GPIO_FULLNAME_SIZE, "%s", name);

			for (size_t j = 0; Eris_gpios[Gpio_count].name[j] != '\0'; j++) {
				if ((unsigned char)Eris_gpios[Gpio_count].name[j] < '0')
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



static enum MHD_Result get_gpio_list(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	pthread_mutex_lock(&Gpio_mutex);
	addsnprintf(&reply, &size, &pos, "[");
	for (int i = 0; i < Gpio_count; i++) {
		if (i > 0)
			addsnprintf(&reply, &size, &pos, ",");
		addsnprintf(&reply, &size, &pos, "{ \"id\": \"%s\", \"name\": \"%s\" } ", Eris_gpios[i].id, Eris_gpios[i].name);
	}
	addsnprintf(&reply, &size, &pos, "]");
	pthread_mutex_unlock(&Gpio_mutex);

	if (reply == NULL)
		ret = send_rest_response(connection, "[]");
	int ret;
	if (reply[0] == '\0')
		ret = send_rest_response(connection, "[]");
	else
		ret = send_rest_response(connection, reply);
	free(reply);

	return ret;
}



static enum MHD_Result post_gpio(struct MHD_Connection *connection)
{
	enum MHD_Result ret = MHD_NO;

	if (connection == NULL)
		goto out;

	int num;

	const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
	if (id == NULL) {
		ret = send_rest_error(connection, "Missing GPIO identifier.", 400);
		goto out;
	}

	const char *direction = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "direction");
	if (direction == NULL) {
		ret = send_rest_error(connection, "Missing GPIO direction.", 400);
		goto out;
	}

	if ((strcmp(direction, "in") != 0) && (strcmp(direction, "out") != 0)) {
		ret = send_rest_error(connection, "Invalid direction.", 400);
		goto out;
	}

	int output = (direction[0] == 'o');

	const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");
	if ((value == NULL)  &&  (output)) {
		ret = send_rest_error(connection, "Missing GPIO value.", 400);
		goto out;
	}

	if ((value != NULL) && ((strcmp(value, "0") != 0) && (strcmp(value, "1") != 0))) {
		ret = send_rest_error(connection, "Invalid value.", 400);
		goto out;
	}

	pthread_mutex_lock(&Gpio_mutex);

	for (num = 0; num < Gpio_count; num ++)
		if (strcmp(id, Eris_gpios[num].id) == 0)
			break;
	if (num >= Gpio_count) {
		ret = send_rest_error(connection, "Unknown GPIO identifier.", 404);
		goto out_unlock;
	}

	if (Eris_gpios[num].request != NULL) {
		ret = send_rest_error(connection, "GPIO line is already reserved by Eris API.", 409);
		goto out_unlock;
	}

	struct gpiod_line_settings *settings;
	settings = gpiod_line_settings_new();
	if (settings == NULL) {
		ret = send_rest_error(connection, "Memory allocation error.", 500);
		goto out_unlock;
	}

	if (output) {
		gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
		gpiod_line_settings_set_output_value(settings, value[0] - '0');
	} else {
		gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	}

	struct gpiod_line_config *config = gpiod_line_config_new();
	if (config == NULL) {
		ret = send_rest_error(connection, "Memory allocation error.", 500);
		goto out_free_settings;
	}

	if (gpiod_line_config_add_line_settings(config, &(Eris_gpios[num].offset), 1, settings) != 0) {
		ret = send_rest_error(connection, "Unable to reserve the GPIO.", 500);
		goto out_free_line_config;
	}

	struct gpiod_request_config * rconfig = gpiod_request_config_new();
	if  (rconfig == NULL) {
		ret = send_rest_error(connection, "Memory allocation error.", 500);
		goto out_free_line_config;
	}
	gpiod_request_config_set_consumer(rconfig, "Eris API");

	struct gpiod_line_request *request;
	request = gpiod_chip_request_lines(Eris_gpios[num].chip, rconfig, config);
	if (request == NULL) {
		ret = send_rest_error(connection, "The GPIO is already reserved by another application.", 409);
		goto out_free_request_config;
	}
	Eris_gpios[num].request = request;
	Eris_gpios[num].output = output;
	ret = send_rest_response(connection, "Ok");

	out_free_request_config:
	gpiod_request_config_free(rconfig);

	out_free_line_config:
	gpiod_line_config_free(config);

	out_free_settings:
	gpiod_line_settings_free(settings);

	out_unlock:
	pthread_mutex_unlock(&Gpio_mutex);

	out:
	return ret;
}



static enum MHD_Result delete_gpio(struct MHD_Connection *connection)
{
	enum MHD_Result ret = MHD_NO;

	if (connection == NULL)
		goto out;

	const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
	if (id == NULL) {
		ret = send_rest_error(connection, "Missing GPIO identifier.", 400);
		goto out;
	}

	pthread_mutex_lock(&Gpio_mutex);
	int num;
	for (num = 0; num < Gpio_count; num ++)
		if (strcmp(id, Eris_gpios[num].id) == 0)
			break;
	if (num >= Gpio_count) {
		ret = send_rest_error(connection, "Unknown GPIO identifier.", 404);
		goto out_unlock;
	}

	if (Eris_gpios[num].request == NULL) {
		ret = send_rest_error(connection, "GPIO already free.", 409);
		goto out_unlock;
	}
	gpiod_line_request_release(Eris_gpios[num].request);

	Eris_gpios[num].request = NULL;
	Eris_gpios[num].output  = -1;

	ret = send_rest_response(connection, "Ok");

	out_unlock:
	pthread_mutex_unlock(&Gpio_mutex);

	out:
	return ret;
}



static enum MHD_Result get_gpio_value(struct MHD_Connection *connection)
{
	enum MHD_Result ret = MHD_NO;

	if (connection == NULL)
		goto out;


	const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
	if (id == NULL) {
		ret = send_rest_error(connection, "Missing GPIO identifier.", 400);
		goto out;
	}

	pthread_mutex_lock(&Gpio_mutex);

	int num;
	for (num = 0; num < Gpio_count; num ++)
		if (strcmp(id, Eris_gpios[num].id) == 0)
			break;
	if (num >= Gpio_count) {
		ret = send_rest_error(connection, "Unknown GPIO identifier.", 404);
		goto out_unlock;
	}

	if (Eris_gpios[num].request == NULL) {
		ret = send_rest_error(connection, "The GPIO is not reserved.", 409);
		goto out_unlock;
	}

	if (Eris_gpios[num].output) {
		ret = send_rest_error(connection, "This GPIO is not readable.", 409);
		goto out_unlock;
	}

	int value = gpiod_line_request_get_value(Eris_gpios[num].request, Eris_gpios[num].offset);
	if (value < 0) {
		ret = send_rest_error(connection, "Unable to read GPIO value.", 500);
		goto out_unlock;
	}

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	addsnprintf(&reply, &size, &pos, "%d", value);
	ret = send_rest_response(connection, reply);
	free(reply);

	out_unlock:
	pthread_mutex_unlock(&Gpio_mutex);

	out:
	return ret;
}



static enum MHD_Result put_gpio_value(struct MHD_Connection *connection)
{
	enum MHD_Result ret = MHD_NO;

	if (connection == NULL)
		goto out;

	const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
	if (id == NULL) {
		ret = send_rest_error(connection, "Missing GPIO identifier.", 400);
		goto out;
	}

	pthread_mutex_lock(&Gpio_mutex);

	int num;
	for (num = 0; num < Gpio_count; num ++)
		if (strcmp(id, Eris_gpios[num].id) == 0)
			break;
	if (num >= Gpio_count) {
		ret = send_rest_error(connection, "Unknown GPIO identifier.", 404);
		goto out_unlock;
	}

	if (Eris_gpios[num].request == NULL) {
		ret = send_rest_error(connection, "The GPIO is not reserved.", 409);
		goto out_unlock;
	}

	if (! Eris_gpios[num].output) {
		ret = send_rest_error(connection, "This GPIO is not writable.", 409);
		goto out_unlock;
	}

	const char *value_string = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "value");
	if (value_string == NULL) {
		ret = send_rest_error(connection, "Missing value.", 400);
		goto out_unlock;
	}

	if ((strcmp(value_string, "0") != 0) && (strcmp(value_string, "1") != 0)) {
		ret = send_rest_error(connection, "Invalid value.", 400);
		goto out_unlock;
	}
	int value = (value_string[0] == '0' ? 0 : 1);

	if (gpiod_line_request_set_value(Eris_gpios[num].request, Eris_gpios[num].offset, value) < 0) {
		ret = send_rest_error(connection, "Unable to set GPIO value.", 500);
		goto out_unlock;
	}
	ret = send_rest_response(connection, "Ok");

	out_unlock:
	pthread_mutex_unlock(&Gpio_mutex);

	out:
	return ret;
}


/*
static enum MHD_Result get_gpio_edge(struct MHD_Connection *connection)
{
	if (connection == NULL)
		return MHD_NO;

	int num;

	const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
	if (id == NULL)
		return send_rest_error(connection, "Missing GPIO identifier.", 400);

	for (num = 0; num < Gpio_count; num ++)
		if (strcmp(id, Eris_gpios[num].id) == 0)
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
	if (strncmp(event, "ris", 3) == 0)
		evtype = GPIOD_EDGE_EVENT_RISING_EDGE;
	else if (strncmp(event, "fal", 3) == 0)
		evtype = GPIOD_EDGE_EVENT_FALLING_EDGE;
	else
		return send_rest_error(connection, "Unknown event (must be 'rising' or 'falling').", 400);

	struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(64);
	int ret = -1;
	if (buffer == NULL)
		ret = send_rest_error(connection, "Unable to get an event buffer.", 500);

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

*/
