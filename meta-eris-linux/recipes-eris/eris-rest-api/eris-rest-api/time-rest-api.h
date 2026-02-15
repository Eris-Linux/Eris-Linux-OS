/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef TIME_REST_API_H
#define TIME_REST_API_H

	#include <microhttpd.h>

	int init_time_rest_api(const char *app);

	enum MHD_Result time_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
