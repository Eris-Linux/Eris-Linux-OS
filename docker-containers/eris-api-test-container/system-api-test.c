/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2026.
   Copyright 2026 Logilin. All rights reserved.
*/
#include <liberis.h>

#include "eris-api-test.h"
#include "system-api-test.h"


// ---------------------- Private macros.

// ---------------------- Private method declarations.

// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int system_info_api_test(int sockfd)
{
	char buffer[BUFFER_SIZE];
	int err;

	sockprintf(sockfd, "\r\n**** Eris Linux System Informations *****\r\n\n");
	sockprintf(sockfd, "    Image type....     ");
	err = eris_get_system_type(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return -1;
	}
	sockprintf(sockfd, "    Board model....    ");
	err = eris_get_system_model(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return -1;
	}

	sockprintf(sockfd, "    System version.... ");
	err = eris_get_system_version(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return -1;
	}

	sockprintf(sockfd, "    System UUID....    ");
	err = eris_get_system_uuid(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return -1;
	}

	sockprintf(sockfd, "\r\n");
	sockprintf(sockfd, "    Number of slots for containers: %d\r\n", eris_get_number_of_slots());


	sockprintf(sockfd, "    Containers:\r\n");
	for (int slot = 0; slot < eris_get_number_of_slots(); slot++) {

		if (! eris_get_container_presence(slot))
			continue;

		sockprintf(sockfd, "      #%d ", slot + 1);

		err = eris_get_container_name(slot, buffer, BUFFER_SIZE);
		if (err == 0) {
			sockprintf(sockfd, "%-32s ", buffer);
		} else {
			sockprintf(sockfd, "ERROR %d\r\n", err);
			return -1;
		}

		err = eris_get_container_version(slot, buffer, BUFFER_SIZE);
		if (err == 0) {
			sockprintf(sockfd, "%-32s ", buffer);
		} else {
			sockprintf(sockfd, "ERROR %d\r\n", err);
			return -1;
		}
		sockprintf(sockfd, "\r\n");
	}
	sockprintf(sockfd, "\r\n\n");

	return 0;
}


// ---------------------- Private methods
