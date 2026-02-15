/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef UPDATE_REST_API_H
#define UPDATE_REST_API_H

	#include <microhttpd.h>

	int init_update_rest_api(const char *app);

	enum MHD_Result update_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
