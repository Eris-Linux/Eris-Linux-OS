/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025-2026 Logilin. All rights reserved.
*/

#include <string.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "network-api-test.h"

// ---------------------- Private macros.

// ---------------------- Private method declarations.

static int get_list_of_interfaces  (int sockfd);
static int get_interface_status    (int sockfd);
static int set_interface_status    (int sockfd);
static int get_interface_config    (int sockfd);
static int set_interface_config    (int sockfd);
static int is_interface_wireless   (int sockfd);
static int get_nameserver_addr     (int sockfd);
static int set_nameserver_addr     (int sockfd);
static int scan_wifi_access_points (int sockfd);
static int connect_to_wifi_ap      (int sockfd);
static int disconnect_from_wifi_ap (int sockfd);
static int get_wifi_quality        (int sockfd);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int network_api_test(int sockfd)
{
	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux Network Interfaces Setup *****\r\n\n");
		sockprintf(sockfd, "1:  Get list of interfaces    7: Get nameserver address      \r\n");
		sockprintf(sockfd, "2:  Get interface status      8: Set nameserver address      \r\n");
		sockprintf(sockfd, "3:  Set interface status      9: Scan Wifi Access Points     \r\n");
		sockprintf(sockfd, "4:  Get interface config     10: Connect to Wifi AP (BROKEN?)\r\n");
		sockprintf(sockfd, "5:  Set interface config     11: Disconnect from Wifi A.P.   \r\n");
		sockprintf(sockfd, "6:  Is interface wireless    12: Get Wifi quality            \r\n");
		sockprintf(sockfd, "0:  Return                                                   \r\n");

		for (;;) {
			char choice[32];
			
			sockprintf(sockfd, "\r\nYour choice: ");
			if (sockgets(sockfd, choice, 32) == NULL)
				break;

			if (strcmp(choice, "0") == 0)
				return 0;

			if (strcmp(choice, "1") == 0) {
				if (get_list_of_interfaces(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "2") == 0) {
				if (get_interface_status(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "3") == 0) {
				if (set_interface_status(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "4") == 0) {
				if (get_interface_config(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "5") == 0) {
				if (set_interface_config(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "6") == 0) {
				if (is_interface_wireless(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "7") == 0) {
				if (get_nameserver_addr(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "8") == 0) {
				if (set_nameserver_addr(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "9") == 0) {
				if (scan_wifi_access_points(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "10") == 0) {
				if (connect_to_wifi_ap(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "11") == 0) {
				if (disconnect_from_wifi_ap(sockfd) != 0)
					break;
				continue;
			}

			if (strcmp(choice, "12") == 0) {
				if (get_wifi_quality(sockfd) != 0)
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

static int get_list_of_interfaces(int sockfd)
{
	char buffer[BUFFER_SIZE];

	int err = eris_get_list_of_network_interfaces(buffer, BUFFER_SIZE);
	if (err == 0) {
		sockprintf(sockfd, "List of network interfaces : %s\r\n", buffer);
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int get_interface_status(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	char buffer[BUFFER_SIZE];
	int err = eris_get_network_interface_status(name, buffer, BUFFER_SIZE);
	if (err == 0) {
		int start = 0;
		int end = 0;
		while ((buffer[end] != ' ') && (buffer[end] != '\0'))
			end++;
		if (buffer[end] =='\0')
			return 0;
		buffer[end] = '\0';
		sockprintf(sockfd, "Status: %s\r\n", buffer);
		end ++;
		start = end;
		while ((buffer[end] != ' ') && (buffer[end] != '\0'))
			end++;
		if (buffer[end] =='\0')
			return 0;
		buffer[end] = '\0';
		sockprintf(sockfd, "Address: %s\r\n", &(buffer[start]));
		end ++;
		start = end;
		while ((buffer[end] != ' ') && (buffer[end] != '\0'))
			end++;
		if (buffer[end] =='\0')
			return 0;
		buffer[end] = '\0';
		sockprintf(sockfd, "Subnet mask: %s\r\n", &(buffer[start]));
		end ++;
		start = end;
		while ((buffer[end] != ' ') && (buffer[end] != '\0'))
			end++;
		buffer[end] = '\0';
		sockprintf(sockfd, "Gateway: %s\r\n", &(buffer[start]));
	} else {
		sockprintf(sockfd, "ERROR %d\r\n", err);
	}
	return 0;
}



static int set_interface_status(int sockfd)
{
	sockprintf(sockfd, "The status will be updated immediately.\r\n");
	sockprintf(sockfd, "Do not shut down the interface you are connected to!\r\n");

	sockprintf(sockfd, "Enter the name of the interface: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	sockprintf(sockfd, "Enter the status ('up' or 'down'): ");
	char status[64];
	if (sockgets(sockfd, status, 64) == NULL)
		return -1;
	if (status[0] == '\0')
		return 0;
	int err = eris_set_network_interface_status(name, status);
	if (err == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int get_interface_config(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface: ");
	char buffer[BUFFER_SIZE];
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	int err = eris_get_network_interface_config(name, buffer, BUFFER_SIZE);
	if (err != 0) {
		sockprintf(sockfd, "ERROR %d\r\n", err);
		return 0;
	}
	char *token;
	if ((token = strtok(buffer, " ")) == NULL)
		return 0;
	sockprintf(sockfd, "    Interface name: %s\r\n", token);
	if ((token = strtok(NULL, " ")) == NULL)
		return 0;
	if (strcmp(token, "atboot") == 0) {
		sockprintf(sockfd, "    Activated at boot\r\n");
	} else {
		sockprintf(sockfd, "    Activated on demand\r\n");
	}
	if ((token = strtok(NULL, " ")) == NULL)
		return 0;
	if (strcmp(token, "dhcp") == 0) {
		sockprintf(sockfd, "    Dynamic address (using DHCP)\r\n");
	} else {
		sockprintf(sockfd, "    Static address\r\n");
		if ((token = strtok(NULL, " ")) == NULL)
			return 0;
		if (strcmp(token, "ipv6") == 0)
			sockprintf(sockfd, "    IP version 6\r\n");
		else
			sockprintf(sockfd, "    IP version 4\r\n");
		if ((token = strtok(NULL, " ")) == NULL)
			return 0;
		sockprintf(sockfd, "    IP address: %s\r\n", token);
		if ((token = strtok(NULL, " ")) == NULL)
			return 0;
		sockprintf(sockfd, "    Subnet mask: %s\r\n", token);
		if ((token = strtok(NULL, " ")) == NULL)
			return 0;
		sockprintf(sockfd, "    Gateway: %s\r\n", token);
	}
	return 0;
}



static int set_interface_config(int sockfd)
{
	int err;

	sockprintf(sockfd, "The new configuration will be applied at the next reboot.\r\n");

	sockprintf(sockfd, "Enter the name of the interface: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	sockprintf(sockfd, "Indicate when the interface must be activated ('1': at boot, '2': on demand): ");
	char activate[64];
	if (sockgets(sockfd, activate, 64) == NULL)
		return -1;
	if (strcmp(activate, "1") == 0)
		strcpy(activate, "atboot");
	else if (strcmp(activate, "2") == 0)
		strcpy(activate, "ondemand");
	else {
		sockprintf(sockfd, "The answer must be '1' or '2'!\r\n");
		return 0;
	}

	sockprintf(sockfd, "Indicate the interface addressing mode ('1': dynamically using DHCP, '2': statically): ");
	char mode[64];
	if (sockgets(sockfd, mode, 64) == NULL)
		return -1;
	if (strcmp(mode, "1") == 0) {
		err = eris_set_network_interface_config(name, activate, "dhcp", NULL, NULL, NULL, NULL);
	} else if (strcmp(mode, "2") != 0) {
		sockprintf(sockfd, "The answer must be '1' or '2'!\r\n");
		return 0;
	} else {
		sockprintf(sockfd, "Indicate the IP protocol version ('1': IPv4, '2': IPv6): ");
		char ip[64];
		if (sockgets(sockfd, ip, 64) == NULL)
			return -1;
		if (strcmp(ip, "1") == 0) {
			strcpy(ip, "ipv4");
		} else if (strcmp(ip, "2") == 0) {
			strcpy(ip, "ipv6");
		} else {
			sockprintf(sockfd, "The answer must be '1' or '2'!\r\n");
			return 0;
		}

		sockprintf(sockfd, "Enter the IP address of the device (ex: '192.168.1.1'): ");
		char address[64];
		if (sockgets(sockfd, address, 64) == NULL)
			return -1;
		if (address[0] == '\0')
			return 0;

		sockprintf(sockfd, "Enter the IP mask of the sub-net (ex: '255.255.255.0'): ");
		char netmask[64];
		if (sockgets(sockfd, netmask, 64) == NULL)
			return -1;
		if (netmask[0] == '\0')
			return 0;

		sockprintf(sockfd, "Enter the IP address of the gateway (ex: '192.168.1.254'): ");
		char gateway[64];
		if (sockgets(sockfd, gateway, 64) == NULL)
			return -1;
				
		err = eris_set_network_interface_config(name, activate,"static", ip, address, netmask, gateway);
	}
	if (err == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int is_interface_wireless(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	int err;
	err = eris_is_network_interface_wireless(name);
	if (err == 1)
		sockprintf(sockfd, "%s is wireless\r\n", name);
	else if (err == 0)
		sockprintf(sockfd, "%s isn't wireless\r\n", name);
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int get_nameserver_addr(int sockfd)
{
	char buffer[BUFFER_SIZE];

	if (eris_get_nameserver_address(buffer, BUFFER_SIZE) == 0)
		sockprintf(sockfd, "DNS address: %s\r\n", buffer);
	return 0;
}



static int set_nameserver_addr(int sockfd)
{
	sockprintf(sockfd, "Enter the address of the Domain Name Server: ");
	char address[64];
	if (sockgets(sockfd, address, 64) == NULL)
	return -1;
	if (address[0] == '\0')
	return 0;
	
	int err;
	err = eris_set_nameserver_address(address);
	if (err == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int scan_wifi_access_points(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface to scan: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;
	
	int err;
	char reply[1024];
	err = eris_scan_wifi(name, reply, 1024);
	if (err == 0)
		sockprintf(sockfd, "%s\r\n", reply);
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);

	return 0;
}



static int connect_to_wifi_ap(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface to use: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	sockprintf(sockfd, "Enter the SSID of the Access Point to connect to: ");
	char ssid[64];
	if (sockgets(sockfd, ssid, 64) == NULL)
		return -1;
	if (ssid[0] == '\0')
		return 0;
	
	sockprintf(sockfd, "Enter the password of the Access Point: ");
	char passwd[64];
	if (sockgets(sockfd, passwd, 64) == NULL)
		return -1;
	if (passwd[0] == '\0')
		return 0;

	int err;
	err = eris_connect_wifi(name, ssid, passwd);
	if (err == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int disconnect_from_wifi_ap(int sockfd)
{
	int err;
	err = eris_disconnect_wifi();
	if (err == 0)
		sockprintf(sockfd, "Ok\r\n");
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}



static int get_wifi_quality(int sockfd)
{
	sockprintf(sockfd, "Enter the name of the interface: ");
	char name[64];
	if (sockgets(sockfd, name, 64) == NULL)
		return -1;
	if (name[0] == '\0')
		return 0;

	int err;
	char reply[512];
	err = eris_get_wifi_quality(name, reply, 512);
	if (err == 0)
		sockprintf(sockfd, "%s\r\n", reply);
	else
		sockprintf(sockfd, "ERROR %d\r\n", err);
	return 0;
}

