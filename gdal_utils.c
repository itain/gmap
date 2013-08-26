/*
 * gdal_utils.c
 * Copyright (C) 2007 Itai Nahshon
 *
 * Simple helper functions related to GeoTransforms
 */
#include "gmap.h"

void
pixel_to_geo_xy(const double *GeoTransform, double pixel_x, double pixel_y, double *geo_x, double *geo_y) {
	*geo_x = GeoTransform[0] + pixel_x * GeoTransform[1] + pixel_y * GeoTransform[2];
	*geo_y = GeoTransform[3] + pixel_x * GeoTransform[4] + pixel_y * GeoTransform[5];
}

bool
geo_to_pixel_xy(const double *GeoTransform, double geo_x, double geo_y, double *pixel_x, double *pixel_y) {
	double det = GeoTransform[1] * GeoTransform[5] - GeoTransform[2] * GeoTransform[4];

	if(det == 0.0)
		return FALSE;

	geo_x -= GeoTransform[0];
	geo_y -= GeoTransform[3];

	*pixel_x = (geo_x * GeoTransform[5] - geo_y * GeoTransform[2] ) / det;
	*pixel_y = (geo_y * GeoTransform[1] - geo_x * GeoTransform[4] ) / det;

	return TRUE;
}

void
multiply_geotransform(const double *G1, const double *G2, double *res) {
	res[0] = G2[0] + G1[0]*G2[1] + G1[3]*G2[2];
	res[1] = G1[1]*G2[1] + G1[4]*G2[2];
	res[2] = G1[2]*G2[1] + G1[5]*G2[2];
	res[3] = G2[3] + G1[0]*G2[4] + G1[3]*G2[5];
	res[4] = G1[1]*G2[4] + G1[4]* G2[5];
	res[5] = G1[2]*G2[4] + G1[5]* G2[5];
}

void
copy_geotransform(const double *G1, double *res) {
	res[0] = G1[0];
	res[1] = G1[1];
	res[2] = G1[2];
	res[3] = G1[3];
	res[4] = G1[4];
	res[5] = G1[5];
}

void
set_unity_geotransform(double *res) {
	res[0] = 0.0;
	res[1] = 1.0;
	res[2] = 0.0;
	res[3] = 0.0;
	res[4] = 0.0;
	res[5] = 1.0;
}

/* rotate about the point x,y - NOT TESTED */
void
set_rotated_geotransform(double *res, double rotation, double x, double y) {
	double c = cos(rotation * M_PI/180);
	double s = sin(rotation * M_PI/180);

	res[0] = c*x+s*y-x;
	res[1] = c;
	res[2] = s;
	res[3] = c*y-s*x-y;
	res[4] = -s;
	res[5] = c;
}

bool
is_unity_geotransform(double *GeoTransform) {
	return(GeoTransform[0] == 0.0 &&
		GeoTransform[1] == 1.0 &&
		GeoTransform[2] == 0.0 &&
		GeoTransform[3] == 0.0 &&
		GeoTransform[4] == 0.0 &&
		GeoTransform[5] == 1.0);
}

void
degrees_to_DMS(double degrees, bool *minus, int *d, int *m, double *s) {
	*minus = degrees < 0;

	degrees = fabs(degrees);

	*d = (int)floor(degrees);
	degrees = 60 * (degrees - *d);
	*m = (int)floor(degrees);
	*s = 60 * (degrees - *m);
}

char *
degrees_to_DMS_str(double degrees) {
	char tmp[256];
	bool minus;
	int d, m;
	double s;

	degrees_to_DMS(degrees, &minus, &d, &m, &s);

	snprintf(tmp, sizeof(tmp), "%s%d %d'%f\"",
		minus?"-":"", d, m, s);

	return gmap_strdup(tmp);
}
