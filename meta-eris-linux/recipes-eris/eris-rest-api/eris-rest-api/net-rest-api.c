/*
 *  ERIS LINUX REST API
 *
 *  (c) 2024-2026: Logilin
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

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "time-rest-api.h"


// ---------------------- Private macros declarations.

#define ERIS_NETWORK_CONFIG_FILE     "/etc/eris-linux/network"
#define SYSTEM_NETWORK_CONFIG_FILE   "/etc/network/interfaces"
#define INTERFACE_NAME_LENGTH 32
#define IP_ADDRESS_LENGTH   INET6_ADDRSTRLEN
#define EOL_CHAR(x) ((x == '\0') || (x == 0x23) || (x == '\n') || (x == '\r'))

// ---------------------- Private types definitions.

typedef struct {

	char  name[INTERFACE_NAME_LENGTH];
	int   at_boot;
	int   ipv6;
	int   dhcp;
	char  ip_address[IP_ADDRESS_LENGTH + 1];
	char  ip_netmask[IP_ADDRESS_LENGTH + 1 ];
	char  ip_gateway[IP_ADDRESS_LENGTH + 1];

} network_interface_t;


// ---------------------- Public method declarations.

static int load_eris_network_configuration    (void);
static int save_eris_network_configuration    (void);
static int write_system_network_configuration (void);

static enum MHD_Result list_network_interfaces      (struct MHD_Connection *connection);
static enum MHD_Result get_network_interface_status (struct MHD_Connection *connection);
static enum MHD_Result set_network_interface_status (struct MHD_Connection *connection);
static enum MHD_Result get_network_interface_config (struct MHD_Connection *connection);
static enum MHD_Result set_network_interface_config (struct MHD_Connection *connection);
static enum MHD_Result get_dns_address              (struct MHD_Connection *connection);
static enum MHD_Result set_dns_address              (struct MHD_Connection *connection);
static enum MHD_Result is_interface_wireless        (struct MHD_Connection *connection);
static enum MHD_Result scan_wifi                    (struct MHD_Connection *connection);
static enum MHD_Result connect_wifi                 (struct MHD_Connection *connection);
static enum MHD_Result disconnect_wifi              (struct MHD_Connection *connection);
static enum MHD_Result get_wifi_quality             (struct MHD_Connection *connection);
static enum MHD_Result get_wifi_access_point        (struct MHD_Connection *connection);
static enum MHD_Result set_wifi_access_point        (struct MHD_Connection *connection);

static int get_ip_and_netmask (int itf);
static int get_default_route  (int itf);


// ---------------------- Private variables declarations.

static  network_interface_t *network_interfaces = NULL;
static  int                  nb_network_interfaces = 0;


// ---------------------- Public methods

int init_net_rest_api(const char *app)
{
	if (load_eris_network_configuration() < 0)
		return -1;
	if (write_system_network_configuration() < 0)
		return -1;
	return 0;
}



enum MHD_Result net_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcasecmp(url, "/api/network/interface/list") == 0) {
		if (strcmp(method, "GET") == 0)
			return list_network_interfaces(connection);
	}
	if (strcasecmp(url, "/api/network/interface/status") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_network_interface_status(connection);
		if (strcmp(method, "PUT") == 0)
			return set_network_interface_status(connection);
	}
	if (strcasecmp(url, "/api/network/interface/config") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_network_interface_config(connection);
		if (strcmp(method, "PUT") == 0)
			return set_network_interface_config(connection);
	}
	if (strcasecmp(url, "/api/network/interface/wireless") == 0) {
		if (strcmp(method, "GET") == 0)
			return is_interface_wireless(connection);
	}
	if (strcasecmp(url, "/api/network/dns") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_dns_address(connection);
		if (strcmp(method, "PUT") == 0)
			return set_dns_address(connection);
	}
	if (strcasecmp(url, "/api/network/wifi") == 0) {
		if (strcmp(method, "GET") == 0)
			return scan_wifi(connection);
		if (strcmp(method, "POST") == 0)
			return connect_wifi(connection);
		if (strcmp(method, "DELETE") == 0)
			return disconnect_wifi(connection);
	}
	if (strcasecmp(url, "/api/network/wifi/quality") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_wifi_quality(connection);
	}
	if (strcasecmp(url, "/api/network/wifi/access-point") == 0) {
		if (strcmp(method, "GET") == 0)
			return get_wifi_access_point(connection);
		if (strcmp(method, "PUT") == 0)
			return set_wifi_access_point(connection);
	}

	return MHD_NO;
}


// ---------------------- Private methods

static int load_eris_network_configuration(void)
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
		return 0;

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
		network_interfaces[nb_network_interfaces - 1].ip_address[IP_ADDRESS_LENGTH - 1] = '\0';

		// IP netmask
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		strncpy(network_interfaces[nb_network_interfaces - 1].ip_netmask, &(line[start]), IP_ADDRESS_LENGTH - 1);
		network_interfaces[nb_network_interfaces - 1].ip_netmask[IP_ADDRESS_LENGTH - 1] = '\0';

		// IP gateway
		for (start = end + 1; isspace(line[start]); start ++)
			;
		if (EOL_CHAR(line[start]))
			continue;
		for (end = start; (! isspace(line[end])) && (! EOL_CHAR(line[start])); end ++)
			;
		line[end] = '\0';
		strncpy(network_interfaces[nb_network_interfaces - 1].ip_gateway, &(line[start]), IP_ADDRESS_LENGTH - 1);
		network_interfaces[nb_network_interfaces - 1].ip_gateway[IP_ADDRESS_LENGTH - 1] = '\0';
	}
	fclose(fp);
	return 0;
}



static int save_eris_network_configuration(void)
{
	FILE *fp;

	fp = fopen(ERIS_NETWORK_CONFIG_FILE, "w");
	if (fp == NULL) {
		fprintf(stderr, "Unable to open '%s'\n", ERIS_NETWORK_CONFIG_FILE);
		return -1;
	}
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
	return  0;
}



static int write_system_network_configuration(void)
{
	FILE *fp = fopen(SYSTEM_NETWORK_CONFIG_FILE, "w");
	if (fp == NULL) {
		fprintf(stderr, "Unable to open '%s'\n", SYSTEM_NETWORK_CONFIG_FILE);
		return -1;
	}

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
	return 0;
}



static enum MHD_Result list_network_interfaces(struct MHD_Connection *connection)
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
	if (reply == NULL)
		return send_rest_error(connection, "No network interface available.", 404);
	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result get_network_interface_status(struct MHD_Connection *connection)
{
	int itf;
	size_t size = 0;
	size_t pos  = 0;
	
	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");

	if (name == NULL)
		return send_rest_error(connection, "Missing interface name.", 400);

	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);

	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, name) == 0)
			break;
	if (itf >= nb_network_interfaces) {
		return send_rest_error(connection, "Unknown interface.", 404);
	}
	
	char filename[256];
	snprintf(filename, 256, "/sys/class/net/%s/operstate", name);
	filename[255] = '\0';
	
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	return send_rest_error(connection, "The interface doesn't exist anymore.", 404);
	
	char line[1024];
	if (fgets(line, 1024, fp) == NULL) {
		fclose(fp);
		return send_rest_error(connection, "The interface state is invalid.", 404);
	}
	fclose(fp);
	
	char *reply = NULL;
	if (strncasecmp(line, "up", 2) == 0) {
		if (get_ip_and_netmask(itf) < 0)
			return send_rest_error(connection, "Unable to obtain interface address.", 500);
		addsnprintf(&reply, &size, &pos, "up ");
		addsnprintf(&reply, &size, &pos, "%s %s ", network_interfaces[itf].ip_address, network_interfaces[itf].ip_netmask);
		if (get_default_route(itf) == 0)
			addsnprintf(&reply, &size, &pos, "%s ", network_interfaces[itf].ip_gateway);
	} else {
		addsnprintf(&reply, &size, &pos, "down ");
	}
	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result set_network_interface_status(struct MHD_Connection *connection)
{
	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");

	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);

	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);

	const char *status = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "status");
	if (status == NULL)
	        return send_rest_error(connection, "Missing interface status.", 400);

	if ((strcmp(status, "up") != 0) && (strcmp(status, "down") != 0))
	        return send_rest_error(connection, "Interface status is invalid.", 400);

	char command[256];

	snprintf(command, 255, "/sbin/if%s  %s",
		status[0] == 'u' ? "up" : "down",
		name);
	command[255] = '\0';

	if (system(command) == 0)
		return send_rest_response(connection, "Ok");
	return send_rest_error(connection, "Unable to set status.", 400);
}



static enum MHD_Result get_network_interface_config(struct MHD_Connection *connection)
{
	int itf;
	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);

	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, name) == 0)
			break;
	if (itf >= nb_network_interfaces)
	        return send_rest_error(connection, "Unknown interface.", 404);

	addsnprintf(&reply, &size, &pos, "%s %s ",
		name,
		network_interfaces[itf].at_boot ? "atboot" : "ondemand");
	
	if (network_interfaces[itf].dhcp) {
		addsnprintf(&reply, &size, &pos, "dhcp");
	} else {
		addsnprintf(&reply, &size, &pos, "static %s ",
			network_interfaces[itf].ipv6 ? "ipv6" : "ipv4");

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
	}

	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result set_network_interface_config(struct MHD_Connection *connection)
{
	int itf;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");

	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);

	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);
	
	for (itf = 0; itf < nb_network_interfaces; itf ++)
		if (strcmp(network_interfaces[itf].name, name) == 0)
			break;

	if (itf >= nb_network_interfaces)
		return send_rest_error(connection, "Unkown interface.", 404);

	const char *activate = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "activate");
	if (activate == NULL)
		return send_rest_error(connection, "Missing 'activate' parameter.", 400);
	if ((strcasecmp(activate, "atboot") != 0) && (strcasecmp(activate, "ondemand") != 0))
		return send_rest_error(connection, "Invalid 'activate' parameter (must be 'atboot' or 'ondemand').", 400);

	const char *mode = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "mode");
	if (mode == NULL)
		return send_rest_error(connection, "Missing 'mode' parameter.", 400);
	if ((strcmp(mode, "static") != 0) && (strcmp(mode, "dhcp") != 0))
		return send_rest_error(connection, "Invalid 'mode' parameter (must be 'dhcp' or 'static').", 400);

	network_interfaces[itf].at_boot = (strcasecmp(activate, "atboot") == 0);
	network_interfaces[itf].dhcp = (strcmp(mode, "dhcp") == 0);

	if (! network_interfaces[itf].dhcp) {

		const char *ip = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "ip");
		if (ip == NULL)
			return send_rest_error(connection, "Missing 'ip' parameter.", 400);
		if ((strcmp(ip, "ipv4") != 0) && (strcmp(ip, "ipv6") != 0))
			return send_rest_error(connection, "Invalid 'ip' parameter (must be 'ipv4' or 'ipv6').", 400);
		network_interfaces[itf].ipv6 = (strcmp(ip, "ipv6") == 0);

		const char *address = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "address");
		if (address == NULL)
			return send_rest_error(connection, "Missing 'address' parameter.", 400);
		strncpy(network_interfaces[itf].ip_address, address, IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_address[IP_ADDRESS_LENGTH - 1] = '\0';

		const char *netmask = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "netmask");
		if (netmask == NULL)
	        return send_rest_error(connection, "Missing 'netmask' parameter.", 400);
		strncpy(network_interfaces[itf].ip_netmask, netmask, IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_netmask[IP_ADDRESS_LENGTH - 1] = '\0';

		const char *gateway = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "gateway");
		if (gateway == NULL)
	        return send_rest_error(connection, "Missing 'gateway' parameter.", 400);
		strncpy(network_interfaces[itf].ip_gateway, gateway, IP_ADDRESS_LENGTH);
		network_interfaces[itf].ip_gateway[IP_ADDRESS_LENGTH - 1] = '\0';

	} else {
		network_interfaces[itf].ipv6 = 0;
		network_interfaces[itf].ip_address[0] = '\0';
		network_interfaces[itf].ip_netmask[0] = '\0';
		network_interfaces[itf].ip_gateway[0] = '\0';
	}
	save_eris_network_configuration();
	write_system_network_configuration();
	load_eris_network_configuration();

	return send_rest_response(connection, "Ok");
}



static enum MHD_Result get_dns_address(struct MHD_Connection *connection)
{
	char ip[IP_ADDRESS_LENGTH];
	FILE *fp;
	char line[1024];

	ip[0] = '\0';
	fp = fopen("/etc/resolv.conf", "r");
	if (fp == NULL) {
        return send_rest_error(connection, "Missing internal 'resolv.conf' file.", 500);
	}
	while (fgets(line, 1024, fp)  != NULL) {
		if (strncmp(line, "nameserver ", 11) == 0) {
			line[strlen(line) - 1] = '\0';
			strncpy(ip, &(line[11]), IP_ADDRESS_LENGTH);
			ip[IP_ADDRESS_LENGTH - 1] = '\0';
			break;
		}
	}
	return send_rest_response(connection, ip);
}



static enum MHD_Result set_dns_address(struct MHD_Connection *connection)
{
	FILE *fp;

	const char *address = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "address");
	if (address == NULL)
	        return send_rest_error(connection, "Missing 'address' parameter.", 400);

	for (int i = 0; address[i] != '\0'; i++) {
		if ((! isxdigit(address[i]))
		 && (address[i] != '.')
		 && (address[i] != ':'))
	        return send_rest_error(connection, "Invalid IP address.", 400);
	}

	fp = fopen("/etc/resolv.conf", "w");
	if (fp != NULL) {
		fprintf(fp, "nameserver %s\n", address);
		fclose(fp);
	}
	return send_rest_response(connection, "Ok");
}



static enum MHD_Result is_interface_wireless(struct MHD_Connection *connection)
{
	char pathname[256];

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);

	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);

	if (snprintf(pathname, 256, "/sys/class/net/%s", name) >= 256)
	        return send_rest_error(connection, "Invalid interface name.", 400);

	if (access(pathname, R_OK) != 0)
	        return send_rest_error(connection, "Unkown interface name.", 404);

	if (snprintf(pathname, 256, "/sys/class/net/%s/wireless", name) >= 256)
	        return send_rest_error(connection, "Invalid interface name.", 400);

	if (access(pathname, R_OK) == 0)
		return send_rest_response(connection, "yes");

	return send_rest_response(connection, "no");
}


#define IW_SSID_PREFIX   "SSID: "

static enum MHD_Result scan_wifi(struct MHD_Connection *connection)
{
	char command[256];
	FILE *fp;
	char line[256];
	char ssid[256];

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);

	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);

	snprintf(command, 256, "/sbin/ip link set dev %s up", name);
	command[255] = '\0';
	if (system(command) != 0)
	        return send_rest_error(connection, "Invalid interface name.", 400);

	snprintf(command, 256, "/usr/sbin/iw dev %s scan", name);
	command[255] = '\0';

	fp = popen(command, "r");
	if (fp == NULL)
	        return send_rest_error(connection, "Unable to scan this interface.", 400);

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
		addsnprintf(&reply, &size, &pos, "\r\n%s", ssid);
	}
	pclose(fp);
	if (reply == NULL)
	        return send_rest_error(connection, "No wifi access point available.", 404);

	int ret = send_rest_response(connection, reply);
	free(reply);
	return ret;
}



static enum MHD_Result connect_wifi(struct MHD_Connection *connection)
{
	FILE *fp = NULL;
	FILE *pp = NULL;
	char line[1024];

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);
			
	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);
	
	const char *ssid = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "ssid");
	if (ssid == NULL)
	        return send_rest_error(connection, "Missing 'ssid' param.", 400);

	const char *pass = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "pass");
	if (pass == NULL)
	        return send_rest_error(connection, "Missing 'pass' param.", 400);

	fp = fopen("/etc/wpa_supplicant.conf", "w");
	if (fp == NULL)
	        return send_rest_error(connection, "Unable to open 'wpa_supplicant.conf' file.", 500);

	fprintf(fp, "# This file is automatically generated by Eris Linux API. DO NOT EDIT\n\n");
	fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\nctrl_interface_group=0\nupdate_config=1\n\n");

	snprintf(line, 1024, "/usr/sbin/wpa_passphrase '%s' %s | grep -v '#psk", ssid, pass);
	line[1023] = '\0';
	pp = popen(line, "r");
	if (pp == NULL) {
		fclose(fp);
	        return send_rest_error(connection, "Unable to call 'wpa_passphrase'.", 500);
	}
	while (fgets(line, 1024, pp) != NULL)
		fputs(line, fp);
	pclose(pp);
	fclose(fp);

	snprintf(line, 1024, "wpa_supplicant -B -Dnl80211 -c/etc/wpa_supplicant.conf -i%s -P /var/run/wpa_supplicant.pid", name);
	line[1023] = '\0';
	system(line);

	return send_rest_response(connection, "Ok");
}


static enum MHD_Result disconnect_wifi(struct MHD_Connection *connection)
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
	return send_rest_response(connection, "Ok");
}



static enum MHD_Result get_wifi_quality(struct MHD_Connection *connection)
{
	FILE *fp;
	char line[256];

	char *reply = NULL;
	size_t size = 0;
	size_t pos  = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing interface name.", 400);
			
	for (int i = 0; name[i] != '\0'; i++)
		if ((name[i] == '/') || ( name == ';'))
			return send_rest_error(connection, "Invalid interface name.", 400);
		
	fp = fopen("/proc/net/wireless", "r");
	if (fp == NULL)
	        return send_rest_error(connection, "Unable to open 'wireless' file.", 500);

	// /proc/net/wireless format:
	// Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
	//  face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
	//  wlo1: 0000   43.  -67.  -256        0      0      0     18     43        0

	// Read the two header lines.
	for (int i = 0; i < 2; i++) {
		if (fgets(line, 256, fp) == NULL) {
			fclose(fp);
		        return send_rest_error(connection, "Invalid 'wireless' file.", 500);
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
		if (strncmp(&line[start], name, end - start) != 0)
			continue;
		// Skip the status fied.
		for (start = end + 1; isblank(line[start]); start++)
			;
		for (end = start; !isblank(line[end]); end++)
			;
		// Read the three values
		if (sscanf(&(line[end]), "%d. %d. %d", &link, &level, &noise) == 3) {
			addsnprintf(&reply, &size, &pos, "link=%d level=%d noise=%d", link, level, noise);
			int ret = send_rest_response(connection, reply);
			fclose(fp);
			free(reply);
			return ret;
		}
		break;
	}
	fclose(fp);
	return send_rest_error(connection, "No wifi quality available.", 400);
}



static enum MHD_Result get_wifi_access_point(struct MHD_Connection *connection)
{
}



static enum MHD_Result set_wifi_access_point(struct MHD_Connection *connection)
{
}



static int get_ip_and_netmask(int itf)
{
	int ret = -1;
	
	struct ifaddrs *ifaddr;

	if (getifaddrs(&ifaddr) == 0) {

		struct ifaddrs *ifa;
		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (strcmp(ifa->ifa_name, network_interfaces[itf].name) != 0)
				continue;

			if (ifa == NULL)
				continue;

			if (ifa->ifa_addr == NULL)
				continue;

			int family = ifa->ifa_addr->sa_family;
			if ((family != AF_INET) && (family != AF_INET6))
				continue;

			void *in_addr;
			in_addr = (family == AF_INET) ?
					(void*)&((struct sockaddr_in*)ifa->ifa_addr)->sin_addr :
					(void*)&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
			inet_ntop(family, in_addr, network_interfaces[itf].ip_address, IP_ADDRESS_LENGTH);

			if (ifa->ifa_netmask != NULL) {
				in_addr = (family == AF_INET) ?
					(void*)&((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr :
					(void*)&((struct sockaddr_in6*)ifa->ifa_netmask)->sin6_addr;
				inet_ntop(family, in_addr, network_interfaces[itf].ip_netmask, IP_ADDRESS_LENGTH);
			}

			ret = 0;
			break;
		}
		freeifaddrs(ifaddr);
	}
	return ret;
}



#define NL_BUFSIZE 8192

static int get_default_route(int itf)
{
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0)
		return -1;

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
		return -1;
	}

	char buf[NL_BUFSIZE];
	ssize_t len;
	while ((len = recv(fd, buf, sizeof(buf), 0)) > 0) {
		for (struct nlmsghdr *nlh = (struct nlmsghdr*)buf; NLMSG_OK(nlh, (unsigned)len); nlh = NLMSG_NEXT(nlh, len)) {
			if ((nlh->nlmsg_type == NLMSG_DONE) || (nlh->nlmsg_type == NLMSG_ERROR)) {
				close(fd);
				return -1;
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
	return 0;
}

