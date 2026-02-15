
#ifndef GPIO_REST_API_H
#define GPIO_REST_API_H

	#include <microhttpd.h>

	int init_gpio_rest_api(const char *app);

	enum MHD_Result gpio_rest_api(struct MHD_Connection *connection, const char *url, const char *method);


#endif
