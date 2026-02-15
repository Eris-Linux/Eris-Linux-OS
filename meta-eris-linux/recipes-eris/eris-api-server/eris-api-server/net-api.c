/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <ifaddrs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/socket.h>

#include "api-server.h"
#include "net-api.h"


// ---------------------- Private macros declarations.

#define ERIS_NETWORK_CONFIG_FILE     "/etc/eris-linux/network"
#define SYSTEM_NETWORK_CONFIG_FILE   "/etc/network/interfaces"
#define INTERFACE_NAME_LENGTH 32
#define IP_ADDRESS_LENGTH   64

#define EOL_CHAR(x) ((x == '\0') || (x == 0x23) || (x == '\n') || (x == '\r'))

// ---------------------- Private types definitions.

typedef struct {

	char  name[INTERFACE_NAME_LENGTH];
	int   at_boot;
	int   ipv6;
	int   dhcp;
	char  ip_address[IP_ADDRESS_LENGTH];
	char  ip_netmask[IP_ADDRESS_LENGTH];
	char  ip_gateway[IP_ADDRESS_LENGTH];

} network_interface_t;


// ---------------------- Public method declarations.

static void load_eris_network_configuration(void);
static void save_eris_network_configuration(void);
static void write_system_network_configuration(void);

static void list_network_interfaces(int sock, int argc, char *argv[]);
static void get_network_interface_status(int sock, int argc, char *argv[]);
static void set_network_interface_status(int sock, int argc, char *argv[]);
static void get_network_interface_config(int sock, int argc, char *argv[]);
static void set_network_interface_config(int sock, int argc, char *argv[]);
static void get_dns_address(int sock, int argc, char *argv[]);
static void set_dns_address(int sock, int argc, char *argv[]);
static void is_interface_wireless(int sock, int argc, char *argv[]);
static void scan_wifi(int sock, int argc, char *argv[]);
static void connect_wifi(int sock, int argc, char *argv[]);
static void disconnect_wifi(int sock, int argc, char *argv[]);
static void get_wifi_quality(int sock, int argc, char *argv[]);
static void get_wifi_access_point_config(int sock, int argc, char *argv[]);
static void set_wifi_access_point_config(int sock, int argc, char *argv[]);

static void get_ip_and_netmask(int itf);
static void get_default_route(int itf);


// ---------------------- Private variables declarations.

static  network_interface_t *network_interfaces = NULL;
static  int nb_network_interfaces = 0;


// ---------------------- Public methods

int init_net_api(void)
{
	load_eris_network_configuration();
	write_system_network_configuration();

	if (register_api_command("list-network-interfaces", "lsni", "List available network interfaces.", list_network_interfaces))
		return -1;
	if (register_api_command("get-network-interface-status", "gnis", "Get status of network interface", get_network_interface_status))
		return -1;
	if (register_api_command("set-network-interface-status", "snis", "Set status of network interface", set_network_interface_status))
		return -1;
	if (register_api_command("get-network-interface-config", "gnic", "Get config of network interface", get_network_interface_config))
		return -1;
	if (register_api_command("set-network-interface-config", "snic", "Set config of network interface", set_network_interface_config))
		return -1;
	if (register_api_command("get-nameserver-address", "gdns", "Get the IP address of the Domain Name Server", get_dns_address))
		return -1;
	if (register_api_command("set-nameserver-address", "sdns", "Set the IP address of the Domain Name Server", set_dns_address))
		return -1;
	if (register_api_command("is-interface-wireless", "iiwi", "Return `yes` if the network interface is wireless", is_interface_wireless))
		return -1;
	if (register_api_command("scan-wifi", "scan", "Scan the SSID reachable on Wifi interface", scan_wifi))
		return -1;
	if (register_api_command("connect-wifi", "cnwf", "Connect to a Wifi access point", connect_wifi))
		return -1;
	if (register_api_command("disconnect-wifi", "dcwf", "Disconnect from the WIfi access point", disconnect_wifi))
		return -1;
	if (register_api_command("get-wifi-quality", "gwqy", "Get Wifi quality", get_wifi_quality))
		return -1;
	if (register_api_command("get-wifi-access-point", "gwap", "Get Wifi access point configuration", get_wifi_access_point_config))
		return -1;
	if (register_api_command("set-wifi-access-point", "swap", "Set Wifi access point configuration", set_wifi_access_point_config))
		return -1;

	return 0;
}


// ---------------------- Private methods


static void load_eris_network_configuration(void)
{
	FILE *fp;
	char line[1024];

	if (network_interfaces != NULL) {
		free(network_interfaces);
		network_interfaces = NULL;
		nb_network_interfaces = 0;
	}

	fp = fopen(ERIS_NETWORK_CONFIG_FILE, "r");
	if (fp == NULL)
		return;

	while (fgets(line, 1023, fp) != NULL) {
		// <interface> <atboot> <inet> <method> <ip addr> <netmask> <gateway>
		// <atboot> = { 'atboot', 'notatboot' }
		// <ip class> = { 'ipv4', 'ipv6' }
		// <method> = { 'static', 'dhcp' }

		// We load our own configuration file, we do not try to be very robust.

		int start;
		for (start = 0; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;

		// Interface name.

		int end;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[end])) ; end ++)
			;
		line[end] = '\0';

		int itf;
		for (itf = 0; itf < nb_network_interfaces; itf++)
			if (strcmp(&(line[start]), network_interfaces[itf].name) == 0)
				break;
		if (itf != nb_network_interfaces)
			continue;

		network_interface_t *newitf;
		newitf = realloc(network_interfaces, (nb_network_interfaces + 1) * sizeof (network_interface_t));
		if (newitf == NULL)
			break;
		network_interfaces = newitf;
		memset(&(network_interfaces[nb_network_interfaces]), 0, sizeof(network_interface_t));
		strncpy(network_interfaces[nb_network_interfaces].name, &(line[start]), INTERFACE_NAME_LENGTH - 1);
		network_interfaces[nb_network_interfaces].name[INTERFACE_NAME_LENGTH - 1] = '\0';
		nb_network_interfaces++;

		// At Boot?
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		network_interfaces[nb_network_interfaces - 1].at_boot = (strcmp(&(line[start]), "atboot") == 0);

		// ipv4/ipv6 ?
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		network_interfaces[nb_network_interfaces - 1].ipv6 = (strcmp(&(line[start]), "ipv6") == 0);

		// DHCP?
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		network_interfaces[nb_network_interfaces - 1].dhcp = (strcmp(&(line[start]), "dhcp") == 0);

		// IP address
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		strncpy(network_interfaces[nb_network_interfaces - 1].ip_address, &(line[start]), IP_ADDRESS_LENGTH - 1);

		// IP netmask
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		strncpy(network_interfaces[nb_network_interfaces - 1].ip_netmask, &(line[start]), IP_ADDRESS_LENGTH - 1);

		// IP gateway
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		strncpy(network_interfaces[nb_network_interfaces - 1].ip_gateway, &(line[start]), IP_ADDRESS_LENGTH - 1);
	}
	fclose(fp);
}



static void save_eris_network_configuration(void)
{
	FILE *fp;

	fp = fopen(ERIS_NETWORK_CONFIG_FILE, "w");
	if (fp == NULL)
		return;
	for (int i = 0; i < nb_network_interfaces; i++) {
		// <interface> <atboot> <inet> <method> <ip addr> <netmask> <gateway>
		// <atboot> = { 'atboot', 'notatboot' }
		// <ip class> = { 'ipv4', 'ipv6' }
		// <method> = { 'static', 'dhcp' }

		fprintf(fp, "%s %s %s %s %s %s %s\n",
			network_interfaces[i].name,
			network_interfaces[i].at_boot ? "atboot" : "notatboot",
			network_interfaces[i].ipv6    ? "ipv6"   : "ipv4",
			network_interfaces[i].dhcp    ? "dhcp"   : "static",
			network_interfaces[i].ip_address,
			network_interfaces[i].ip_netmask,
			network_interfaces[i].ip_gateway);
	}
	fclose(fp);
}



static void write_system_network_configuration(void)
{
	FILE *fp = fopen(SYSTEM_NETWORK_CONFIG_FILE, "w");
	if (fp == NULL)
		return;

	fprintf(fp, "auto lo\niface lo inet loopback\n\n");

	for (int i = 0; i < nb_network_interfaces; i++) {
		if (network_interfaces[i].at_boot) {
			fprintf(fp, "auto %s\n", network_interfaces[i].name);
		}
		if (network_interfaces[i].dhcp) {
			fprintf(fp, "iface %s %s dhcp\n",
				network_interfaces[i].name,
				network_interfaces[i].ipv6 ? "inet6"   : "inet");
		} else {
			fprintf(fp, "iface %s %s static\n",
				network_interfaces[i].name,
				network_interfaces[i].ipv6 ? "inet6"   : "inet");
			fprintf(fp, "\t address %s\n\t netmask %s\n\t gateway %s\n",
				network_interfaces[i].ip_address,
				network_interfaces[i].ip_netmask,
				network_interfaces[i].ip_gateway);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}



static void list_network_interfaces(int sock, int argc, char *argv[])
{
	DIR *dir;
	struct dirent *entry;

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	dir = opendir("/sys/class/net");
	if (dir != NULL) {
		while ((entry = readdir(dir)) != NULL) {

			if (entry->d_name[0] == '.')
				continue;

			char filename[512];
			snprintf(filename, 511, "/sys/class/net/%s/device", entry->d_name);
			if (access(filename, F_OK) != 0)
				continue;

			if (pos != 0)
				addsnprintf(&reply, &size, &pos, " ", entry->d_name);
			addsnprintf(&reply, &size, &pos, "%s", entry->d_name);
		}
		closedir(dir);
	}
	if (reply != NULL) {
		send_reply(sock, strlen(reply), reply);
		free(reply);
	} else {
		send_error(sock, ENODEV, "No network interface available.");
	}
}



static void get_network_interface_status(int sock, int argc, char *argv[])
{
	int itf;
	FILE *fp;
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	char filename[256];
	char line[1024];

	if (argc < 1) {
		send_error(sock, EINVAL, "get-network-interface-status command needs an argument.");
		return;
	}

	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, argv[0]) == 0)
			break;
	if (itf >= nb_network_interfaces) {
		send_error(sock, EINVAL, "Unknown interface name.");
		return;
	}

	snprintf(filename, 256, "/sys/class/net/%s/operstate", argv[0]);
	filename[255] = '\0';
	fp = fopen(filename, "r");
	if (fp == NULL) {
		send_error(sock, ENODEV, "The interface doesn't exist.");
		return;
	}
	if (fgets(line, 1024, fp) == NULL) {
		fclose(fp);
		send_error(sock, EINVAL, "The interface is invalid.");
		return;
	}
	fclose(fp);
	if (strncasecmp(line, "up", 2) == 0) {
		addsnprintf(&reply, &size, &pos, "up ");
		get_ip_and_netmask(itf);
		addsnprintf(&reply, &size, &pos, "%s %s ", network_interfaces[itf].ip_address, network_interfaces[itf].ip_netmask);
		get_default_route(itf);
		addsnprintf(&reply, &size, &pos, "%s ", network_interfaces[itf].ip_gateway);
	} else {
		addsnprintf(&reply, &size, &pos, "down ");
	}
	send_reply(sock, strlen(reply), reply);
}



static void set_network_interface_status(int sock, int argc, char *argv[])
{
	if (argc < 2) {
		send_error(sock, EINVAL, "set-network-interface needs two arguments.");
		return;
	}

	if ((strcmp(argv[1], "up") != 0) && (strcmp(argv[1], "down") != 0)) {
		send_error(sock, EINVAL, "The action is invalid.");
		return;
	}

	char command[256];

	snprintf(command, 255, "/sbin/if%s  %s",
		argv[1][0] == 'u' ? "up" : "down",
		argv[0]);
	command[255] = '\0';

	if (system(command) == 0) {
		send_reply(sock, 2, "ok");
	} else {
		send_error(sock, errno, "Failed to set interface status");
	}
}



static void get_network_interface_config(int sock, int argc, char *argv[])
{
	int itf;
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	if (argc < 1) {
		send_error(sock, EINVAL, "get-network-interface-config needs an argument.");
		return;
	}

	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, argv[0]) == 0)
			break;
	if (itf >= nb_network_interfaces) {
		send_error(sock, EINVAL, "Unknown interface name.");
		return;
	}

	addsnprintf(&reply, &size, &pos, "%s %s %s %s ",
		argv[0],
		network_interfaces[itf].ipv6 ? "ipv6" : "ipv4",
		network_interfaces[itf].at_boot ? "atboot" : "notatboot",
		network_interfaces[itf].dhcp  ? "dhcp" : "static");

	if (network_interfaces[itf].ip_address[0] == '\0')
		addsnprintf(&reply, &size, &pos, "0.0.0.0 ");
	else
		addsnprintf(&reply, &size, &pos, "%s ", network_interfaces[itf].ip_address);

	if (network_interfaces[itf].ip_netmask[0] == '\0')
		addsnprintf(&reply, &size, &pos, "0.0.0.0 ");
	else
		addsnprintf(&reply, &size, &pos, "%s ", network_interfaces[itf].ip_netmask);

	if (network_interfaces[itf].ip_gateway[0] == '\0')
		addsnprintf(&reply, &size, &pos, "0.0.0.0 ");
	else
		addsnprintf(&reply, &size, &pos, "%s ", network_interfaces[itf].ip_gateway);

	send_reply(sock, strlen(reply), reply);
	free(reply);
}



static void set_network_interface_config(int sock, int argc, char *argv[])
{
	int itf;

	if ((argc < 4)
	 || ((strcmp(argv[1], "atboot") != 0) && (strcmp(argv[1], "notatboot") != 0))
	 || ((strcmp(argv[2], "ipv4") != 0) && (strcmp(argv[2], "ipv6") != 0))
	 || ((strcmp(argv[3], "dhcp") != 0) && (strcmp(argv[3], "static") != 0))) {
		send_error(sock, EINVAL, "Invalid argument.");
		return;
	}

	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, argv[0]) == 0)
			break;
	if (itf >= nb_network_interfaces) {
		send_error(sock, EINVAL, "Unknown interface name.");
		return;
	}

	network_interfaces[itf].at_boot = (strcmp(argv[1], "atboot") == 0);
	network_interfaces[itf].ipv6 = (strcmp(argv[2], "ipv6") == 0);
	network_interfaces[itf].dhcp = (strcmp(argv[3], "dhcp") == 0);

	if (argc >= 4) {
		strncpy(network_interfaces[itf].ip_address, argv[4], IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_address[IP_ADDRESS_LENGTH - 1] = '\0';
	}

	if (argc >= 5) {
		strncpy(network_interfaces[itf].ip_netmask, argv[5], IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_netmask[IP_ADDRESS_LENGTH - 1] = '\0';
	}

	if (argc >= 6) {
		strncpy(network_interfaces[itf].ip_gateway, argv[6], IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_gateway[IP_ADDRESS_LENGTH - 1] = '\0';
	}

	save_eris_network_configuration();
	write_system_network_configuration();
	send_reply(sock, 2, "ok");
	load_eris_network_configuration();
}



static void get_dns_address(int sock, int argc, char *argv[])
{
	char ip[IP_ADDRESS_LENGTH];
	FILE *fp;
	char line[1024];

	ip[0] = '\0';
	fp = fopen("/etc/resolv.conf", "r");
	if (fp == NULL) {
		send_error(sock, errno, "Internal error.");
		return;
	}
	while (fgets(line, 1024, fp)  != NULL) {
		if (strncmp(line, "nameserver ", 11) == 0) {
			line[strlen(line) - 1] = '\0';
			strncpy(ip, &(line[11]), IP_ADDRESS_LENGTH);
			ip[IP_ADDRESS_LENGTH - 1] = '\0';
			break;
		}
	}
	fclose(fp);
	send_reply(sock, strlen(ip), ip);
}



static void set_dns_address(int sock, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 1) {
		send_error(sock, EINVAL, "set-dns-address needs an argument.");
		return;
	}
	for (int i = 0; argv[0][i] != '\0'; i++) {
		if ((! isxdigit(argv[0][i]))
		 && (argv[0][1] != '.')
		 && (argv[0][1] != ':')) {
			send_error(sock, errno, "Invalid IP address.");
			return;
		}
	}

	fp = fopen("/etc/resolv.conf", "w");
	if (fp == NULL)
		return;
	fprintf(fp, "nameserver %s\n", argv[0]);
	fclose(fp);

	send_reply(sock, 2, "ok");
}



static void is_interface_wireless(int sock, int argc, char *argv[])
{
	char pathname[256];

	if (argc < 1) {
		send_error(sock, EINVAL, "is-interface-wireless needs an argument.");
		return;
	}
	if (snprintf(pathname, 256, "/sys/class/net/%s", argv[0]) >= 256) {
		send_error(sock, EINVAL, "Wrong interface name.");
		return;
	}
	if (access(pathname, R_OK) != 0) {
		send_error(sock, ENODEV, "Wrong interface name.");
		return;
	}
	if (snprintf(pathname, 256, "/sys/class/net/%s/wireless", argv[0]) >= 256) {
		send_error(sock, EINVAL, "Wrong interface name.");
		return;
	}
	if (access(pathname, R_OK) == 0) {
		send_reply(sock, 3, "yes");
	} else {
		send_reply(sock, 2, "no");
	}
}


#define IW_SSID_PREFIX   "SSID: "

static void scan_wifi(int sock, int argc, char *argv[])
{
	char command[256];
	FILE *fp;
	char line[256];
	char ssid[256];

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	if (argc < 1) {
		send_error(sock, EINVAL, "scan-wifi needs an argument.");
		return;
	}

	snprintf(command, 256, "/sbin/ip link set dev %s up", argv[0]);
	command[255] = '\0';
	if (system(command) != 0) {
		send_error(sock, errno, "Unable to activate this interface.");
		return;
	}

	snprintf(command, 256, "/usr/sbin/iw dev %s scan", argv[0]);
	command[255] = '\0';

	fp = popen(command, "r");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to scan this interface.");
		return;
	}
	while (fgets(line, 256, fp) != NULL) {
		int start = 0;
		if (! isspace(line[start]))
			continue;
		start++;
		while (isspace(line[start]))
			start++;
		if (strncmp(&(line[start]), IW_SSID_PREFIX, sizeof(IW_SSID_PREFIX) - 1) != 0)
			continue;
		start += sizeof(IW_SSID_PREFIX) - 1;
		int i = 0;
		while ((i < 255) && (line[start + i] != '\n') && (line[start + i] != '\r') && (line[start + i] != '\0')) {
			ssid[i] = line[start + i];
			i++;
		}
		ssid[i] = '\0';
		addsnprintf(&reply, &size, &pos, "\n%s", ssid);
	}
	pclose(fp);
	if (reply != NULL)
		send_reply(sock, strlen(reply), reply);
	else
		send_error(sock, ENOENT, "No Wifi server available.");
	free(reply);
}



static void connect_wifi(int sock, int argc, char *argv[])
{
	FILE *fp = NULL;
	FILE *pp = NULL;
	char line[1024];

	if (argc < 3) {
		send_error(sock, EINVAL, "connect-wifi needs three arguments.");
		return;
	}

	fp = fopen("/etc/wpa_supplicant.conf", "w");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to open wpa_supplicant.conf.");
		return;
	}
	fprintf(fp, "# This file is automatically generated by Eris Linux API. DO NOT EDIT\n\n");
	fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\nctrl_interface_group=0\nupdate_config=1\n\n");

	snprintf(line, 1024, "/usr/sbin/wpa_passphrase '%s' %s | grep -v '#psk", argv[1], argv[2]);
	line[1023] = '\0';
	pp = popen(line, "r");
	if (pp == NULL) {
		send_error(sock, errno, "Unable to call wpa_passphrase.");
		return;
	}
	while (fgets(line, 1024, pp) != NULL)
		fputs(line, fp);
	pclose(pp);
	fclose(fp);

	snprintf(line, 1024, "wpa_supplicant -B -Dnl80211 -c/etc/wpa_supplicant.conf -i%s -P /var/run/wpa_supplicant.pid", argv[0]);
	line[1023] = '\0';
	system(line);

	send_reply(sock, 2, "Ok");
}



static void disconnect_wifi(int sock, int argc, char *argv[])
{
	FILE *fp;
	char line[256];
	unsigned long pid;

	fp = fopen("/var/run/wpa_supplicant.pid", "r");
	if (fp != NULL) {
		while(fgets(line, 256, fp) != NULL) {
			line[255] = '\0';
			if (sscanf(line, "%lu", &pid) == 1)
				kill(pid, SIGTERM);
		}
		fclose(fp);
	}
	send_reply(sock, 2, "Ok");
}



static void get_wifi_quality(int sock, int argc, char *argv[])
{
	FILE *fp;
	char line[256];

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;
	int found = 0;

	fp = fopen("/proc/net/wireless", "r");
	if (fp == NULL) {
		send_error(sock, errno, "Unable to read /proc/net/wireless.");
		return;
	}
	// /proc/net/wireless format:
	// Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
	//  face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
	//  wlo1: 0000   43.  -67.  -256        0      0      0     18     43        0

	// Read the two header lines.
	for (int i = 0; i < 2; i++) {
		if (fgets(line, 256, fp) == NULL) {
			fclose(fp);
			return;
		}
	}
	while(fgets(line, 256, fp) != NULL) {
		line[255] = '\0';
		int noise = 0;
		int level = 0;
		int link  = 0;

		int start, end;
		for (start = 0; isblank(line[start]); start++)
			;
		for (end = start; (isalnum(line[end])) || (line[end] == '-') || (line[end] == '_'); end ++)
			;
		if (strncmp(&line[start], argv[0], end - start) != 0)
			continue;
		// Skip the status fied.
		for (start = end + 1; isblank(line[start]); start++)
			;
		for (end = start; !isblank(line[end]); end++)
			;
		// Read the three values
		if (sscanf(&(line[end]), "%d. %d. %d", &link, &level, &noise) == 3) {
			addsnprintf(&reply, &size, &pos, "link=%d level=%d noise=%d", link, level, noise);
			send_reply(sock, strlen(reply), reply);
			found = 1;
			free(reply);
		}
		break;
	}
	fclose(fp);
	if (!found) {
		send_error(sock, ENOENT, "No Wifi quality available.");
	}
}



static void get_wifi_access_point_config(int sock, int argc, char *argv[])
{
}



static void set_wifi_access_point_config(int sock, int argc, char *argv[])
{
}



static void get_ip_and_netmask(int itf)
{
	struct ifaddrs *ifaddr;

	if (getifaddrs(&ifaddr) == 0) {

		struct ifaddrs *ifa;

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (strcmp(ifa->ifa_name, network_interfaces[itf].name) == 0)
				break;
		}
		if ((ifa != NULL) && (ifa->ifa_addr != NULL)) {
			int family = ifa->ifa_addr->sa_family;
			if (family == AF_INET || family == AF_INET6) {

				void *in_addr;
				in_addr = (family == AF_INET) ?
				(void*)&((struct sockaddr_in*)ifa->ifa_addr)->sin_addr :
				(void*)&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;

				inet_ntop(family, in_addr, network_interfaces[itf].ip_address, IP_ADDRESS_LENGTH);

				in_addr = (family == AF_INET) ?
				(void*)&((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr :
				(void*)&((struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr;

				inet_ntop(family, in_addr, network_interfaces[itf].ip_netmask, IP_ADDRESS_LENGTH);
			}
		}

		freeifaddrs(ifaddr);
	}
}



#define NL_BUFSIZE 8192

static void get_default_route(int itf)
{
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0)
		return;

	struct {
		struct nlmsghdr nlh;
		struct rtmsg rtm;
	} req;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.nlh.nlmsg_type  = RTM_GETROUTE;
	req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	req.rtm.rtm_family  = AF_UNSPEC;  // IPv4 + IPv6

	if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
		close(fd);
		return;
	}

	char buf[NL_BUFSIZE];
	ssize_t len;
	while ((len = recv(fd, buf, sizeof(buf), 0)) > 0) {
		for (struct nlmsghdr *nlh = (struct nlmsghdr*)buf; NLMSG_OK(nlh, (unsigned)len); nlh = NLMSG_NEXT(nlh, len)) {
			if ((nlh->nlmsg_type == NLMSG_DONE) || (nlh->nlmsg_type == NLMSG_ERROR)) {
				close(fd);
				return;
			}

			struct rtmsg *rtm = NLMSG_DATA(nlh);
			struct rtattr *rta = RTM_RTA(rtm);
			int rtl = RTM_PAYLOAD(nlh);

			if (rtm->rtm_table != RT_TABLE_MAIN)
				continue;

			if (rtm->rtm_dst_len != 0)
				continue;

			char gw[INET6_ADDRSTRLEN] = {0};
			char ifname[IF_NAMESIZE] = {0};

			for (; RTA_OK(rta, rtl); rta = RTA_NEXT(rta, rtl)) {
				switch (rta->rta_type) {
					case RTA_GATEWAY:
						if (rtm->rtm_family == AF_INET) {
							if (! network_interfaces[itf].ipv6)
								inet_ntop(AF_INET, RTA_DATA(rta), gw, sizeof(gw));
						} else if (rtm->rtm_family == AF_INET6) {
							if (network_interfaces[itf].ipv6)
								inet_ntop(AF_INET6, RTA_DATA(rta), gw, sizeof(gw));
						}
						break;
					case RTA_OIF:
						if_indextoname(*(int*)RTA_DATA(rta), ifname);
						break;
				}
			}

			if (strcmp(ifname, network_interfaces[itf].name) != 0)
				continue;
			if (strlen(gw) != 0) {
				strncpy(network_interfaces[itf].ip_gateway, gw, IP_ADDRESS_LENGTH);
				network_interfaces[itf].ip_gateway[IP_ADDRESS_LENGTH - 1] = '\0';
			}
		}
	}
	close(fd);
}

