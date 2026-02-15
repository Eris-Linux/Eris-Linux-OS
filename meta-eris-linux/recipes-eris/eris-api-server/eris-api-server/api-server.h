/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef API_SERVER_H
#define API_SERVER_H

	int  register_api_command(const char *command, const char *abbreviation, const char *help, void (*function)(int sock, int argc, char *argv[]));

	void send_reply(int sock, size_t length, const void *data);
	void send_error(int sock, int code, const char *label);

	int  addsnprintf(char **string, size_t *size, size_t *pos, const char *format, ...);

	int read_parameter_value(const char *parameter, char **value);
	int write_parameter_value(const char *parameter, const char *value);

#endif
