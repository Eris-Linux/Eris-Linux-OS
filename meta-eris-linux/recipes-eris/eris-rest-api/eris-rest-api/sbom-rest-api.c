
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "sbom-rest-api.h"


// ---------------------- Private macros declarations.

#define LICENSE_MANIFEST  "/usr/share/common-licenses/license.manifest"
#define GENERIC_PREFIX    "/usr/share/common-licenses/generic_"
#define PACKAGE_NAME_PREFIX     "PACKAGE NAME: "
#define RECIPE_NAME_PREFIX      "RECIPE NAME: "
#define PACKAGE_VERSION_PREFIX  "PACKAGE VERSION: "
#define PACKAGE_LICENSE_PREFIX  "LICENSE: "


// ---------------------- Private types and structures.

struct eris_api_package {
	char *name;
	char *version;
	char *details;
	char *licenses;
};


struct eris_api_license {
	char *name;
};


// ---------------------- Private method declarations.

static int  initialize_sbom(void);
static void add_package(const char *name, const char *version, const char *details);
static void add_license_if_not_exists(const char *name);
static int  compare_packages(const void *a , const void *b);
static int  compare_licenses(const void *a , const void *b);

static enum MHD_Result get_packages_list    (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_package_version  (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_package_licenses (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_licenses_list    (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_license_text     (struct MHD_Connection *connection, const char *url);


// ---------------------- Private variables.

static struct eris_api_package  *eris_api_packages = NULL;
static int nb_eris_api_packages = 0;

static struct eris_api_license  *eris_api_licenses = NULL;
static int nb_eris_api_licenses = 0;


// ---------------------- Public methods

int init_sbom_rest_api(const char *app)
{
	return initialize_sbom();
}



enum MHD_Result sbom_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcmp(method, "GET") != 0)
		return MHD_NO;

	if (strcasecmp(url, "/api/package/list") == 0)
		return get_packages_list(connection, url);

	if (strcasecmp(url, "/api/package/version") == 0)
		return get_package_version(connection, url);

	if (strcasecmp(url, "/api/package/licenses") == 0)
		return get_package_licenses(connection, url);

	if (strcasecmp(url, "/api/license/list") == 0)
		return get_licenses_list(connection, url);

	if (strcasecmp(url, "/api/license/text") == 0)
		return get_license_text(connection, url);

	return MHD_NO;
}


// ---------------------- Private methods

#define NAME_MAX  1024

static int initialize_sbom(void)
{
	FILE *fp = fopen(LICENSE_MANIFEST, "r");
	if (fp != NULL) {

		char line[1024];

		char package_version[NAME_MAX];
		char recipe_name[NAME_MAX];
		char package_license[NAME_MAX];

		while (fgets(line, 1024, fp) != NULL) {

			if (line[0] == '\0')
				continue;

			line[strlen(line) - 1] = '\0';

			if (strncmp(line, PACKAGE_NAME_PREFIX, strlen(PACKAGE_NAME_PREFIX)) == 0) {
				continue;

			} else if (strncmp(line, PACKAGE_VERSION_PREFIX, strlen(PACKAGE_VERSION_PREFIX)) == 0) {
				strncpy(package_version, &line[strlen(PACKAGE_VERSION_PREFIX)], NAME_MAX);
				package_version[NAME_MAX - 1] = '\0';

			} else if (strncmp(line, RECIPE_NAME_PREFIX, strlen(RECIPE_NAME_PREFIX)) == 0) {
				strncpy(recipe_name, &line[strlen(RECIPE_NAME_PREFIX)], NAME_MAX);
				recipe_name[NAME_MAX - 1] = '\0';

			} else if (strncmp(line, PACKAGE_LICENSE_PREFIX, strlen(PACKAGE_LICENSE_PREFIX)) == 0) {
				strncpy(package_license, &line[strlen(PACKAGE_LICENSE_PREFIX)], NAME_MAX);
				package_license[NAME_MAX - 1] = '\0';

			} else {
				add_package(recipe_name, package_version, package_license);
				recipe_name[0] = package_version[0] = package_license[0] = '\0';
			}
		}
		add_package(recipe_name, package_version, package_license);
		recipe_name[0] = package_version[0] = package_license[0] = '\0';
		fclose(fp);
	}

	qsort(eris_api_packages, nb_eris_api_packages, sizeof(struct eris_api_package), compare_packages);
	qsort(eris_api_licenses, nb_eris_api_licenses, sizeof(struct eris_api_license), compare_licenses);

	return 0;
}



static void add_package(const char *name, const char *version, const char *details)
{
	int i;

	if ((name[0] == '\0') || (version[0] == '\0') || (details[0] == '\0'))
		return;

	for (i = 0; i < nb_eris_api_packages; i++) {
		if (strcmp(name, eris_api_packages[i].name) == 0)
			break;
	}
	if (i != nb_eris_api_packages)
		return;

	struct eris_api_package *p = realloc(eris_api_packages, sizeof(struct eris_api_package) * (nb_eris_api_packages + 1));
	if (p != NULL) {
		eris_api_packages = p;
		p = &(eris_api_packages[nb_eris_api_packages]);
		nb_eris_api_packages ++;

		p->name = malloc(strlen(name) + 1);
		if (p->name != NULL)
			strcpy(p->name, name);

		p->version = malloc(strlen(version) + 1);
		if (p->version != NULL)
			strcpy(p->version, version);

		p->details = malloc(strlen(details) + 1);
		if (p->details != NULL)
			strcpy(p->details, details);

		p->licenses = malloc(strlen(details) + 1);
		if (p->licenses != NULL) {
			int j = 0;
			int k = 0;
			while (details[j] != '\0') {
				while ((isalnum(details[j]))
				    || (details[j] == '_')
				    || (details[j] == '-')
				    || (details[j] == '.'))
					p->licenses[k++] = details[j++];
				if (details[j] == '\0')
					break;
				p->licenses[k++] = ' ';
				while ((! isalnum(details[j]))
				    && (details[j] != '_')
				    && (details[j] != '-')
				    && (details[j] != '.')
				    && (details[j] != '\0'))
					j++;
			}
			p->licenses[k] = '\0';
		}
		int start = 0;
		while (p->licenses[start] != '\0') {
			while (p->licenses[start] == ' ') {
				start++;
				continue;
			}
			int stop = start + 1;
			while ((p->licenses[stop] != ' ') && (p->licenses[stop] != '\0'))
				stop++;
			char save = p->licenses[stop];
			p->licenses[stop] = '\0';
			add_license_if_not_exists(&p->licenses[start]);
			p->licenses[stop] = save;
			start = stop;
		}
	}
}



static void add_license_if_not_exists(const char *name)
{
	int i;
	for (i = 0; i < nb_eris_api_licenses; i++) {
		if (strcmp(eris_api_licenses[i].name, name) == 0)
			return;
	}
	struct eris_api_license  *p = realloc(eris_api_licenses, (nb_eris_api_licenses + 1) * sizeof(struct eris_api_license));
	if (p == NULL)
		return;
	eris_api_licenses = p;
	p = &(eris_api_licenses[nb_eris_api_licenses]);

	p->name = malloc(strlen(name) + 1);
	if (p->name == NULL)
		return;
	strcpy(p->name, name);
	nb_eris_api_licenses++;
}



static int compare_packages(const void *a , const void *b)
{
	const struct eris_api_package *pa = (struct eris_api_package *) a;
	const struct eris_api_package *pb = (struct eris_api_package *) b;
	return strcasecmp(pa->name, pb->name);
}



static int compare_licenses(const void *a , const void *b)
{
	const struct eris_api_license *la = (struct eris_api_license *) a;
	const struct eris_api_license *lb = (struct eris_api_license *) b;
	return strcasecmp(la->name, lb->name);
}



static enum MHD_Result get_packages_list(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	for (int i = 0; i < nb_eris_api_packages; i ++)
		addsnprintf(&reply, &size, &pos, "%s ", eris_api_packages[i].name);

	if (reply != NULL) {
		int ret = send_rest_response(connection, reply);
		free(reply);
		return ret;
	}
	return send_rest_error(connection, "No package found.", 404);
}



static enum MHD_Result get_package_version(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing package name.", 400);

	for (int i = 0; i < nb_eris_api_packages; i ++) {
		if (strcmp(name, eris_api_packages[i].name) == 0) {
			addsnprintf(&reply, &size, &pos, "%s", eris_api_packages[i].version);
			break;
		}
	}

	if (reply != NULL) {
		int ret = send_rest_response(connection, reply);
		free(reply);
		return ret;
	}
	return send_rest_error(connection, "Package not found.", 404);
}



static enum MHD_Result get_package_licenses(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing package name.", 400);

	for (int i = 0; i < nb_eris_api_packages; i ++) {
		if (strcmp(name, eris_api_packages[i].name) == 0) {
			addsnprintf(&reply, &size, &pos, "%s", eris_api_packages[i].details);
			break;
		}
	}

	if (reply != NULL) {
		int ret = send_rest_response(connection, reply);
		free(reply);
		return ret;
	}
	return send_rest_error(connection, "Package not found.", 404);
}



static enum MHD_Result get_licenses_list(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	for (int i = 0; i < nb_eris_api_licenses; i++) {
		addsnprintf(&reply, &size, &pos, "%s", eris_api_licenses[i].name);
		if (i < nb_eris_api_licenses - 1)
			addsnprintf(&reply, &size, &pos, " ");
	}

	if (reply != NULL) {
		int ret = send_rest_response(connection, reply);
		free(reply);
		return ret;
	}
	return send_rest_error(connection, "No license found.", 404);
}



static enum MHD_Result get_license_text(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing license name.", 400);

	if (strcasecmp(name, "CLOSED") == 0) {

		addsnprintf(&reply, &size, &pos, "This is a closed-source package.\nThere is no redistribution license.");

	} else {
		int len = strlen(name) + strlen(GENERIC_PREFIX);
		char *filename = malloc(len + 1);
		if (filename == NULL)
			return send_rest_error(connection, "No license found.", 404);
		snprintf(filename, len + 1, "%s%s", GENERIC_PREFIX, name);
		filename[len] = '\0';

		FILE *fp = fopen(filename, "r");
		free(filename);

		if (fp == NULL) {
			addsnprintf(&reply, &size, &pos, "The text of this license is not found.");

		} else {
			char line[4096];
			while (fgets(line, 4095, fp) != NULL)
				addsnprintf(&reply, &size, &pos, "%s", line);
			fclose(fp);
		}
	}
	if (reply != NULL) {
		int ret = send_rest_response(connection, reply);
		free(reply);
		return ret;
	}
	return send_rest_error(connection, "License not found.", 404);
}

