/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2026.
   Copyright 2026 Logilin. All rights reserved.
*/

#include <stdio.h>
#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "update-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int get_update_status           (int sockfd);
static int get_reboot_needed_flag      (int sockfd);
static int set_reboot_needed_flag      (int sockfd);
static int get_server_contact_period   (int sockfd);
static int set_server_contact_period   (int sockfd);
static int contact_server_now          (int sockfd);
static int get_automatic_reboot_flag   (int sockfd);
static int set_automatic_reboot_flag   (int sockfd);
static int get_container_update_policy (int sockfd);
static int set_container_update_policy (int sockfd);
static int reboot_now                  (int sockfd);
static int force_rollback              (int sockfd);
static int restore_factory_preset      (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int update_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux System & Containers Update *****\r\n\n");
		sockprintf(sockfd, "1: Get Update Status            8: Set 'Automatic Reboot' Flag \r\n");
		sockprintf(sockfd, "2: Get 'Reboot Needed' Flag     9: Get Container Update Policy  \r\n");
		sockprintf(sockfd, "3: Set 'Reboot Needed' Flag    10: Set Container Update Policy \r\n");
		sockprintf(sockfd, "4: Get Server Contact Period   11: Reboot Now                  \r\n");
		sockprintf(sockfd, "5: Set Server Contact Period   12: Force System Rollback       \r\n");
		sockprintf(sockfd, "6: Contact the Server Now      13: Restore Factory Presets     \r\n");
		sockprintf(sockfd, "7: Get 'Automatic Reboot' Flag                                 \r\n");
		sockprintf(sockfd, "0: Return                                                      \r\n");

		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (get_update_status(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (get_reboot_needed_flag(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (set_reboot_needed_flag(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (get_server_contact_period(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "5") == 0) {
				if (set_server_contact_period(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "6") == 0) {
				if (contact_server_now(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "7") == 0) {
				if (get_automatic_reboot_flag(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "8") == 0) {
				if (set_automatic_reboot_flag(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "9") == 0) {
				if (get_container_update_policy(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "10") == 0) {
				if (set_container_update_policy(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "11") == 0) {
				if (reboot_now(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "12") == 0) {
				if (force_rollback(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "13") == 0) {
				if (restore_factory_preset(sockfd) != 0)
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

static int get_update_status(int sockfd)
{
	int ret = eris_get_system_update_status();
	if (ret < 0) {
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	} else {
		switch(ret) {
			case 1: sockprintf(sockfd, "System is OK, no update is pending.\r\n"); break;
			case 2: sockprintf(sockfd, "System update install in progress.\r\n"); break;
			case 3: sockprintf(sockfd, "System update install Ok. Ready to reboot.\r\n"); break;
			case 4: sockprintf(sockfd, "System update install failed.\r\n"); break;
			case 5: sockprintf(sockfd, "System reboot in progress.\r\n"); break;
			default: sockprintf(sockfd, "Unkown update status.\r\n"); break;
		}
	}
	return 0;
}



static int get_reboot_needed_flag(int sockfd)
{
	int ret = eris_get_reboot_needed_flag();
	if (ret < 0)
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	else if (ret == 0)
		sockprintf(sockfd, "No reboot is pending\r\n");
	else if (ret == 1)
		sockprintf(sockfd, "A reboot will occur at the next server contact\r\n");
	return 0;
}



static int set_reboot_needed_flag(int sockfd)
{
	sockprintf(sockfd, "Program a reboot at next server contact ('Yes' or 'No'): ");
	char reply[64];
	if (sockgets(sockfd, reply, 64) == NULL)
		return -1;
	if (reply[0] == '\0')
		return 0;
	
	int ret = eris_set_reboot_needed_flag((reply[0] == 'Y') || (reply[0] == 'y'));
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	return 0;
}



static int get_server_contact_period(int sockfd)
{
	int ret = eris_get_server_contact_period();
	if (ret < 0)
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	else 
		sockprintf(sockfd, "Server contact period: %d s.\r\n", ret);
	return 0;
}



static int set_server_contact_period(int sockfd)
{
	sockprintf(sockfd, "Server contact period in seconds [0-86400]: ");

	char reply[64];
	if (sockgets(sockfd, reply, 64) == NULL)
		return -1;
	if (reply[0] == '\0')
		return 0;
	int period;
	if ((sscanf(reply, "%d", & period) != 1)
	 || (period < 0)
	 || (period > 86400))
		return 0;

	int ret = eris_set_server_contact_period(period);
	if (ret == 0) {
		sockprintf(sockfd, "Ok\r\n");
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	}
	return 0;
}



static int contact_server_now(int sockfd)
{
	int ret = eris_contact_server();
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	return 0;
}



static int get_automatic_reboot_flag(int sockfd)
{
	int ret = eris_get_automatic_reboot_flag();
	if (ret < 0)
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	else if (ret == 0)
		sockprintf(sockfd, "The system won't reboot after update.\r\n");
	else if (ret == 1)
		sockprintf(sockfd, "The system will automatically reboot after update.\r\n");
	return 0;
}



static int set_automatic_reboot_flag(int sockfd)
{
	sockprintf(sockfd, "Automatically reboot the system after an update ('Yes' or 'No')? ");
	char reply[64];
	if (sockgets(sockfd, reply, 64) == NULL)
		return -1;
	if (reply[0] == '\0')
		return 0;
	
	int ret = eris_set_automatic_reboot_flag((reply[0] == 'Y') || (reply[0] == 'y'));
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	return 0;
}



static int get_container_update_policy(int sockfd)
{
	int ret = eris_get_container_update_policy();
	if (ret < 0)
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	else if (ret == 0)
		sockprintf(sockfd, "The containers are updated only at system reboot.\r\n");
	else if (ret == 1)
		sockprintf(sockfd, "The containers are updated as soon as possible.\r\n");
	return 0;
}



static int set_container_update_policy(int sockfd)
{
	sockprintf(sockfd, "Update containers only at system reboot ('0') or as soon as possible ('1')? ");
	char reply[64];
	if (sockgets(sockfd, reply, 64) == NULL)
		return -1;
	if (reply[0] == '\0')
		return 0;
	
	int ret = eris_set_container_update_policy((reply[0] == '1'));
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	return 0;
}



static int reboot_now(int sockfd)
{
	int ret = eris_reboot();
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	return 0;
}



static int force_rollback(int sockfd)
{
	/*
	int ret = eris_rollback();
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	*/
	sockprintf(sockfd, "This feature is not implemented yet\r\n");
	return 0;
}



static int restore_factory_preset(int sockfd)
{
	/*
	int ret = eris_restore_factory_preset();
	if (ret == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", ret);
	*/
	sockprintf(sockfd, "This feature is not implemented yet\r\n");
	return 0;
}


