/*
 *  ERIS LINUX API LIBRARY
 *
 *  (c) 2025-2026 Logilin
 *  All rights reserved
 */

#include <ctype.h>
#include <errno.h>
#include <curl/curl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "liberis.h"


// ---------------------- Private macros.

#define REPLY_STATUS_OK     0
#define REPLY_STATUS_ERROR -1
#define REST_API_PREFIX     "http://host.docker.internal:8080"


// ---------------------- Private type and structures.

typedef struct {
	char  *string;
	size_t size;
} easy_curl_memory_t;


// ---------------------- Private method declarations.


// ---------------------- Private methods

static void    create_easy_curl_key (void);
static CURL   *get_easy_curl_handle (void);
static size_t  easy_curl_callback   (void *content, size_t size, size_t count, void *user_ptr);
static int     perform_request      (const char *url, const char *method, char *reply, size_t size);


// ---------------------- Private variables.

static pthread_key_t  easy_curl_key;
static pthread_once_t easy_curl_once;


// ---------------------- Public methods

/******************************* GPIO ****************************************/

int eris_get_list_of_gpio(char *buffer, size_t size)
{
	if ((buffer == NULL) || (size == 0)) {
		errno = EINVAL;
		return -1;
	}
	return perform_request(REST_API_PREFIX "/api/gpio/list", "GET", buffer, size);
}



int eris_request_gpio_for_input(const char *name)
{
	char reply[128];
	char request[128];

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}

	snprintf(request, 127, "%s/api/gpio?name=%s&direction=in", REST_API_PREFIX, name);
	int err = perform_request(request, "GET", reply, 128);
	if ((err == 0) && (strcmp(reply, "Ok") == 0))
			return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 403)
		errno = EALREADY;
	else if (err == 404)
		errno = ENODEV;
	else if (err == 500)
		errno = EIO;
	return -1;
}



int eris_request_gpio_for_output(const char *name, int value)
{
	char reply[128];
	char request[128];
	int  ret = -1;

	if (name == NULL) {
		errno = EINVAL;
		return ret;
	}

	snprintf(request, 127, "%s/api/gpio?name=%s&direction=out&value=%d", REST_API_PREFIX, name, value);
	int err = perform_request(request, "GET", reply, 128);
	if ((err == 0) && (strcmp(reply, "Ok") == 0))
		return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 403)
		errno = EALREADY;
	else if (err == 404)
		errno = ENODEV;
	else if (err == 500)
		errno = EIO;
	return -1;
}



int eris_release_gpio(const char *name)
{
	char reply[128];
	char request[128];
	int  ret = -1;

	if (name == NULL) {
		errno = EINVAL;
		return ret;
	}

	snprintf(request, 127, "%s/api/gpio?name=%s", REST_API_PREFIX, name);
	int err = perform_request(request, "DELETE", reply, 128);
	if ((err == 0) && (strcmp(reply, "Ok") == 0))
		return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 404)
		errno = ENODEV;
	return -1;
}



int eris_read_gpio_value(const char *name)
{
	char reply[128];
	char request[128];

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 127, "%s/api/gpio/value?name=%s", REST_API_PREFIX, name);
	int err = perform_request(request, "GET", reply, 128);
	if (err == 0)
		return (reply[0] == '1');
	if (err == 400)
		errno = EINVAL;
	else if (err == 404)
		errno = ENODEV;
	return -1;
}



int eris_write_gpio_value(const char *name, int value)
{
	char reply[128];
	char request[128];
	int  ret = -1;

	if (name == NULL) {
		errno = EINVAL;
		return ret;
	}
	snprintf(request, 127, "%s/api/gpio/value?name=%s&value=%d", REST_API_PREFIX, name, value);
	int err = perform_request(request, "PUT", reply, 128);
	if ((err == 0) && (strcmp(reply, "Ok") == 0))
		return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 404)
		errno = ENODEV;
	return -1;
}



int eris_wait_gpio_edge(const char *name, const char *edge)
{
	char reply[128];
	char request[128];

	if ((name == NULL) || (edge == NULL)) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 127, "%s/api/gpio/edge?name=%s&type=%s", REST_API_PREFIX, name, edge);
	int err = perform_request(request, "GET", reply, 128);
	if ((err == 0) && (strcmp(reply, "Ok") == 0))
		return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 404)
		errno = ENODEV;
	else if (err == 500)
		errno = EIO;
	return -1;
}


/****************************** NETWORK **************************************/

int eris_get_list_of_network_interfaces(char *buffer, size_t size)
{
	if ((buffer == NULL) || (size == 0)) {
		errno = EINVAL;
		return -1;
	}
	return perform_request(REST_API_PREFIX "/api/network/interface/list", "GET", buffer, size);
}



int eris_get_network_interface_status(const char *interface, char *buffer, size_t size)
{
	char request[128];

	if ((interface == NULL) || (buffer == NULL) || (size == 0)) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 127, "%s/api/network/interface/status?name=%s", REST_API_PREFIX, interface);
	return perform_request(request, "GET", buffer, size);
}



int eris_set_network_interface_status(const char *interface, const char *status)
{
	char reply[128];
	char request[128];
	int  ret;

	if ((interface == NULL) || (status == NULL)) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 127, "%s/api/network/interface/status?name=%s&status=%s", REST_API_PREFIX, interface, status);
	if ((ret = perform_request(request, "PUT", reply, 128)) != 0)
		return ret;
	if (strcmp(reply, "Ok") == 0)
		return 0;
	return -1;
}



int eris_get_network_interface_config(const char *interface, char *buffer, size_t size)
{
	char request[127];

	snprintf(request, 127, "%s/api/network/interface/config?name=%s", REST_API_PREFIX, interface);
	return perform_request(request, "GET", buffer, size);
}



int eris_set_network_interface_config(const char *interface, const char *activate,
                                 const char *mode, const char *ip, 
                                 const char *address, const char *netmask,
                                 const char *gateway)
{
	char request[1024];

	if ((interface == NULL)
	 || (strlen(interface) > 32)
	 || (activate == NULL)
	 || ((strcmp(activate, "atboot") != 0) && (strcmp(activate, "ondemand") != 0))
	 || (mode == NULL)
	 || ((strcmp(mode, "dhcp") != 0) && (strcmp(mode, "static") != 0))) {
		errno = EINVAL;
		return -1;
	}
	if (ip == NULL)
		ip = "ipv4";
	if (address == NULL)
		address = "";
	if (netmask == NULL)
		netmask = "";
	if (gateway == NULL)
		gateway = "";

	if ((strlen(address) > 32)
	 || (strlen(netmask) > 32)
	 || (strlen(gateway) > 32)) {
		errno = EINVAL;
		return -1;
	}

	snprintf(request, 1024, "%s/api/network/interface/config?name=%s&activate=%s&mode=%s&ip=%s&address=%s&netmask=%s&gateway=%s",
		REST_API_PREFIX, interface, activate, mode, ip, address, netmask, gateway);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	else if (err == 404)
		errno = ENODEV;
	return -1;
}



int eris_get_nameserver_address(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/network/dns", "GET", buffer, size);
}



int eris_set_nameserver_address(const char *address)
{
	char request[512];
	
	if (address == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/network/dns?address=%s", REST_API_PREFIX, address);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	return -1;
}



int eris_is_network_interface_wireless(const char *interface)
{
	char request[512];

	if (interface == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/network/interface/wireless?name=%s", REST_API_PREFIX, interface);

	char reply[512];
	int err = perform_request(request, "GET", reply, 512);
	if (err == 0)
		return (strcmp(reply, "yes") == 0);
	if (err == 400)
		errno = EINVAL;
	return -1;
}



int eris_scan_wifi(const char *interface, char *buffer, size_t size)
{
	char request[512];

	if (interface == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/network/wifi?name=%s", REST_API_PREFIX, interface);

	int err = perform_request(request, "GET", buffer, size);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	if (err == 404)
		errno = ENODEV;
	return -1;
}



int eris_connect_wifi(const char *interface, const char *ssid, const char *password)
{
	char request[512];

	if ((interface == NULL)
	 || (strlen(interface) > 32)
	 || (ssid == NULL)
	 || (strlen(ssid) > 128)
	 || (password == NULL)
	 || (strlen(password) > 128)
	 || (snprintf(request, 512, "%s/api/network/wifi?name=%s&ssid=%s&pass=%s",
			REST_API_PREFIX, interface, ssid, password) >= 512)) {
		errno = EINVAL;
		return -1;
	}

	char reply[512];

	int err = perform_request(request, "POST", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	if (err == 500)
		errno = EIO;
	return -1;
}



int eris_disconnect_wifi(void)
{
	char reply[512];

	int err = perform_request(REST_API_PREFIX "/api/network/wifi", "DELETE", reply, 512);
	if (err == 0)
		return 0;
	errno = EINVAL;
	return -1;
}



int eris_get_wifi_quality(const char *interface, char *buffer, size_t size)
{
	char request[512];

	if ((interface == NULL)
	 || (strlen(interface) > 32)
	 || (snprintf(request, 512, "%s/api/network/wifi/quality?name=%s",
			REST_API_PREFIX, interface) >= 512)) {
		errno = EINVAL;
		return -1;
	}
	int err = perform_request(request, "GET", buffer, size);
	if (err == 0)
		return 0;
	errno = EINVAL;
	return -1;
}


/******************************* SBOM ****************************************/

int eris_get_list_of_packages(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/package/list", "GET", buffer, size);
}



int eris_get_package_version(const char *name, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/package/version?name=%s", REST_API_PREFIX, name);

	return perform_request(request, "GET", buffer, size);
}



int eris_get_package_licenses(const char *name, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/package/licenses?name=%s", REST_API_PREFIX, name);

	return perform_request(request, "GET", buffer, size);
}



int eris_get_list_of_licenses(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/license/list", "GET", buffer, size);
}



int eris_get_license_text(const char *name, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/license/text?name=%s", REST_API_PREFIX, name);

	return perform_request(request, "GET", buffer, size);
}


/****************************** SYSTEM ***************************************/

int eris_get_system_model(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/system/model", "GET", buffer, size);
}



int eris_get_system_type(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/system/type", "GET", buffer, size);
}



int eris_get_system_uuid(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/system/uuid", "GET", buffer, size);
}



int eris_get_system_version(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/system/version", "GET", buffer, size);
}



int eris_get_number_of_slots(void)
{
	char buffer[32];

	if (perform_request(REST_API_PREFIX "/api/container/count", "GET", buffer, 32) < 0)
		return 0;
	int count;
	if (sscanf(buffer, "%d", &count) != 1)
		return 0;
	if ((count < 0) || (count > 64)) // Arbitrary limit, to be refined.
		return 0;

	return count;
}



int eris_get_container_name(int slot, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/container/name?index=%d", REST_API_PREFIX, slot);

	return perform_request(request, "GET", buffer, size);

}



int eris_get_container_presence(int slot)
{
	char request[128];

	snprintf(request, 127, "%s/api/container/presence?index=%d", REST_API_PREFIX, slot);

	char reply[32];
	if (perform_request(request, "GET", reply, 32) != 0)
		return -1;
	return (strcmp(reply, "present") == 0);
}



int eris_get_container_status(int slot, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/container/status?index=%d", REST_API_PREFIX, slot);

	return perform_request(request, "GET", buffer, size);
}



int eris_get_container_version(int slot, char *buffer, size_t size)
{
	char request[128];

	snprintf(request, 127, "%s/api/container/version?index=%d", REST_API_PREFIX, slot);

	return perform_request(request, "GET", buffer, size);
}


/****************************** UPDATE ***************************************/

int eris_get_system_update_status(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/status", "GET", reply, 128) != 0) {
		errno = EINVAL;
		return -1;
	}
	return reply[0] - '0';
}



int eris_get_reboot_needed_flag(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/reboot/pending", "GET", reply, 128) != 0)
		return -1;

	if (reply[0] == 'y')
		return 1;

	if (reply[0] == 'n')
		return 0;

	errno = EINVAL;
	return -1;
}



int eris_set_reboot_needed_flag(int flag)
{
	char request[512];
	char reply[512];

	snprintf(request, 512, "%s/api/update/reboot/pending?reboot=%s", REST_API_PREFIX, flag ? "yes" : "no");
	return perform_request(request, "PUT", reply, 128);
}



int eris_get_server_contact_period(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/contact/period", "GET", reply, 128) != 0)
		return -1;

	return atoi(reply);
}


int eris_set_server_contact_period(int period)
{
	char request[512];
	char reply[512];

	snprintf(request, 512, "%s/api/update/contact/period?period=%d", REST_API_PREFIX, period);
	return perform_request(request, "PUT", reply, 128);
}



int eris_contact_server(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/contact/now", "POST", reply, 128) != 0) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}



int eris_get_automatic_reboot_flag(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/reboot/automatic", "GET", reply, 128) != 0)
		return -1;

	if ((reply[0] == 'Y') || (reply[0] == 'y'))
		return 1;

	if ((reply[0] == 'N')  || (reply[0] == 'n'))
		return 0;

	errno = EINVAL;
	return -1;
}


int eris_set_automatic_reboot_flag(int flag)
{
	char request[512];
	char reply[512];

	snprintf(request, 512, "%s/api/update/reboot/automatic?auto=%s", REST_API_PREFIX, flag ? "yes" : "no");
	return perform_request(request, "PUT", reply, 128);
}



int eris_get_container_update_policy(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/container/policy", "GET", reply, 128) != 0)
		return -1;

	if (reply[0] == 'i') // immediate
		return 1;

	if (reply[0] == 'a') // atreboot
		return 0;

	errno = EINVAL;
	return -1;
}


int eris_set_container_update_policy(int policy)
{
	char request[512];
	char reply[512];

	snprintf(request, 512, "%s/api/update/container/policy?policy=%s", REST_API_PREFIX, policy == 1 ? "immediate" : "atreboot");
	return perform_request(request, "PUT", reply, 128);
}


int eris_restore_factory_preset(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/factory", "POST", reply, 128) != 0) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}


int eris_rollback(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/rollback", "POST", reply, 128) != 0) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}


int eris_reboot(void)
{
	char reply[128];

	if (perform_request(REST_API_PREFIX "/api/update/reboot/now", "POST", reply, 128) != 0) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}


/***************************** TIME ******************************************/

int eris_get_ntp_server(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/ntp/server", "GET", buffer, size);
}



int eris_set_ntp_server(const char *server)
{
	char request[512];
	
	if (server == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/time/ntp/server?server=%s", REST_API_PREFIX, server);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	return -1;
}



int eris_get_ntp_enable(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/ntp", "GET", buffer, size);
}



int eris_set_ntp_enable(const char *enable)
{
	char request[512];
	
	if (enable == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/time/ntp?status=%s", REST_API_PREFIX, enable);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	return -1;
}



int eris_list_time_zones(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/zone/list", "GET", buffer, size);
}



int eris_get_time_zone(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/zone", "GET", buffer, size);
}



int eris_set_time_zone(const char *timezone)
{
	char request[512];
	
	if (timezone == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/time/zone?zone=%s", REST_API_PREFIX, timezone);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	return -1;
}



int eris_get_local_time(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/local", "GET", buffer, size);
}



int eris_get_system_time(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/time/system", "GET", buffer, size);
}



int eris_set_system_time(const char *time)
{
	char request[512];
	
	if (time == NULL) {
		errno = EINVAL;
		return -1;
	}
	snprintf(request, 512, "%s/api/time/system?time=%s", REST_API_PREFIX, time);

	char reply[512];
	int err = perform_request(request, "PUT", reply, 512);
	if (err == 0)
		return 0;
	if (err == 400)
		errno = EINVAL;
	return -1;
}


/*************************** WATCHDOG ****************************************/

int eris_feed_watchdog(void)
{
	char reply[512];

	return perform_request(REST_API_PREFIX "/api/watchdog", "POST", reply, 512);
}



int eris_disable_watchdog(void)
{
	char reply[512];

	return perform_request(REST_API_PREFIX "/api/watchdog", "DELETE", reply, 512);
}



int eris_get_watchdog_delay(void)
{
	char reply[512];

	if (perform_request(REST_API_PREFIX "/api/watchdog/delay", "GET", reply, 512) < 0)
		return -1;
	return atoi(reply);
}



int eris_set_watchdog_delay(int delay)
{
	char request[512];
	char reply[512];

	snprintf(request, 512, "%s/api/watchdog/delay?delay=%d", REST_API_PREFIX, delay);

	return perform_request(request, "PUT", reply, 512);
}



int eris_start_watchdog_feeder(void)
{
	char reply[512];

	return perform_request(REST_API_PREFIX "/api/watchdog/feeder", "POST", reply, 512);
}



int eris_stop_watchdog_feeder(void)
{
	char reply[512];

	return perform_request(REST_API_PREFIX "/api/watchdog/feeder", "DELETE", reply, 512);
}



int eris_watchdog_feeder_status(char *buffer, size_t size)
{
	return perform_request(REST_API_PREFIX "/api/watchdog/feeder", "GET", buffer, size);
}



// ---------------------- Private methods

static void create_easy_curl_key(void)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	(void) pthread_key_create(&easy_curl_key, NULL);
}



static CURL *get_easy_curl_handle(void)
{
	pthread_once(&easy_curl_once, create_easy_curl_key);
	CURL *handle = pthread_getspecific(easy_curl_key);
	if (handle == NULL) {
		handle = curl_easy_init();
		pthread_setspecific(easy_curl_key, handle);
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
	}
	return handle;
}



static size_t easy_curl_callback(void *content, size_t size, size_t count, void *user_ptr)
{
	size_t content_size = size * count;

	if (user_ptr == NULL)
		return content_size;

	easy_curl_memory_t *memory = (easy_curl_memory_t *)user_ptr;

	char *newptr = realloc(memory->string, memory->size + content_size + 1);
	if (newptr == NULL)
		return 0;
	memory->string = newptr;
	memcpy(&(memory->string[memory->size]), content, content_size);
	memory->size += content_size;
	memory->string[memory->size] = '\0';

	return content_size;
}



static int perform_request(const char *url, const char *method, char *reply, size_t size)
{
	CURL *handle = get_easy_curl_handle();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, method);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, easy_curl_callback);

	easy_curl_memory_t memory = { NULL, 0};
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &memory);

	int result = curl_easy_perform(handle);
	if (result != CURLE_OK)
		return -1;

	reply[0] = '\0';

	long http_code;
	curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);

	if (memory.size == 0)
		return 0;
	
	if (size >= memory.size + 1) { // memory.size doesn't count the final '\0'
		memcpy(reply, memory.string, memory.size);
		reply[memory.size] = '\0';
	} else {
		memcpy(reply, memory.string, size);
		reply[size - 1] = '\0';
	}
	free(memory.string);
	if (http_code != 200)
		return -http_code;
	return 0;
}



