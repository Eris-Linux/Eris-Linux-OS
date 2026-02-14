/*
 *  ERIS LINUX API LIBRARY
 *
 *  (c) 2025 Logilin
 *  All rights reserved
 */

#ifndef LIBERIS_H
#define LIBERIS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/

/**
 *  @defgroup GPIOS
 *  @brief Functions to communicate using GPIO lines.
 */

/**
 * @defgroup GPIO_SETUP
 * @ingroup  GPIOS
 * @brief    Configuration of available GPIO.
 *
 */

/**
 * @brief Get a list of available GPIOs.
 *
 * @ingroup GPIO_CONFIG
 *
 * @param buffer    The buffer to fill with the list.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the provided buffer with the space-separated list of
 * available GPIOs.
 *
 * Example of use:
 *
 * @code
 * #include <liberis.h>
 * #include <stdio.h>
 *
 * int main(void)
 * {
 *   char list[512];
 *
 *   if (list_gpio_names(list, 512) != 0) {
 *      return EXIT_FAILURE;
 *   }
 *   printf("%s\n", list);
 *   return EXIT_SUCCESS;
 * }
 * @endcode
 */
int eris_get_list_of_gpio(char *buffer, size_t size);

/**
 * @brief Reserve a GPIO line and configure it for input.
 *
 * @ingroup GPIO_SETUP
 *
 * @param name    The name of the GPIO.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_request_gpio_for_input(const char *name);

/**
 * @brief Configure a GPIO line for output and set its initial value.
 *
 * @ingroup GPIO_SETUP
 *
 * @param name    The name of the GPIO.
 * @param value   The new value for the GPIO.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_request_gpio_for_output(const char *name, int value);

/**
 * @brief Release a GPIO line you don't use anymore.
 *
 * @ingroup GPIO_SETUP
 *
 * @param name    The name of the GPIO.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_release_gpio(const char *name);

/**
 * @defgroup GPIO_ACTION
 * @ingroup  GPIOS
 * @brief    Actions on reserved GPIO pins.
 *
 */

/**
 * @brief Read the value on a GPIO pin configured in input.
 *
 * @ingroup GPIO_ACTION
 *
 * @param name    The name of the GPIO.
 *
 * @return On success this function return 0 or 1 depending on the GPIO input.
 * On error -1 is returned and `errno` is set appropriately.
 *
 */
int eris_read_gpio_value(const char *name);

/**
 * @brief Write a value on a GPIO pin configured in output.
 *
 * @ingroup GPIO_ACTION
 *
 * @param name    The name of the GPIO.
 * @param value   The value (0 or 1) to write to the GPIO.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 */
int eris_write_gpio_value(const char *name, int value);

/**
 * @brief Wait for a certain change of value in input of a GPIO.
 *
 * @ingroup GPIO_ACTION
 *
 * @param name    The name of the GPIO.
 * @param edge    "rising" or "falling".
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 */
int eris_wait_gpio_edge(const char *name, const char *edge);


/*****************************************************************************/

/**
 *  @defgroup NETWORK
 *  @brief Functions to setup network.
 */

/**
 * @defgroup NETCONFIG
 * @ingroup  NETWORK
 * @brief    This is a group of functions to configure network interfaces.
 *
 */

/**
 * @brief Get a list of network interfaces.
 *
 * @ingroup NETCONFIG
 *
 * @param buffer    The buffer to fill with the list.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the provided buffer with the space-separated list of
 * available network interfaces.
 *
 * You can call @ref is_network_interface_wireless() to distinguish between
 * wired and wireless interfaces.
 *
 * Example of use:
 * @code
 * #include <liberis.h>
 * #include <stdio.h>
 *
 * int main(void)
 * {
 *   char list[512];
 *
 *   if (eris_list_network_interfaces(list, 512) != 0) {
 *      return EXIT_FAILURE;
 *   }
 *
 *   char *interface;
 *
 *   while ((interface = strtok(list, " ")) != NULL) {
 *     printf("- %s", interface);
 *
 *     if (eris_is_network_interface_wireless(interface))
 *       printf(" (wireless)");
 *
 *     printf("\n");
 *   }
 *   return EXIT_SUCCESS;
 * }
 * @endcode
 */
int eris_get_list_of_network_interfaces(char *buffer, size_t size);

/**
 * @brief Get the status of a network interface.
 *
 * @ingroup NETCONFIG
 *
 * @param interface The name of the interface considered.
 * @param buffer    The buffer to fill with the status.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the provided buffer with "up" or "down" depending on
 * the status of the interface.
 *
 * Example of use:
 * @code
 * int display_status(const char *interface)
 * {
 *   char buffer[8];
 *
 *   if (get_network_interface_status(interface, buffer, 8) < 0)
 *     return -1;
 *   printf("%s: %s\n", interface, buffer);
 *   return 0;
 * }
 * @endcode
 */
int eris_get_network_interface_status(const char *interface, char *buffer, size_t size);

/**
 * @brief Activate or disactivate a network interface.
 *
 * @ingroup NETCONFIG
 *
 * This function enables the interface if @p status is "up", or disables it
 * if @p status is "down". Any other string than "up" or "down" is invalid and
 * this function returns @ref ERR_INVALID_ARGUMENT.
 *
 * If the interface activation fails, the function will returns an error.
 *
 * @param interface  The name of the concerned interface.
 * @param status     The desired status ("up" or "down").
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_network_interface_status(const char *interface, const char *status);

/**
 * @brief Get the configuration of a network interface.
 *
 * @ingroup NETCONFIG
 *
 * @param interfacce  The name of the interface considered.
 * @param buffer      The buffer to fill with the configuration.
 * @param size        The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * The configuration stored in the buffer is a string with the following
 * space-separated fields:
 *
 * - the interface name
 * - `atboot` or `ondemand` depending whether the interface is automatically activated at boot
 * - `dhcp` or `static` depending on the way to obtain IP address.
 * - `4` or `6` depending on the IP version to use (only if the third field is `static`)
 * - the IP address (only if the third field is `static`)
 * - the subnet mask (only if the third field is `static`)
 * - the gateway address to access Internet (only if the third field is `static`)
 * - the address of the DNS name server (only if the third field is `static`)
 *
 * Example of use:
 */
int eris_get_network_interface_config(const char *interface, char *buffer, size_t size);

/**
 * @brief Set the configuration of a network interface.
 *
 * @ingroup NETCONFIG
 * @param interface The name of the interface considered.
 * @param activate  `atboot` if the interface has to be activated at boot, `ondemand` otherwise.
 * @param mode      `dhcp` if the address is automatically affected or `static` otherwise.
 * @param ip        `ipv4` or `ipv6` depending of the IP address version (ignored if 3th field is `dhcp`).
 * @param address   IP address of the interface (ignored if 3th field is `dhcp`)
 * @param netmask   IP mask of the subnet (ignored if 3th field is `dhcp`)
 * @param gateway   IP address of the gateway (ignored if 3th field is `dhcp`)
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * Example of use:
 * @code
 *   eris_set_network_interface_config("eth0", "atboot", "static", "ipv4", "192.168.1.1", "255.255.255.0", "192.168.1.254");
 *   eris_set_network_interface_config("eth1", "ondemand", "dhcp", NULL, NULL, NULL, NULL);
 * @endcode
 */
int eris_set_network_interface_config(const char *interface, const char *autoconf,
                                 const char *mode, const char *ip, 
                                 const char *address, const char *netmask,
                                 const char *gateway);

/**
 * @brief Get the IP address of the Domain Name Server.
 *
 * @ingroup NETCONFIG
 * @param buffer      The buffer to fill with the address.
 * @param size        The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 *   char buffer[128];
 *   if (eris_get_nameserver_address(buffer, 128) < 0)
 *     return -1;
 */
int eris_get_nameserver_address(char *buffer, size_t size);

/**
 * @brief Set the IP address of the Domain Name Server.
 *
 * @ingroup NETCONFIG
 * @param address     The IP address of the DNS
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * Example of use:
 * @code
 *   eris_set_nameserver_address("8.8.4.4");
 * @endcode
 */
int eris_set_nameserver_address(const char *address);


/**
 * @defgroup WIFI
 * @ingroup  NETWORK
 * @brief    This is a group of functions to scan and connect to a wifi server.
 *
 */

/**
 * @brief Indicate whether a network interface is wireless or not.
 *
 * @ingroup WIFI
 *
 * @param interface  the name of the interface.
 *
 * @return 1 if the interface is wireless, 0 if it is not, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * See @ref list_network_interfaces() for an example of use.
 */
int eris_is_network_interface_wireless(const char *interface);

/**
 * @brief Scan the available Wifi access points.
 *
 * @ingroup WIFI
 *
 * @param interface  The name of the interface.
 * @param buffer     The buffer to fill with SSID of access points.
 * @param size       The maximum size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the provided buffer with the line-separated list of
 * SSID of available Wifi access points.
 *
 * We choose to use `\n` as list spearator because SSIDs may contain most of
 * the available characters, including spaces.
 *
 */
int eris_scan_wifi(const char *interface, char *buffer, size_t size);

/**
 * @brief Connect to a Wifi access point.
 *
 * @ingroup WIFI
 *
 * @param interface  The name of the interface.
 * @param ssid       The identifier of the access point.
 * @param password   The passphrase to use to connect.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_connect_wifi(const char *interface, const char *ssid, const char *password);

/**
 * @brief Disconnect to any Wifi access points.
 *
 * @ingroup WIFI
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_disconnect_wifi(void);

/**
 * @brief Get the Wifi connection quality.
 *
 * @ingroup WIFI
 *
 * @param interface  The name of the interface.
 * @param buffer     The buffer to fill with Wifi quality.
 * @param size       The maximum size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the a line containing 3 fields.
 * For example:
 *
 * @code
 *   link=70 level=-37 noise=-256
 * @endcode
 *
 * The values are:
 * - link quality (percent)
 * - signal level in dBm
 * - noise level in dBm
 *
 */
int eris_get_wifi_quality(const char *interface, char *buffer, size_t size);


/*****************************************************************************/

/**
 *  @defgroup SBOM
 *  @brief Functions to access Software Bill of Materials.
 */

/**
 * @defgroup SBOM_PACKAGES
 * @ingroup  SBOM
 * @brief    This is a group of functions to get information about packages.
 *
 */

/**
 * @brief Get the list of Eris Linux installed packages.
 *
 * @ingroup SBOM_PACKAGES
 *
 * @param buffer    The buffer to fill with the list of packages.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with a space-separated list of packages
 * embedded in Eris Linux system (but not in containers).
 *
 * The system packages are installed during the production of the system
 * and cannot be modified further.
 *
 * We suggest using a 4Kb sized buffer to ensure the list fits.
 *
 */
int eris_get_list_of_packages(char *buffer, size_t size);

/**
 * @brief Get the version of a packages.
 *
 * @ingroup SBOM_PACKAGES
 *
 * @param name      The name of the package extracted from the result of `eris_list_packages`.
 * @param buffer    The buffer to fill with the version of the package.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the version of the package used to
 * build Eris Linux system.
 *
 * A small buffer of 32 bytes, for example, will generally be sufficient.
 *
 */
int eris_get_package_version(const char *name, char *buffer, size_t size);

/**
 * @brief Get the licenses concerning a package.
 *
 * @ingroup SBOM_PACKAGES
 *
 * @param name      The name of the package extracted from the result of `eris_list_packages`.
 * @param buffer    The buffer to fill with the details of the licenses.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * Some packages contain files coming from multiple sources. These files may
 * be released under different licenses. Sometimes, the author of a package
 * allows the user to choose from several licenses for its usage.
 *
 * This function populates the buffer with the logical combination of
 * applicable licenses. Most of the time only one license name will appear.
 *
 */
int eris_get_package_licenses(const char *name, char *buffer, size_t size);

/**
 * @defgroup SBOM_LICENSES
 * @ingroup  SBOM
 * @brief    This is a group of functions to get information about licenses.
 *
 */
/**
 * @brief Get the list of the licenses used by installed packages.
 *
 * @ingroup SBOM_LICENSES
 *
 * @param buffer    The buffer to fill with the list of licenses.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with a space-separated list of the
 * licenses used by the packages embedde din Eris Linux system (not in the
 * containers).
 *
 * We suggest using a 1Kb sized buffer to ensure the list fits.
 *
 */
int eris_get_list_of_licenses(char *buffer, size_t size);

/**
 * @brief Get the text of a license.
 *
 * @ingroup SBOM_LICENSES
 *
 * @param name      The name of the license.
 * @param buffer    The buffer to fill with the text of the license.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the text of the given license.
 *
 * Some license texts extend to 350 Kb. Therefore we recommend providing a
 * quite large buffer (512Kb for example).
 */
int eris_get_license_text(const char *name, char *buffer, size_t size);

/*****************************************************************************/

/**
 *  @defgroup SYSTEM
 *  @brief Functions to access system identifiers
 */

/**
 * @defgroup SYSTEM_INFO
 * @ingroup  SYSTEM
 * @brief    This is a group of functions to get information about the system.
 *
 */

/**
 * @brief Read Eris Linux system image type.
 *
 * @ingroup SYSTEM_INFO
 *
 * @param buffer    The buffer to fill with the system image type.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the "type" of the image installed.
 *
 * Some type of images: "Base"", "Graphical", "Realtime",
 * "Realtime-Graphical". And there are specific image types to handle
 * customers-specific boards.
 *
 * The type of the image is written during image production and can not be
 * modified.
 *
 */
int eris_get_system_type(char *buffer, size_t size);

/**
 * @brief Read Eris Linux system model
 *
 * @ingroup SYSTEM_INFO
 *
 * @param buffer    The buffer to fill with the device model.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with model name of the device hosting the
 * API server.
 *
 * Some example of device models: "Raspberry-Pi-4", "Qemu-x86",
 * "Beagle-Bone-Black"...
 *
 * The device model is written during image production and can't be modified.
 *
 */
int eris_get_system_model(char *buffer, size_t size);


/**
 * @brief Get the Universally Unique Identifier (UUID) of the machine.
 *
 * @ingroup SYSTEM_INFO
 *
 * This function fills the provided buffer with the UUID of the machine.
 *
 * A UUID (Universally Unique Identifier) is a 128-bits number usually
 * represented by a string of the form `dddddddd-dddd-dddd-dddd-dddddddddddd`
 * (`d` being an hexadecimal digit).
 *
 * The UUID is randomly generated but the probablity to generate two identical
 * UUID is extremly low.
 * 
 * @param buffer    the buffer to fill with the UUID string.
 * @param size      the size of the buffer.
 *
 * The buffer must be at least 37 bytes long (36 chars + NUL char).
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
 int eris_get_system_uuid(char *buffer, size_t size);
 
  
 /**
 * @brief Read Eris Linux system version
 *
 * @ingroup SYSTEM_INFO
 *
 * @param buffer    The buffer to fill with the version number.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the version of the Eris Linux system
 * image.
 *
 * The version number is modified at each system update.
 *
 * The version number is written during image production and can not be
 * modified.
 *
 */ 
int eris_get_system_version(char *buffer, size_t size);


/**
 * @brief Return the number of slots for container in the system.
 *
 * @ingroup SYSTEM_INFO
 *
 * @return The number of slots in the system.
 *
 * @details
 *
 * A slot, numbered from 0 to number-of-slots - 1, can receive a container.
 *
 */ 
int eris_get_number_of_slots(void);


/**
 * @brief Read the name of the container in a given slot.
 *
 * @ingroup SYSTEM_INFO
 *
 * @param slot      The number of the slot hosting the container.
 * @param buffer    The buffer to fill with the name.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the name of the container currently
 * hosted in the indicated slot. If the slot doesn't host any container, the
 * buffer will be empty.
 *
 * The name of the container is remotely set by the device manager and can not
 * be modified from the target.
 *
 */ 
int eris_get_container_name(int slot, char *buffer, size_t size);


/**
 * @brief Check if a container is present in a given slot.
 *
 * @ingroup SYSTEM_INFO
 *
 * @param slot      The number of the slot hosting the container.
 *
 * @return 1 if a container is present in the slot, 0 if the slot is empty, -1
 *  on error and errno is set appropriately.
 *
 */ 
int eris_get_container_presence(int slot);


/**
 * @brief Get the status of the container in a given slot.
 *
 * @ingroup SYSTEM_INFO
 *
 * @param slot      The number of the slot hosting the container.
 * @param buffer    The buffer to fill with the status.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the status of the container
 * currently hosted in the indicated slot. If the slot doesn't host any
 * container, the buffer will be empty.
 *
 * The version status is one of the following strings:
 * "running" or "stopped".
 *
 */ 
int eris_get_container_status(int container, char *buffer, size_t size);


/**
 * @brief Read the version number  of the container in a given slot.
 *
 * @ingroup SYSTEM_INFO
 *
 * @param slot      The number of the slot hosting the container.
 * @param buffer    The buffer to fill with the version number.
 * @param size      The size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * This function fills the buffer with the version number of the container
 * currently hosted in the indicated slot. If the slot doesn't host any
 * container, the buffer will be empty.
 *
 * The version of the container is remotely set by the device manager and can
 * not be modified from the target.
 *
 */ 
int eris_get_container_version(int container, char *buffer, size_t size);


/*****************************************************************************/
/**
 *  @defgroup TIME
 *  @brief Time and timezone related functions
 */

/**
 * @defgroup NTP
 * @ingroup  TIME
 * @brief    Functions to configure an NTP server.

 * A NTP (Network Time Protocol) server is a machine with a precise time
 * keeping source (GPS, atomic clock, etc.). This server  is configured to
 * answer other machine's requests to setup their own system clock.
 *
 * There are many NTP servers on Internet. The one used by default by Eris
 * is `pool.ntp.org`, but you can prefer your own server (maybe on a local
 * network, or totaly disable the NTP use.
 */

/**
 * @brief Get the current NTP server used by the machine.
 *
 * @ingroup NTP
 *
 * This function fills the provided buffer with the current NTP server.
 *
 * @param buffer    the buffer to fill with the server adress or name.
 * @param size      the size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_get_ntp_server(char *buffer, size_t size);


/**
 * @brief Set the NTP server to use by the machine.
 *
 * @ingroup NTP
 *
 * This function stores the given NTP server in Eris system.
 *
 * @param server    the hostname or the IP address of the NTP server.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_ntp_server(const char *server);


/**
 * @brief Get the current NTP status.
 *
 * @ingroup NTP
 *
 * This functions fills the provided buffer with the string `yes` or `no`
 * depending whether the NTP usage is currently enabled or disabled.
 *
 * @param buffer    the buffer to fill with the NTP status.
 * @param size      the size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_get_ntp_enable(char *buffer, size_t size);


/**
 * @brief Set the NTP status.
 *
 * @ingroup NTP
 *
 * This function stores the NTP status (`yes` or `no`) to enable or disable
 * NTP usage.
 *
 * @param status   `yes` or `no`.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_ntp_enable(const char *status);


/**
 * @defgroup TIMEZONE
 * @ingroup  TIME
 * @brief    Functions to list timezones and configure system timezone.
 *
 * The system clock is configured (using NTP for example) and maintained in
 * UTC (Universal Time Coordinate) timezone.
 *
 * The timezone corresponding to the system geographical position is
 * configured with the functions of this group.
 */

/**
 * @brief List the timezones known by the system.
 *
 * @ingroup TIMEZONE
 *
 * This function fills the provided buffer with the list of all timezones
 * available.
 *
 * @param buffer    the buffer to fill with the list of timezone.
 * @param size      the size of the buffer.
 *
 * The recommanded size of the buffer is about 10kbytes.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_list_time_zones(char *buffer, size_t size);


/**
 * @brief Get the current timezone of the system.
 *
 * @ingroup TIMEZONE
 *
 * This function fills the provided buffer with the name of the timezone used
 * to display local time.
 *
 * @param buffer    the buffer to fill with the timezone name
 * @param size      the size of the buffer.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_get_time_zone(char *buffer, size_t size);


/**
 * @brief Set the system timezone
 *
 * @ingroup TIMEZONE
 *
 * This function stores the name of the timezone to use when computing local
 * time.
 *
 * The name must be in the list returned by `eris_list_time_zones()`.
 *
 * @param timezone   the name of the timezone.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_time_zone(const char *timezone);


/**
 * @defgroup LOCAL_AND_SYSTEM_TIME
 * @ingroup  TIME
 * @brief    Functions to access local time and system time.
 *
 * The system clock is configured (using NTP for example) and maintained in
 * UTC (Universal Time Coordinate) timezone.
 *
 * The local time is computed using the timezone of the system.
 *
 * The preferred way to keep the system clock up to date is to use Network
 * Time Protocol (NTP), but in case this is not possible, you can set the
 * systme clock from a user input.
 */

/**
 * @brief Get the local time
 *
 * @ingroup LOCAL_AND_SYSTEM_TIME
 *
 * This function fills the provided buffer with the local time.
 *
 * The format for local time is `YYYY-MM-DD hh:mm:ss:uuuuuu`
 * (`uuuuuu` represents microseconds).
 *
 * @param buffer    the buffer to fill.
 * @param size      the size of the buffer.
 *
 * The buffer zie must be at least 27 bytes.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_get_local_time(char *buffer, size_t size);


/**
 * @brief Get the system time
 *
 * @ingroup LOCAL_AND_SYSTEM_TIME
 *
 * This function fills the provided buffer with the system time (i.e. the
 * current time in UTC timezone).
 *
 * The format for system time is `YYYY-MM-DD hh:mm:ss:uuuuuu`
 * (`uuuuuu` represents microseconds).
 *
 * @param buffer    the buffer to fill.
 * @param size      the size of the buffer.
 *
 * The buffer zie must be at least 27 bytes.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_get_system_time(char *buffer, size_t size);


/**
 * @brief Set the system time
 *
 * @ingroup LOCAL_AND_SYSTEM_TIME
 *
 * This function sets the system clock with the (UTC timezone) time provided
 * in the buffer.
 *
 * The format for system time may be:
 *
 * - `YYYY-MM-DDThh:mm:ss`
 * - `YYYY-MM-DD hh:mm:ss`
 * - `YYYY/MM/DD hh:mm:ss`
 * - `YYYY:MM:DD:hh:mm:ss`
 *
 * @param time    the system time.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_system_time(const char *time);


/*****************************************************************************/

/**
 * @defgroup SYSTEM_UPDATE
 * @ingroup  SYSTEM
 * @brief    This is a group of functions to update the system content.
 *
 */

/**
 * @brief  Get the current status of system update.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @retval 1    System is OK, no update is pending.
 * @retval 2    System update install in progress.
 * @retval 3    System update install Ok. Ready to reboot.
 * @retval 4    System update install failed.
 * @retval 5    System reboot in progress.
 * @retval -1   An error has occured during the request, errno is set appropriately.
 *
 * @details
 *
 * This function shows if the device's system is up to date, if an update is
 * pending, or if an update is ready.
 *
 *
 */
int eris_get_system_update_status(void);


/**
 * @brief  Get the state of the "Reboot Needed" flag.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @retval 0    No reboot is programmed.
 * @retval 1    A reboot will occur as soon as possible.
 * @retval -1   An error has occured during the request, errno is set appropriately.
 *
 * @details
 *
 * This flag may be set by the update system or any other custom application
 * (see `eris_set_reboot_needed_flag`) to indicate that a reboot is needed to
 * complete an update.
 *
 * Please avoid starting time-consuming tasks or large data writing programs
 * when the flag is set.
 */
int eris_get_reboot_needed_flag(void);


/**
 * @brief  Set the state of the "Reboot Needed" flag.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @param flag   0 (reset) or 1 (set) the flag asking for a reboot.
 *
 * @return 0 on success and  -1 on error and errno is set appropriately.
 *
 */
int eris_set_reboot_needed_flag(int flag);


/**
 * @brief  Get the period in seconds that the device waits between contacts with the server.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @returns the period in seconds, or -1 on error and errno is set appropriately.
 *
 * @details
 *
 * If the period is 0, the device will contact the server only when explicitly
 * asked by `eris_contact_server()` method (see below).
 */
int eris_get_server_contact_period(void);


/**
 * @brief  Set the period in seconds that the device waits between contacts with the server.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @param the period in seconds.
 *
 * @returns 0 on success or -1 on error and errno is set appropriately.
 *
 * @details
 *
 * If the period is 0, the device will contact the server only when explicitly
 * asker by `eris_contact_server` (see below).
 */
int eris_set_server_contact_period(int period);


/**
 * @brief  Request a contact with the server.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @returns 0 on success or -1 on error and errno is set appropriately.
 *
 * @details
 *
 * Asking to explicitly contact the server is especially usefull when the
 * period of contact is zero. Otherwise it may be needed to report a new
 * condition on a container for example.
 */
int eris_contact_server(void);


/**
 * @brief  Read the Automatic-Reboot flag
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @retval 1  Automatic reboot is allowed.
 * @retval 0  Reboot is differed until explicit request.
 * @retval -1   An error has occured during the request, errno is set appropriately.
 *
 * @details
 *
 * When an update is available for the Eris Linux system (not the container's
 * content), it will be automatically downloaded as soon as it is detected.
 * Once installation is complete the system must be rebooted in order to run
 * the new system version. The Automatic Reboot flag determines whether the
 * system reboots immediately after the update is installed or if it waits for
 * a reboot request from the server or a container.
 *
 */
int eris_get_automatic_reboot_flag(void);


/**
 * @brief  Write the Automatic-Reboot flag
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @param autoreboot: 1 (automatic reboot is allowed), 0 (reboot is differed until explicit request).
 *
 * @returns 0 on success or -1 on error and errno is set appropriately.
 *
 * @details
 *
 * When an update is available for the Eris Linux system (not the container's
 * content), it will be automatically downloaded as soon as it is detected.
 * Once installation is complete the system must be rebooted in order to run
 * the new system version. The Automatic Reboot flag determines whether the
 * system reboots immediately after the update is installed or if it waits for
 * a reboot request from the server or a container.
 * 
 */
int eris_set_automatic_reboot_flag(int autoreboot);


/**
 * @brief  Read the Container Update Policy flag
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @retval 0  Update of container content only after system reboot.
 * @retval 1  Immediate restart of container after content update.
 * @retval -1 An error has occured during the request, errno is set appropriately.
 *
 * @details
 *
 * When an update is available for the content of a container, the new content
 * can be installed immediately and the container can be restarted as soon as
 * possible. Alternatively, we can keep the current container running until
 * the system is rebooted.
 *
 */
int eris_get_container_update_policy(void);


/**
 * @brief  Write the Container-Update-Policy flag.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @param policy: 1 (immediate update and restart), 0 (update at system reboot).
 *
 * @returns 0 on success or -1 on error and errno is set appropriately.
 *
 * @details
 *
 * When an update is available for the content of a container, the new content
 * can be installed immediately and the container can be restarted as soon as
 * possible. Alternatively, we can keep the current container running until
 * the system is rebooted.
 */
int eris_set_container_update_policy(int policy);


/**
 * @brief  Force the system to return to a factory preset state.
 *
 * @ingroup SYSTEM_UPDATE
 *
 *
 * @returns only on error.
 *
 * @details
 *
 * Sometimes, it is necessary to reset the entire system to its factory
 * default settings. For example, this function may be used to erase all
 * personal pieces of information before reselling the device.
 *
 */
int eris_restore_factory_preset(void);


/**
 * @brief  Roolback to the previous system version
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 */
int eris_rollback(void);


/**
 * @brief  Reboot the system.
 *
 * @ingroup SYSTEM_UPDATE
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 *
 * @details
 *
 * The system will reboot as soon as possible.
 *
 * If some application is currently writing data on the flash memory, the data
 * will probably be lost, and (in rare cases) the whole data partition may
 * suffer from an inconsistent state when restarting.
 *
 */
int eris_reboot(void);


/*****************************************************************************/

/**
 *  @defgroup WATCHDOG
 *  @brief System watchdog related methods
 */

/**
 * @defgroup WATCHDOG_SETUP
 * @ingroup  WATCHDOG
 * @brief    Functions to configure and feed the system watchdog.
 *
 * A watchdog is a physical (if possible) or logical (otherwise) device that
 * once initialized must be refreshed regularly or it will automatically
 * reboot the system.
 *
 * This is very useful or embedded systems to avoid devices that stay in an
 * unusable state after a crash.
 *
 * The functions of this group allow you to read or set the delay (in seconds)
 * before reboot, and to refresh the watchdog thus resetting the reboot timer.
 */

/**
 * @brief Refresh the watchdog timer
 *
 * @ingroup WATCHDOG_SETUP
 *
 * This function reset the watchdog timer allowing *delay* seconds before
 * rebooting the system (*delay* beeing the number of seconds set by
 * `eris_set_watchdog_delay()`.
 *
 * Some people use the expression "kick the watchdog", but we like dogs
 * (and other animals) and refuse to use it.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_feed_watchdog(void);



/**
 * @brief Disable the watchdog timer
 *
 * @ingroup WATCHDOG_SETUP
 *
 * This function disable the watchdog timer. This is useful during
 * development stage, but probably dangerous on deployed systems.
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_disable_watchdog(void);



/**
 * @brief Read the watchdog delay.
 *
 * @ingroup WATCHDOG_SETUP
 *
 * This function returns the number of seconds between a watchdog refresh and
 * system reboot.
 *
 * @return the delay on success, -1 on error and errno is set appropriately.
 */
int eris_get_watchdog_delay(void);



/**
 * @brief Set the watchdog delay.
 *
 * @ingroup WATCHDOG_SETUP
 *
 * This function configures the watchdog, then starts it.
 * 
 * @param delay     delay in seconds before reboot (must be in [1-48])
 *
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_set_watchdog_delay(int delay);



/**
 * @defgroup WATCHDOG_FEEDER
 * @ingroup  WATCHDOG
 * @brief    Functions to configure the automatic watchdog feeder.
 *
 * Eris Linux contains an automatic watchdog feeder that feeds the watchdog
 * every second.
 *
 * It is started automatically when the system boots.
 */

/**
 * @brief Start the automatic watchdog feeder.
 *
 * @ingroup WATCHDOG_FEEDER
 *
 * This function starts the automatic watchdog feeder if it is not already running.
 * 
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_start_watchdog_feeder(void);



/**
   * @brief Stop the automatic watchdog feeder.
   *
   * @ingroup WATCHDOG_FEEDER
   *
   * This function stops the automatic watchdog feeder if it is running.
   * 
   * @return 0 on success, -1 on error and errno is set appropriately.
   */
int eris_stop_watchdog_feeder(void);



/**
 * @brief Get the automatic watchdog feeder status
 *
 * @ingroup WATCHDOG_FEEDER
 *
 * This function fills the provided buffer with the automatic watchdog feeder
 * status ('running' or 'stopped').
 * 
 * @return 0 on success, -1 on error and errno is set appropriately.
 */
int eris_watchdog_feeder_status(char *buffer, size_t size);


#ifdef __cplusplus
}
#endif

#endif
