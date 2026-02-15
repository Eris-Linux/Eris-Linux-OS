/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "api-server.h"
#include "gpio-api.h"


// ---------------------- Private types and structures.

struct eris_api_gpio {
	char *name;
	unsigned int offset;
	struct gpiod_chip *chip;
	struct gpiod_line_request *request;
	int output;
};


// ---------------------- Private method declarations.

static int  load_gpio_names(void);
static void list_gpio_names(int sock, int argc, char *argv[]);
static void get_gpio_direction(int sock, int argc, char *argv[]);
static void request_gpio_for_input(int sock, int argc, char *argv[]);
static void request_gpio_for_output(int sock, int argc, char *argv[]);
static void release_gpio(int sock, int argc, char *argv[]);
static void read_gpio_value(int sock, int argc, char *argv[]);
static void write_gpio_value(int sock, int argc, char *argv[]);
static void wait_gpio_edge(int sock, int argc, char *argv[]);
static void get_test_input_gpio(int sock, int argc, char *argv[]);
static void get_test_output_gpio(int sock, int argc, char *argv[]);


// ---------------------- Private variables.

static struct eris_api_gpio * Eris_gpios = NULL;
static int Gpio_count = 0;

static char *Test_input_gpio  = "@TEST_INPUT_GPIO@";
static char *Test_output_gpio = "@TEST_OUTPUT_GPIO@";


// ---------------------- Public methods

int init_gpio_api(void)
{
	if (load_gpio_names() != 0)
		return -1;

	if (register_api_command("list-gpio-names", "lsgp", "List all GPIO names.", list_gpio_names))
		return -1;

	if (register_api_command("get-gpio-direction", "ggpd", "Read the current direction of a GPIO line.", get_gpio_direction))
		return -1;

	if (register_api_command("request-gpio-for-input", "rgpi", "Request a given GPIO line for input.", request_gpio_for_input))
		return -1;

	if (register_api_command("request-gpio-for-output", "rgpo", "Request a given GPIO line for output.", request_gpio_for_output))
		return -1;

	if (register_api_command("release-gpio", "rlgp", "Release a previously requested GPIO line.", release_gpio))
		return -1;

	if (register_api_command("read-gpio-value", "rdgp", "Read the value from an input GPIO line.", read_gpio_value))
		return -1;

	if (register_api_command("write-gpio-value", "wrgp", "Write a value on an output GPIO line.", write_gpio_value))
		return -1;

	if (register_api_command("wait-gpio-edge", "wgpe", "Wait for a specific signal edge on a GPIO line.", wait_gpio_edge))
		return -1;

	if (register_api_command("get-test-input-gpio", "gtig", "Get a GPIO name suitable for a test in input.", get_test_input_gpio))
		return -1;

	if (register_api_command("get-test-output-gpio", "gtog", "Get a GPIO name suitable for a test in output.", get_test_output_gpio))
		return -1;

	return 0;
}


// ---------------------- Private methods

static int load_gpio_names(void)
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
			if (Eris_gpios == NULL)
				return -1;

			Eris_gpios[Gpio_count].offset = gpiod_line_info_get_offset(line);
			Eris_gpios[Gpio_count].request = NULL;
			Eris_gpios[Gpio_count].chip = chip;
			Eris_gpios[Gpio_count].output = -1;
			char *name = gpiod_line_info_get_name(line);
			if (name == NULL)
				name = "";
			Eris_gpios[Gpio_count].name = malloc(strlen(name) + 1);
			if (Eris_gpios[Gpio_count].name != NULL)
				strcpy(Eris_gpios[Gpio_count].name, name);
			Gpio_count ++;
			gpiod_line_info_free(line);
		}
		gpiod_chip_info_free(info);

		// Don't close `chip`, it will be used (via Eris_gpios[Gpio_count].chip) to request lines.
		// gpiod_chip_close(chip);
        }
        closedir(dir);
	return 0;
}



static void list_gpio_names(int sock, int argc, char *argv[])
{
        char *reply = NULL;
        size_t size = 0;
        size_t pos  = 0;

	if (argc > 0) {
		send_error(sock, EINVAL, "list-gpio-names doesn't take any argument.");
		return;
	}

	for (int i = 0; i < Gpio_count; i++) {
		if (pos != 0)
			addsnprintf(&reply, &size, &pos, " ");
		addsnprintf(&reply, &size, &pos, "%s", Eris_gpios[i].name);
	}
	if (reply == NULL) {
	        send_error(sock, ENODEV, "No GPIO available.");
		return;
	}
        send_reply(sock, strlen(reply), reply);
        free(reply);
}



static void get_gpio_direction(int sock, int argc, char *argv[])
{
	int num;

	if (argc < 1) {
		send_error(sock, EINVAL, "get-gpio-direction command needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "get-gpio-direction takes only one argument.");
		return;
	}

	for (num = 0; num < Gpio_count; num ++) {
		if (strcmp(argv[0], Eris_gpios[num].name) == 0)
			break;
	}
	if (num >= Gpio_count) {
		send_error(sock, ENODEV, "The GPIO name does not exist.");
		return;
	}

	struct gpiod_line_info *info = gpiod_chip_get_line_info(Eris_gpios[num].chip, Eris_gpios[num].offset);
	if (! info) {
		send_error(sock, EBUSY, "The GPIO line does not exists.");
		return;
	}

	if (gpiod_line_info_get_direction(info) == GPIOD_LINE_DIRECTION_INPUT)
	        send_reply(sock, 5, "Input");
	else
	        send_reply(sock, 6, "Output");

	gpiod_line_info_free(info);
}



static void request_gpio_for_input(int sock, int argc, char *argv[])
{
	int num;

	if (argc < 1) {
		send_error(sock, EINVAL, "request-gpio-for-input command needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "request-gpio-for-input takes only one argument.");
		return;
	}

	for (num = 0; num < Gpio_count; num ++) {
		if (strcmp(argv[0], Eris_gpios[num].name) == 0)
			break;
	}
	if (num >= Gpio_count) {
		send_error(sock, ENODEV, "The GPIO name does not exist.");
		return;
	}
	if (Eris_gpios[num].request != NULL) {
		send_error(sock, EBUSY, "The GPIO line is already reserved.");
		return;
	}

	struct gpiod_line_settings *settings;
	settings = gpiod_line_settings_new();
	if (settings == NULL) {
		send_error(sock, ENOMEM, "Memory allocation error.");
		return;
	}

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

	struct gpiod_line_config *config = gpiod_line_config_new();
	if (config == NULL) {
		gpiod_line_settings_free(settings);
		send_error(sock, ENOMEM, "Memory allocation error.");
		return;
	}

	if (gpiod_line_config_add_line_settings(config, &(Eris_gpios[num].offset), 1, settings) != 0) {
		send_error(sock, errno, "Unable to obtain this GPIO line.");
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
		return;
	}

	struct gpiod_request_config * rconfig = gpiod_request_config_new();
	if  (rconfig == NULL) {
		send_error(sock, errno, "Memory allocation error.");
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
	}
	gpiod_request_config_set_consumer(rconfig, "Eris API");

	Eris_gpios[num].request = gpiod_chip_request_lines(Eris_gpios[num].chip, rconfig, config);
	Eris_gpios[num].output = 0;

	gpiod_request_config_free(rconfig);
	gpiod_line_config_free(config);
	gpiod_line_settings_free(settings);

        send_reply(sock, 2, "Ok");
}



static void request_gpio_for_output(int sock, int argc, char *argv[])
{
	int num;

	if (argc < 2) {
		send_error(sock, EINVAL, "request-gpio-for-output command needs two arguments.");
		return;
	}
	if (argc > 2) {
		send_error(sock, EINVAL, "request-gpio-for-output takes only two arguments.");
		return;
	}
	for (num = 0; num < Gpio_count; num ++) {
		if (strcmp(argv[0], Eris_gpios[num].name) == 0)
			break;
	}
	if (num >= Gpio_count) {
		send_error(sock, ENODEV, "The GPIO name does not exist.");
		return;
	}
	if (Eris_gpios[num].request != NULL) {
		send_error(sock, EBUSY, "The GPIO line is already reserved.");
		return;
	}

	int value;
	if ((sscanf(argv[1], "%d", &value) != 1)
	 || (value < 0)
	 || (value > 1)) {
		send_error(sock, EINVAL, "The value is invalid.");
		return;
	}

	struct gpiod_line_settings *settings;
	settings = gpiod_line_settings_new();
	if (settings == NULL) {
		send_error(sock, errno, "Memory allocation error.");
		return;
	}

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, value);

	struct gpiod_line_config *config = gpiod_line_config_new();
	if (config == NULL) {
		send_error(sock, errno, "Memory allocation error.");
		gpiod_line_settings_free(settings);
		return;
	}

	if (gpiod_line_config_add_line_settings(config, &(Eris_gpios[num].offset), 1, settings) != 0) {
		send_error(sock, errno, "Unable to obtain this GPIO line.");
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
		return;
	}

	struct gpiod_request_config * rconfig = gpiod_request_config_new();
	if  (rconfig == NULL) {
		send_error(sock, errno, "Memory allocation error.");
		gpiod_line_config_free(config);
		gpiod_line_settings_free(settings);
	}
	gpiod_request_config_set_consumer(rconfig, "Eris API");

	Eris_gpios[num].request = gpiod_chip_request_lines(Eris_gpios[num].chip, rconfig, config);
	Eris_gpios[num].output = 1;

	gpiod_request_config_free(rconfig);
	gpiod_line_config_free(config);
	gpiod_line_settings_free(settings);

        send_reply(sock, 2, "Ok");
}



static void release_gpio(int sock, int argc, char *argv[])
{
	int num;

	if (argc < 1) {
		send_error(sock, EINVAL, "release-gpio needs an argument.");
		return;
	}
	if (argc > 1) {
		send_error(sock, EINVAL, "release-gpio takes only one argument.");
		return;
	}
	for (num = 0; num < Gpio_count; num ++) {
		if (strcmp(argv[0], Eris_gpios[num].name) == 0)
			break;
	}
	if (num >= Gpio_count) {
		send_error(sock, ENODEV, "The GPIO name does not exist.");
		return;
	}
	if (Eris_gpios[num].request == NULL) {
		send_error(sock, ENODEV, "The GPIO line is already free.");
		return;
	}

	gpiod_line_request_release(Eris_gpios[num].request);

	Eris_gpios[num].request = NULL;

        send_reply(sock, 2, "Ok");
}



static void read_gpio_value(int sock, int argc, char *argv[])
{
	int num;

        if (argc < 1) {
                send_error(sock, EINVAL, "read-gpio-value needs an argument.");
                return;
        }
	if (argc > 1) {
		send_error(sock, EINVAL, "read-gpio-value takes only one argument.");
		return;
	}
        for (num = 0; num < Gpio_count; num ++) {
                if (strcmp(argv[0], Eris_gpios[num].name) == 0)
                        break;
        }
        if (num >= Gpio_count) {
                send_error(sock, ENODEV, "The GPIO name does not exist.");
                return;
        }
        if (Eris_gpios[num].request == NULL) {
                send_error(sock, ENODEV, "This GPIO line is not reserved.");
                return;
        }
	if (Eris_gpios[num].output != 0){
                send_error(sock, EIO, "This GPIO line is not readable.");
                return;
	}

	int value = gpiod_line_request_get_value(Eris_gpios[num].request, Eris_gpios[num].offset);

        char *reply = NULL;
        size_t size = 0;
        size_t pos  = 0;

	addsnprintf(&reply, &size, &pos, "%d", value);
	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void write_gpio_value(int sock, int argc, char *argv[])
{
	int num;

        if (argc < 2) {
                send_error(sock, EINVAL, "write-gpio-value needs two arguments.");
                return;
        }
	if (argc > 2) {
		send_error(sock, EINVAL, "write-gpio-value takes only two arguments.");
		return;
	}
        for (num = 0; num < Gpio_count; num ++) {
                if (strcmp(argv[0], Eris_gpios[num].name) == 0)
                        break;
        }
        if (num >= Gpio_count) {
                send_error(sock, ENODEV, "The GPIO name does not exist.");
                return;
        }
        if (Eris_gpios[num].request == NULL) {
                send_error(sock, ENODEV, "This GPIO line is not reserved.");
                return;
        }
	if (Eris_gpios[num].output != 1){
                send_error(sock, EIO, "This GPIO line is not writable.");
                return;
	}

	int value;
	if ((sscanf(argv[1], "%d", &value) != 1)
	 || (value < 0)
	 || (value > 1)) {
		send_error(sock, EINVAL, "The value is invalid.");
		return;
	}

	gpiod_line_request_set_value(Eris_gpios[num].request, Eris_gpios[num].offset, value);

        send_reply(sock, 2, "Ok");
}



static void wait_gpio_edge(int sock, int argc, char *argv[])
{
	int num;

        if (argc < 2) {
                send_error(sock, EINVAL, "wait-gpio-edge needs two argument.");
                return;
        }
	if (argc > 2) {
		send_error(sock, EINVAL, "wait-gpio-edge takes only two arguments.");
		return;
	}
        for (num = 0; num < Gpio_count; num ++) {
                if (strcmp(argv[0], Eris_gpios[num].name) == 0)
                        break;
        }
        if (num >= Gpio_count) {
                send_error(sock, ENODEV, "The GPIO name does not exist.");
                return;
        }
        if (Eris_gpios[num].request == NULL) {
                send_error(sock, ENODEV, "This GPIO line is not reserved.");
                return;
        }
	if (Eris_gpios[num].output != 0){
                send_error(sock, EIO, "This GPIO line is not readable.");
                return;
	}

	enum gpiod_edge_event_type evtype;

	if (strncasecmp(argv[1], "ris", 3) == 0) {
		evtype = GPIOD_EDGE_EVENT_RISING_EDGE;
	} else if (strncasecmp(argv[1], "fal", 3) == 0) {
		evtype = GPIOD_EDGE_EVENT_FALLING_EDGE;
	} else {
                send_error(sock, EINVAL, "This event type is invalid.");
                return;
	}

	struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(64);

	for (;;) {
		int n = gpiod_line_request_wait_edge_events(Eris_gpios[num].request, -1);
		if (n == 0)
			break;
		if (n < 0) {
			send_error(sock, errno, "Unable to wait events on this GPIO line.");
			break;
		}
		n = gpiod_line_request_read_edge_events(Eris_gpios[num].request, buffer, 64);
		if (n < 0) {
			send_error(sock, errno, "Unable to read events on this GPIO line.");
			break;
		}
		if (n == 0) {
			send_reply(sock, 7, "Timeout");
			break;
		}
		for (int i = 0; i < n; i ++) {
			struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, i);
			if (gpiod_edge_event_get_event_type(event) == evtype) {
				send_reply(sock, 2, "Ok");
				goto out_buffer_free;
			}
		}
	}

	out_buffer_free:
	gpiod_edge_event_buffer_free(buffer);
}


static void get_test_input_gpio(int sock, int argc, char *argv[])
{
	send_reply(sock, strlen(Test_input_gpio), Test_input_gpio);
}


static void get_test_output_gpio(int sock, int argc, char *argv[])
{
	send_reply(sock, strlen(Test_output_gpio), Test_output_gpio);
}
