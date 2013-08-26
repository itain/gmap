/*
 * file_utils.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"

/*
If it's possible, return just the relative part of filename. Otherwise return the full name.
*/
char *
get_relative_filename(const char *filename, const char *basedir) {
	size_t len;
	if(filename == NULL)
		return NULL;
	
	if(!g_path_is_absolute(filename) || basedir == NULL || !g_path_is_absolute(basedir))
		return gmap_strdup(filename);	// what else can we do?

	len = strlen(basedir);

	if(strlen(filename) <= len +2)	// at least dir-separator and one more character
		return gmap_strdup(filename);

	if(basedir[len-1] == '/' || basedir[len-1] == '\\')
		len--;

	if(filename[len] != '/' && filename[len-1] != '\\')
		return gmap_strdup(filename);

	if(strncmp(basedir, filename, len))
		return gmap_strdup(filename);

	return gmap_strdup(filename+len+1);
}

/* Some functions always require the full path! */
char *
get_absolute_filename(const char *filename) {
	if(g_path_is_absolute(filename))
		return gmap_strdup(filename);
	else
		return g_build_filename(
			g_get_current_dir(),
			filename, NULL);
}
