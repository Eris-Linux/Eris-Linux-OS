/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef NET_REST_API_H
#define NET_REST_API_H

	#include <microhttpd.h>

	int init_net_rest_api(const char *app);

	enum MHD_Result net_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
