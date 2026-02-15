#ifndef ERIS_REST_API_H
#define ERIS_REST_API_H

#include <microhttpd.h>

enum MHD_Result send_rest_error    (struct MHD_Connection *connection, const char *err_message, unsigned int err_code);
enum MHD_Result send_rest_response (struct MHD_Connection *connection, const char *reply_message);

int read_parameter_value(const char *parameter, char **value);
int write_parameter_value(const char *parameter, const char *value);

#endif


