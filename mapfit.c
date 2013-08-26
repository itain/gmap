/*
 * mapfit.c
 * Copyright (C) 2007 Itai Nahshon
 *
 * The mathematics sed to caculate the GeoTransform from the calibration points of a map.
 */
#include "gmap.h"

/* Invert a 2X2 matrix */
static bool
Matrix_invert(const double mat[4], double inv[4]) {
	double det;

	det = mat[0] * mat[3] - mat[1] * mat[2];
	if(det == 0) {
		fprintf(stderr, "Matrix_invert: det = 0\n");
		return FALSE;
	}
	inv[0] = mat[3] / det;
	inv[1] = -(mat[1] / det);
	inv[2] = -(mat[2] / det);
	inv[3] = mat[0] / det;

	return TRUE;
}

/* Multiply two 2X2 matrices */
static void
Matrix_mult(const double mat1[4], const double mat2[4], double res[4]) {
	res[0] = mat1[0] * mat2[0] + mat1[1] * mat2[2];
	res[1] = mat1[0] * mat2[1] + mat1[1] * mat2[3];
	res[2] = mat1[2] * mat2[0] + mat1[3] * mat2[2];
	res[3] = mat1[2] * mat2[1] + mat1[3] * mat2[3];
}

/* 3 points or more! */
static bool
gt_from_cpoints(struct cpoint **cpoints, int n_cpoints, double GeoTransform[6]) {
	double sxx = 0.0;
	double syy = 0.0;
	double sxy = 0.0;
	double txx = 0.0;
	double txy = 0.0;
	double tyx = 0.0;
	double tyy = 0.0;
	double avmapx = 0.0;
	double avmapy = 0.0;
	double avgeox = 0.0;
	double avgeoy = 0.0;
	int i;
	double m1[4], m2[4], m3[4], m4[4];
	for(i = 0; i < n_cpoints; i++) {
		avmapx += cpoints[i]->mapx;
		avmapy += cpoints[i]->mapy;
		avgeox += cpoints[i]->geox;
		avgeoy += cpoints[i]->geoy;
	}
	avmapx /= n_cpoints;
	avmapy /= n_cpoints;
	avgeox /= n_cpoints;
	avgeoy /= n_cpoints;
	for(i = 0; i < n_cpoints; i++) {
		sxx += (cpoints[i]->mapx - avmapx) * (cpoints[i]->mapx - avmapx);
		syy += (cpoints[i]->mapy - avmapy) * (cpoints[i]->mapy - avmapy);
		sxy += (cpoints[i]->mapx - avmapx) * (cpoints[i]->mapy - avmapy);
		txx += (cpoints[i]->mapx - avmapx) * (cpoints[i]->geox - avgeox);
		txy += (cpoints[i]->mapx - avmapx) * (cpoints[i]->geoy - avgeoy);
		tyx += (cpoints[i]->mapy - avmapy) * (cpoints[i]->geox - avgeox);
		tyy += (cpoints[i]->mapy - avmapy) * (cpoints[i]->geoy - avgeoy);
	}
        if(sxx == 0 || syy == 0)
		return FALSE;		// All points are in a straight line along an axis
	m1[0] = 1;
	m1[1] = sxy / syy;
	m1[2] = sxy / sxx;
	m1[3] = 1;
	if(!Matrix_invert(m1, m2))	// m2 = m1 ** -1
		return FALSE;		// All points are otherwise in a straight line
	m3[0] = txx / sxx;
	m3[1] = tyx / syy;
	m3[2] = txy / sxx;
	m3[3] = tyy / syy;
	Matrix_mult(m3, m2, m4);	// m4 = m3 X m2
	GeoTransform[0] = avgeox - avmapx * m4[0] - avmapy * m4[1];
	GeoTransform[1] = m4[0];
	GeoTransform[2] = m4[1];
	GeoTransform[3] = avgeoy - avmapx * m4[2] - avmapy * m4[3];
	GeoTransform[4] = m4[2];
	GeoTransform[5] = m4[3];
	return TRUE;
}

#if 0
/* Try to project every point and calculate the error */
void
map_calc_GeoReference_error(struct Map *map) {
	int i;
	double gx, gy;

	for(i = 0; i < map->n_calibration_points; i++) {
		if(!map->calibration_points[i].valid) {
			map->calibration_points[i].errx = 0.0;
			map->calibration_points[i].erry = 0.0;
			continue;
		}

		gx = map->GeoTransform[0] +
			map->calibration_points[i].mapx * map->GeoTransform[1] +
			map->calibration_points[i].mapy * map->GeoTransform[2];

		gy = map->GeoTransform[3] +
			map->calibration_points[i].mapx * map->GeoTransform[4] +
			map->calibration_points[i].mapy * map->GeoTransform[5];

		map->calibration_points[i].errx = gx - map->calibration_points[i].geox;
		map->calibration_points[i].erry = gy - map->calibration_points[i].geoy;
	}
}
#endif

bool
map_calc_GeoReference(struct Map *map) {
	struct cpoint **cpoint;
	int i, n;
	bool ret = FALSE;

	if(map->n_calibration_points < 2)
		return FALSE;

	cpoint = (struct cpoint **)gmap_malloc(map->n_calibration_points * sizeof(struct cpoint *));
	if(cpoint == NULL)
		return FALSE;

	n = 0;
	for(i = 0; i < map->n_calibration_points; i++) {
		if(map->calibration_points[i].valid)
			cpoint[n++] = &map->calibration_points[i];
	}

	if(n > 2) {
		ret =  gt_from_cpoints(cpoint, n, map->GeoTransform);
	}
	else if(n == 2 &&
			cpoint[0]->mapx != cpoint[1]->mapx &&
			cpoint[0]->mapy != cpoint[1]->mapy) {
		/* Assume map has NO rotation ! */
		map->GeoTransform[2] = 0.0;
		map->GeoTransform[4] = 0.0;
		map->GeoTransform[1] = (cpoint[1]->geox - cpoint[0]->geox) / (cpoint[1]->mapx - cpoint[0]->mapx);
		map->GeoTransform[5] = (cpoint[1]->geoy - cpoint[0]->geoy) / (cpoint[1]->mapy - cpoint[0]->mapy);
		map->GeoTransform[0] = cpoint[0]->geox - cpoint[0]->mapx * map->GeoTransform[1];
		map->GeoTransform[3] = cpoint[0]->geoy - cpoint[0]->mapy * map->GeoTransform[5];
		ret = TRUE;
	}

	gmap_free(cpoint);
	return ret;
}
