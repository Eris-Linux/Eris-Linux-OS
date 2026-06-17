/*
 *  ERIS LINUX API
 *
 *  Author: Christophe BLAESS.
 *
 *  (C) Logilin 2024-2026. All rights reserved
 */

#ifndef REBOOT_REST_API_H
#define REBOOT_REST_API_H

	#include <microhttpd.h>

	int init_reboot_rest_api(const char *app);

	enum MHD_Result reboot_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
