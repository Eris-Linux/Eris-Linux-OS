#ifndef SBOM_REST_API_H
#define SBOM_REST_API_H

	#include <microhttpd.h>

	int init_sbom_rest_api(const char *app);

	enum MHD_Result sbom_rest_api(struct MHD_Connection *connection, const char *url, const char *method);

#endif
