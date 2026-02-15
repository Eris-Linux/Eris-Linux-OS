/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "gpio-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int get_list_of_gpio        (int sockfd);
static int request_gpio_for_input  (int sockfd);
static int request_gpio_for_output (int sockfd);
static int release_gpio            (int sockfd);
static int read_gpio_value         (int sockfd);
static int write_gpio_value        (int sockfd);
static int wait_for_gpio_edge      (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int gpio_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux GPIO API *****\r\n\n");
		sockprintf(sockfd, "1:  Get list of GPIOs         5:  Read GPIO value            \r\n");
		sockprintf(sockfd, "2:  Request GPIO for input    6:  Write GPIO value           \r\n");
		sockprintf(sockfd, "3:  Request GPIO for output   7:  Wait for edge (BROKEN!)    \r\n");
		sockprintf(sockfd, "4:  Release GPIO                                             \r\n");
		sockprintf(sockfd, "0:  Return                                                   \r\n");
		
		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;
			if (choice[0] == '\0')
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (get_list_of_gpio(sockfd) != 0)
					return -1;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (request_gpio_for_input(sockfd) != 0)
					return -1;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (request_gpio_for_output(sockfd) != 0)
					return -1;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (release_gpio(sockfd) != 0)
					return -1;
				continue;
			}				

			if (strcmp(choice, "5") == 0) {
				if (read_gpio_value(sockfd) != 0)
					return -1;
				continue;
			}

			if (strcmp(choice, "6") == 0) {
				if (write_gpio_value(sockfd) != 0)
					return -1;
				continue;
			}

			if (strcmp(choice, "7") == 0) {
				if (wait_for_gpio_edge(sockfd) != 0)
					return -1;
				continue;
			}

			sockprintf(sockfd, "INVALID CHOICE");
			break;
		}
	}
	return 0;
}


// ---------------------- Private methods

static int get_list_of_gpio(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err = eris_get_list_of_gpio(buffer, BUFFER_SIZE);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return -1;
	}
	sockprintf(sockfd, "%s\r\n", buffer);
	return 0;
}



static int request_gpio_for_input(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	int err = eris_request_gpio_for_input(name);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return 0;
	}
	sockprintf(sockfd, "Ok\r\n");
	return 0;
}



static int request_gpio_for_output(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	sockprintf(sockfd, "Enter the initial value: ");
	char value[64];
	if (sockgets(sockfd, value, 64) == NULL)
		return -1;
	if (value[0] == '\0')
		return 0;

	sockprintf(sockfd, "Be careful, this operation may be dangerous. Are you sure (Y/N)? ");
	char confirm[32];
	if (sockgets(sockfd, confirm, 32) == NULL)
		return -1;
	if ((confirm[0] != 'y') && (confirm[0] != 'Y')) {
		sockprintf(sockfd, "Canceled\r\n");
		return 0;
	}

	int err = eris_request_gpio_for_output(name, strcmp(value, "0") == 0 ? 0 : 1);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return 0;
	}
	sockprintf(sockfd, "Ok\r\n");
	return 0;
}



static int release_gpio(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	int err = eris_release_gpio(name);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return 0;
	}
	sockprintf(sockfd, "Ok\r\n");
	return 0;
}



static int read_gpio_value(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	int ret = eris_read_gpio_value(name);
	if (ret < 0) {
		sockprintf(sockfd, "ERROR %d (are you sure you have requested the GPIO for input?)\r\n", ret);
		return 0;
	}
	sockprintf(sockfd, "Value: %d\r\n", ret);
	return 0;
}



static int write_gpio_value(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	sockprintf(sockfd, "Enter the new value: ");
	char value[64];
	if (sockgets(sockfd, value, 64) == NULL)
		return -1;
	if (value[0] == '\0')
		return 0;

	int err = eris_write_gpio_value(name, strcmp(value, "0") == 0 ? 0 : 1);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d (are you sure you have requested the GPIO for output?)\r\n", err);
		return 0;
	}
	sockprintf(sockfd, "Ok\r\n");
	return 0;
}



static int wait_for_gpio_edge(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the GPIO: ");

	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	sockprintf(sockfd, "Enter the waited edge ('rising' or 'falling'): ");
	char event[64];
	if (sockgets(sockfd, event, 64) == NULL)
		return -1;
	if (event[0] == '\0')
		return 0;

	int err = eris_wait_gpio_edge(name, event);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d (are you sure you have requested the GPIO for input?)\r\n", err);
		return 0;
	}
	sockprintf(sockfd, "Ok\r\n");
	return 0;
}

