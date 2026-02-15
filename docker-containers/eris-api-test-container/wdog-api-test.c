/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2026.
   Copyright 2026 Logilin. All rights reserved.
*/

#include <stdio.h>
#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "sbom-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int feed_watchdog          (int sockfd);
static int disable_watchdog       (int sockfd);
static int get_watchdog_delay     (int sockfd);
static int set_watchdog_delay     (int sockfd);
static int start_watchdog_feeder  (int sockfd);
static int stop_watchdog_feeder   (int sockfd);
static int watchdog_feeder_status (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int wdog_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux Watchdog Features *****\r\n\n");
		sockprintf(sockfd, "1:  Feed the watchdog        5: Start the watchdog feeder   \r\n");
		sockprintf(sockfd, "2:  Disable the watchdog     6: Stop the watchdog feeder    \r\n");
		sockprintf(sockfd, "3:  Get watchdog delay       7: Get the feeder status       \r\n");
		sockprintf(sockfd, "4:  Set watchdog delay                                      \r\n");
		sockprintf(sockfd, "0:  Return                                                  \r\n");

		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (feed_watchdog(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (disable_watchdog(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (get_watchdog_delay(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (set_watchdog_delay(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "5") == 0) {
				if (start_watchdog_feeder(sockfd) != 0)
					break;
				continue;
			}
			if (strcmp(choice, "6") == 0) {
				if (stop_watchdog_feeder(sockfd) != 0)
					break;
				continue;
			}
			if (strcmp(choice, "7") == 0) {
				if (watchdog_feeder_status(sockfd) != 0)
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

static int feed_watchdog(int sockfd)
{
	int err = eris_feed_watchdog();
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d (no watchdog available ?)\r\n", err);
	}
	return 0;
}



static int disable_watchdog(int sockfd)
{
	int err = eris_disable_watchdog();
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d (no watchdog available ?)\r\n", err);
	}
	return 0;
}



static int get_watchdog_delay(int sockfd)
{
	int ret = eris_get_watchdog_delay();
	if (ret > 0) {
		sockprintf(sockfd, "Delay = %d s.\r\n", ret);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	}
	return 0;
}



static int set_watchdog_delay(int sockfd)
{
	sockprintf(sockfd, "Enter the watchdog delay in seconds [1-48]: ");
	char delay_str[64];
	if (sockgets(sockfd, delay_str, 64) == NULL)
		return -1;
	if (delay_str[0] == '\0')
		return 0;
	int delay;
	if (sscanf(delay_str, "%d", &delay) != 1)
		return 0;

	int err = eris_set_watchdog_delay(delay);
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int start_watchdog_feeder(int sockfd)
{
	int err = eris_start_watchdog_feeder();
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d (already running?)\r\n", err);
	}
	return 0;
}



static int stop_watchdog_feeder(int sockfd)
{
	int err = eris_stop_watchdog_feeder();
	if (err == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d (already stopped?)\r\n", err);
	}
	return 0;
}



static int watchdog_feeder_status(int sockfd)
{
	char buffer[64];

	int err = eris_watchdog_feeder_status(buffer, 64);
	if (err == 0) {
		sockprintf(sockfd, "Status: %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}

