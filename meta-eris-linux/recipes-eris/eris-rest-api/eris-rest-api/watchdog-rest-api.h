/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2026: Logilin
 *  All rights reserved
 */

#ifndef WATCHDOG_REST_API_H
#define WATCHDOG_REST_API_H

	#include <microhttpd.h>

	int init_watchdog_rest_api(const char *name);

	enum MHD_Result watchdog_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
