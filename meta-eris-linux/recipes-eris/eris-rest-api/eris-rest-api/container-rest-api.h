/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef CONTAINER_REST_API_H
#define CONTAINER_REST_API_H

	#include <microhttpd.h>

	int init_container_rest_api(const char *app);

	enum MHD_Result container_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
