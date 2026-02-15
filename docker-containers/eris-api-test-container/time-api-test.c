/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2026.
   Copyright 2026 Logilin. All rights reserved.
*/

#include <stdlib.h>
#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "time-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int get_ntp_enable  (int sockfd);
static int set_ntp_enable  (int sockfd);
static int get_ntp_server  (int sockfd);
static int set_ntp_server  (int sockfd);
static int list_time_zones (int sockfd);
static int get_time_zone   (int sockfd);
static int set_time_zone   (int sockfd);
static int get_local_time  (int sockfd);
static int get_system_time (int sockfd);
static int set_system_time (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int time_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux Time Features *****\r\n\n");
		sockprintf(sockfd, "1:  Get NTP status           6: Get local time zone         \r\n");
		sockprintf(sockfd, "2:  Set NTP status           7: Set local time zone         \r\n");
		sockprintf(sockfd, "3:  Get NTP server           8: Get local time              \r\n");
		sockprintf(sockfd, "4:  Set NTP server           9: Get system time             \r\n");
		sockprintf(sockfd, "5:  List of time zones      10: Set system time             \r\n");
		sockprintf(sockfd, "0:  Return                                                  \r\n");

		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (get_ntp_enable(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (set_ntp_enable(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (get_ntp_server(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (set_ntp_server(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "5") == 0) {
				if (list_time_zones(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "6") == 0) {
				if (get_time_zone(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "7") == 0) {
				if (set_time_zone(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "8") == 0) {
				if (get_local_time(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "9") == 0) {
				if (get_system_time(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "10") == 0) {
				if (set_system_time(sockfd) != 0)
					break;
				continue;
			}

			sockprintf(sockfd, "INVALID CHOICE");
			break;
		}
	}
	return 0;
}


// ---------------------- Private methods

static int get_ntp_enable(int sockfd)
{
	char buffer[64];

	int err = eris_get_ntp_enable(buffer, 64);
	if (err == 0) {
		sockprintf(sockfd, "NTP in use: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int set_ntp_enable  (int sockfd)
{
	sockprintf(sockfd, "Enter the use ('yes' or 'no') of the NTP protocol: ");
	char status[64];
	if (sockgets(sockfd, status, 64) == NULL)
		return -1;
	if (status[0] == '\0')
		return 0;
	
	int err = eris_set_ntp_enable(status);
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_ntp_server(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err = eris_get_ntp_server(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "NTP time server is at: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int set_ntp_server(int sockfd)
{
	sockprintf(sockfd, "Enter the address of the NTP time server: ");
	char address[64];
	if (sockgets(sockfd, address, 64) == NULL)
		return -1;
	if (address[0] == '\0')
		return 0;
	
	int err = eris_set_ntp_server(address);
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}




static int list_time_zones(int sockfd)
{
	char *buffer = malloc(16384);
	if (buffer == NULL)
		return -1;
	int err = eris_list_time_zones(buffer, 16384);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	free(buffer);
	return 0;
}



static int get_time_zone(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err = eris_get_time_zone(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "Time zone of the device: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int set_time_zone(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the time zone of the device: ");
	char timezone[64];
	if (sockgets(sockfd, timezone, 64) == NULL)
		return -1;
	if (timezone[0] == '\0')
		return 0;
	
	int err = eris_set_time_zone(timezone);
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_local_time(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err = eris_get_local_time(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "Local time of the device: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;

}



static int get_system_time(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err = eris_get_system_time(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "UTC System time of the device: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int set_system_time(int sockfd)
{
	sockprintf(sockfd, "Enter the UTC system time (format YYYY:MM:DD:hh:mm:ss): ");
	char systime[64];
	if (sockgets(sockfd, systime, 64) == NULL)
		return -1;
	if (systime[0] == '\0')
		return 0;
	
	int err = eris_set_system_time(systime);
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}

