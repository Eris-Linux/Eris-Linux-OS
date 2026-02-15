/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025-2026 Logilin. All rights reserved.
*/

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <liberis.h>

#include "eris-api-test.h"
#include "gpio-api-test.h"
#include "network-api-test.h"
#include "sbom-api-test.h"
#include "system-api-test.h"
#include "time-api-test.h"
#include "update-api-test.h"
#include "wdog-api-test.h"


// ---------------------- Private macros.

#define CONNECTION_PORT   10000


// ---------------------- Private method declarations.

static void *thread_function  (void *arg);


// ---------------------- Private variables.

// ---------------------- Public variable definitions.

// ---------------------- Public methods

int main(void)
{
	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	int option = 1;
	(void) setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(CONNECTION_PORT);
	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	listen(server_sock, 3);

	int client_sock;
	while ((client_sock = accept(server_sock, NULL, 0)) != -1) {
		pthread_t client_thread;
		unsigned long int  arg = (unsigned long int) client_sock;
		if (pthread_create(&client_thread, NULL, thread_function, (void*)arg) < 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
		pthread_detach(client_thread);
	}

	close(server_sock);
	return EXIT_SUCCESS;
}



int sockprintf(int sockfd, const char *format, ...)
{
	va_list args;
	va_list args_copy;

	va_start(args, format);
	va_copy(args_copy, args);

	int size = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);
	if (size < 0) {
		va_end(args);
		return -1;
	}

	char *buffer = malloc(size + 1);
	if (buffer == NULL) {
		va_end(args);
		return -1;
	}

	vsnprintf(buffer, size + 1, format, args);
	va_end(args);

	int bytes_sent = send(sockfd, buffer, size, 0);

	free(buffer);
	return bytes_sent;
}



char *sockgets(int sockfd, char *buffer, size_t size)
{
	if ((buffer == NULL) || (size == 0))
		return NULL;

	int bytes_read = read(sockfd, buffer, size - 1);
	if (bytes_read <= 0)
		return NULL;
	buffer[bytes_read] = '\0';
	int i = 0;
	while ((buffer[i] != '\n') && (buffer[i] != '\r') && (buffer[i] != '\0'))
		i++;
	buffer[i] = '\0';
	return buffer;
}


// ---------------------- Private methods

static void *thread_function(void *arg)
{
	unsigned long ulongarg = (unsigned long int)arg;
	int sockfd = (int) ulongarg;

	for (;;) {
		sockprintf(sockfd, "\r\n**** Eris Linux API test *****\r\n\n");
		sockprintf(sockfd, "1: System Identification        6: Software Bill of Materials \r\n");
		sockprintf(sockfd, "2: System & Containers Update   7: Network Interfaces         \r\n");
		sockprintf(sockfd, "3: Time Setup                   8: General Purpose I/O        \r\n");
		sockprintf(sockfd, "4: Watchdog Configuration       9: Display Features(*)        \r\n");
		sockprintf(sockfd, "5: Audio Features(*)                                          \r\n");
		sockprintf(sockfd, "0: Quit                                                       \r\n");
		sockprintf(sockfd, "                      (*) Coming soon                         \r\n");
		sockprintf(sockfd, "Your choice: ");

		char buffer[BUFFER_SIZE];

		if (sockgets(sockfd, buffer, BUFFER_SIZE) == NULL)
			break;

		if (strcmp(buffer, "0") == 0)
			break;

		if (strcmp(buffer, "1") == 0)
			if (system_info_api_test(sockfd) < 0)
				break;

		if (strcmp(buffer, "2") == 0)
			if (update_api_test(sockfd) < 0)
				break;

		if (strcmp(buffer, "3") == 0)
			if (time_api_test(sockfd) < 0)
					break;

		if (strcmp(buffer, "4") == 0)
			if (wdog_api_test(sockfd) < 0)
				break;

		if (strcmp(buffer, "6") == 0)
			if (sbom_api_test(sockfd) < 0)
				break;

		if (strcmp(buffer, "7") == 0)
			if (network_api_test(sockfd) < 0)
				break;

		if (strcmp(buffer, "8") == 0)
			if (gpio_api_test(sockfd) < 0)
				break;

//		if (strcmp(buffer, "9") == 0)
//			if (display_api_test(sockfd) < 0)
//				break;

//		if (strcmp(buffer, "5") == 0)
//			if (audio_api_test(sockfd) < 0)
//				break;
	}

	close(sockfd);
	return NULL;
}


