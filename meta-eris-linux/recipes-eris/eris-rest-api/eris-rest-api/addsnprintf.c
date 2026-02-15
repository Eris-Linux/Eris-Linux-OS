#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "addsnprintf.h"

int addsnprintf(char **string, size_t *size, size_t *pos, const char *format, ...)
{
	ssize_t  length;
	va_list  list;
	char    *newstring;

	va_start(list, format);
	length = vsnprintf(NULL, 0, format, list);
	va_end(list);

	if (length < 0)
		return -1;

	if ((length + (*pos)) >= *size) {
		newstring = realloc(*string, length + (*pos) + 1);
		if (newstring == NULL)
			return -1;
		*string = newstring;
		*size = length + (*pos) + 1;
	}

	va_start(list, format);
	length = vsnprintf(*string + *pos, *size, format, list);
	va_end(list);
	if (length < 0)
		return -1;

	*pos += length;
	(*string)[*pos] = '\0';

	return 0;
}



