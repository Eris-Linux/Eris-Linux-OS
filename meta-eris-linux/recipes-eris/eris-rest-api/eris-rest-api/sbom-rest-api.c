
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "addsnprintf.h"
#include "eris-rest-api.h"
#include "sbom-rest-api.h"


// ---------------------- Private macros declarations.

#define LICENSE_MANIFEST        "/usr/share/common-licenses/license.manifest"
#define GENERIC_PREFIX          "/usr/share/common-licenses/generic_"

#define PACKAGE_NAME_PREFIX     "PACKAGE NAME: "
#define PACKAGE_VERSION_PREFIX  "PACKAGE VERSION: "
#define RECIPE_NAME_PREFIX      "RECIPE NAME: "
#define PACKAGE_LICENSE_PREFIX  "LICENSE: "


// ---------------------- Private types and structures.

struct eris_api_package {
	char *name;
	char *version;
	char *recipe;
	char *licenses;
	char *details;
};


struct eris_api_license {
	char *name;
};


// ---------------------- Private method declarations.

static int  initialize_sbom(void);
static void add_package(const char *name, const char *version, const char *recipe, const char *license_details);
static void add_license_if_not_exists(const char *name);
static int  compare_packages(const void *a , const void *b);
static int  compare_licenses(const void *a , const void *b);

static enum MHD_Result get_packages_list    (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_package_version  (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_package_recipe   (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_package_licenses (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_licenses_list    (struct MHD_Connection *connection, const char *url);
static enum MHD_Result get_license_text     (struct MHD_Connection *connection, const char *url);


// ---------------------- Private variables.

static struct eris_api_package *eris_api_packages = NULL;
static int nb_eris_api_packages = 0;

static struct eris_api_license *eris_api_licenses = NULL;
static int nb_eris_api_licenses = 0;


// ---------------------- Public methods

int init_sbom_rest_api(const char *app)
{
	(void) app;

	return initialize_sbom();
}



enum MHD_Result sbom_rest_api(struct MHD_Connection *connection, const char *url, const char *method)
{
	if (strcmp(method, "GET") != 0)
		return MHD_NO;

	if (strcmp(url, "/api/sbom/package-list") == 0)
		return get_packages_list(connection, url);

	if (strcmp(url, "/api/sbom/package-version") == 0)
		return get_package_version(connection, url);

	if (strcmp(url, "/api/sbom/package-recipe") == 0)
		return get_package_recipe(connection, url);

	if (strcmp(url, "/api/sbom/package-licenses") == 0)
		return get_package_licenses(connection, url);

	if (strcmp(url, "/api/sbom/license-list") == 0)
		return get_licenses_list(connection, url);

	if (strcmp(url, "/api/sbom/license-text") == 0)
		return get_license_text(connection, url);

	return MHD_NO;
}


// ---------------------- Private methods

#define ERIS_FIELD_MAX  1024

static int initialize_sbom(void)
{
	FILE *fp = fopen(LICENSE_MANIFEST, "r");
	if (fp != NULL) {

		char line[1024];

		char package_name[ERIS_FIELD_MAX] = "";
		char package_version[ERIS_FIELD_MAX] = "";
		char recipe_name[ERIS_FIELD_MAX] = "";
		char license_details[ERIS_FIELD_MAX] = "";

		while (fgets(line, sizeof(line), fp) != NULL) {

			if (line[0] == '\0') {
				if (package_name[0] != '\0')
					add_package(package_name, package_version, recipe_name, license_details);
				package_name[0] = package_version[0] = recipe_name[0] = license_details[0] = '\0';
				continue;
			}

			line[strcspn(line, "\n")] = '\0';

			if (strncmp(line, PACKAGE_NAME_PREFIX, strlen(PACKAGE_NAME_PREFIX)) == 0) {
				strncpy(package_name, &line[strlen(PACKAGE_VERSION_PREFIX)], sizeof(package_name));
				package_name[sizeof(package_name) - 1] = '\0';

			} else if (strncmp(line, PACKAGE_VERSION_PREFIX, strlen(PACKAGE_VERSION_PREFIX)) == 0) {
				strncpy(package_version, &line[strlen(PACKAGE_VERSION_PREFIX)], sizeof(package_version));
				package_version[sizeof(package_version) - 1] = '\0';

			} else if (strncmp(line, RECIPE_NAME_PREFIX, strlen(RECIPE_NAME_PREFIX)) == 0) {
				strncpy(recipe_name, &line[strlen(RECIPE_NAME_PREFIX)], sizeof(recipe_name) );
				recipe_name[sizeof(recipe_name) - 1] = '\0';

			} else if (strncmp(line, PACKAGE_LICENSE_PREFIX, strlen(PACKAGE_LICENSE_PREFIX)) == 0) {
				strncpy(license_details, &line[strlen(PACKAGE_LICENSE_PREFIX)], sizeof(license_details));
				license_details[sizeof(license_details) - 1] = '\0';
			}
		}
		fclose(fp);
	}

	qsort(eris_api_packages, nb_eris_api_packages, sizeof(struct eris_api_package), compare_packages);
	qsort(eris_api_licenses, nb_eris_api_licenses, sizeof(struct eris_api_license), compare_licenses);

	return 0;
}



static void add_package(const char *package_name, const char *package_version, const char *recipe_name, const char *license_details)
{
	int i;

	if ((package_name == NULL) || (package_version == NULL) || (recipe_name == NULL) || (license_details == NULL))
		return;

	if ((package_name[0] == '\0') || (package_version[0] == '\0') || (recipe_name[0] == '\0') || (license_details[0] == '\0'))
		return;

	for (i = 0; i < nb_eris_api_packages; i++) {
		if (strcmp(package_name, eris_api_packages[i].name) == 0)
			return;
	}

	struct eris_api_package pkg;

	memset(&pkg, 0, sizeof(pkg));

	pkg.name     = strdup(package_name);
	pkg.version  = strdup(package_version);
	pkg.recipe   = strdup(recipe_name);
	pkg.details  = strdup(license_details);
	pkg.licenses = strdup(license_details);

	if ((pkg.name == NULL) || (pkg.version == NULL) || (pkg.recipe == NULL) || (pkg.licenses == NULL) || (pkg.details == NULL)) {
		free(pkg.name);
		free(pkg.version);
		free(pkg.recipe);
		free(pkg.details);
		free(pkg.licenses);
		return;
	}

	int j = 0;
	int k = 0;
	while (pkg.details[j] != '\0') {
		while ((isalnum((unsigned char)(pkg.details[j])))
		    || (pkg.details[j] == '_')
		    || (pkg.details[j] == '-')
		    || (pkg.details[j] == '.'))
			pkg.licenses[k++] = pkg.details[j++];
		if (pkg.details[j] == '\0')
			break;
		pkg.licenses[k++] = ' ';
		while ((! isalnum((unsigned char)(pkg.details[j])))
		    && (pkg.details[j] != '_')
		    && (pkg.details[j] != '-')
		    && (pkg.details[j] != '.')
		    && (pkg.details[j] != '\0'))
			j++;
	}
	pkg.licenses[k] = '\0';

	int start = 0;
	while (pkg.licenses[start] != '\0') {
		while (pkg.licenses[start] == ' ') {
			start++;
			continue;
		}
		int stop = start + 1;
		while ((pkg.licenses[stop] != ' ') && (pkg.licenses[stop] != '\0')) {
			stop++;
			char save = pkg.licenses[stop];
			pkg.licenses[stop] = '\0';
			add_license_if_not_exists(&pkg.licenses[start]);
			pkg.licenses[stop] = save;
			start = stop;
		}
	}


	struct eris_api_package *new_packages;
	new_packages = realloc(eris_api_packages, sizeof(struct eris_api_package) * (nb_eris_api_packages + 1));
	if (new_packages == NULL) {
		free(pkg.name);
		free(pkg.version);
		free(pkg.recipe);
		free(pkg.details);
		free(pkg.licenses);
		return;
	}

	eris_api_packages = new_packages;
	eris_api_packages[nb_eris_api_packages++] = pkg;

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
	if ((pa == NULL) || (pb == NULL))
		return 0;
	if ((pa->name == NULL) && (pb->name == NULL))
		return 0;
	if (pa->name == NULL)
		return 1;
	if (pb->name == NULL)
		return -1;
	return strcasecmp(pa->name, pb->name);
}



static int compare_licenses(const void *a , const void *b)
{
	const struct eris_api_license *la = (struct eris_api_license *) a;
	const struct eris_api_license *lb = (struct eris_api_license *) b;
	if ((la == NULL) || (lb == NULL))
		return 0;
	if ((la->name == NULL) || (lb->name == NULL))
		return 0;
	if (la->name == NULL)
		return 1;
	if (lb->name == NULL)
		return -1;
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



static enum MHD_Result get_package_recipe(struct MHD_Connection *connection, const char *url)
{
	char *reply = NULL;
	size_t size = 0;
	size_t pos = 0;

	const char *name = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
	if (name == NULL)
	        return send_rest_error(connection, "Missing package name.", 400);

	for (int i = 0; i < nb_eris_api_packages; i ++) {
		if (strcmp(name, eris_api_packages[i].name) == 0) {
			addsnprintf(&reply, &size, &pos, "%s", eris_api_packages[i].recipe);
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
			return send_rest_error(connection, "Allocation error.", 500);
		snprintf(filename, len + 1, "%s%s", GENERIC_PREFIX, name);
		filename[len] = '\0';

		FILE *fp = fopen(filename, "r");
		free(filename);

		if (fp == NULL) {
			addsnprintf(&reply, &size, &pos, "The text of this license is not found.");

		} else {
			char line[4096];
			while (fgets(line, sizeof(line), fp) != NULL)
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

