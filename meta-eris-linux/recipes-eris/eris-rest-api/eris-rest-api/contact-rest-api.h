/*
 *  ERIS LINUX API
 *
 *  (c) 2024-2025: Logilin
 *  All rights reserved
 */

#ifndef CONTACT_REST_API_H
#define CONTACT_REST_API_H

	#include <microhttpd.h>

	int init_contact_rest_api(const char *app);

	enum MHD_Result contact_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
