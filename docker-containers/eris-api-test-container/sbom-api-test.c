/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2026.
   Copyright 2026 Logilin. All rights reserved.
*/

#include <stdlib.h>
#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "sbom-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int get_list_of_packages (int sockfd);
static int get_package_version  (int sockfd);
static int get_package_licenses (int sockfd);
static int get_list_of_licenses (int sockfd);
static int get_license_text     (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int sbom_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux Software Bill of Materials *****\r\n\n");
		sockprintf(sockfd, "1:  Get package list         4: Get license list            \r\n");
		sockprintf(sockfd, "2:  Get package version      5: Get license text            \r\n");
		sockprintf(sockfd, "3:  Get package license(s)                                  \r\n");
		sockprintf(sockfd, "0:  Return                                                  \r\n");

		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (get_list_of_packages(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (get_package_version(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (get_package_licenses(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (get_list_of_licenses(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "5") == 0) {
				if (get_license_text(sockfd) != 0)
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

static int get_list_of_packages(int sockfd)
{
	char buffer[4096];

	int err = eris_get_list_of_packages(buffer, 4096);
	if (err == 0) {
		sockprintf(sockfd, "List of packages : %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_package_version(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the package: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	char buffer[BUFFER_SIZE];
	int err = eris_get_package_version(name, buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_package_licenses(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the package: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	char buffer[BUFFER_SIZE];
	int err = eris_get_package_licenses(name, buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_list_of_licenses(int sockfd)
{
	char buffer[4096];
	int err = eris_get_list_of_licenses(buffer, 4096);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_license_text(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the license: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	char *buffer = malloc(32768);
	if (buffer == NULL)
		return 0;
	int err = eris_get_license_text(name, buffer, 32768);
	if (err == 0) {
		sockprintf(sockfd, "%s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	free(buffer);
	return 0;
}


