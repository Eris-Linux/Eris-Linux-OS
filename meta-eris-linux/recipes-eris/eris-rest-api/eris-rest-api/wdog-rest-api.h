/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2026: Logilin
 *  All rights reserved
 */

#ifndef WDOG_REST_API_H
#define WDOG_REST_API_H

	#include <microhttpd.h>

	int init_wdog_rest_api(const char *name);

	enum MHD_Result wdog_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
