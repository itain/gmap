/*
 * affinegrid.c
 * (Copyright C) 2007 Itai Nahshon
 *
 * Implement a grid layer where all grid lines are straight and parallel.
 */
#include "gmap.h"
#include <math.h>

static void
affine_grid_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	double x, y;
	double	xmin, xmax, ymin, ymax;
	double xlo, ylo;
	double x0, y0, x1, y1;
	double r, g, b;
	struct AffineGridData *ad = (struct AffineGridData *)layer->data;
	double GeoTransform[6];

	if(ad->GeoTransform)
		multiply_geotransform(rc->rt->GeoTransform, ad->GeoTransform, GeoTransform);
	else
		copy_geotransform(rc->rt->GeoTransform, GeoTransform);

	pixel_to_geo_xy(GeoTransform, rc->x, rc->y, &x, &y); xmin = x; xmax = x; ymin = y; ymax = y;
	pixel_to_geo_xy(GeoTransform, rc->x+rc->w, rc->y, &x, &y); xmin = MIN(x, xmin); xmax = MAX(x, xmax); ymin = MIN(y, ymin); ymax = MAX(y, ymax);
	pixel_to_geo_xy(GeoTransform, rc->x, rc->y+rc->h, &x, &y); xmin = MIN(x, xmin); xmax = MAX(x, xmax); ymin = MIN(y, ymin); ymax = MAX(y, ymax);
	pixel_to_geo_xy(GeoTransform, rc->x+rc->w, rc->y+rc->h, &x, &y); xmin = MIN(x, xmin); xmax = MAX(x, xmax); ymin = MIN(y, ymin); ymax = MAX(y, ymax);
	xlo = xmax - fmod(xmax, ad->hspaces);
	ylo = ymax - fmod(ymax, ad->vspaces);

	cairo_set_line_width(rc->ct, ad->line_width);
	r = ((ad->vcolor >> 16) & 0xff) / 255.;
	g = ((ad->vcolor >> 8) & 0xff) / 255.;
	b = ((ad->vcolor >> 0) & 0xff) / 255.;
	cairo_set_source_rgb(rc->ct, r, g, b);
	while(xlo >= xmin) {
		geo_to_pixel_xy(GeoTransform, xlo, ymin, &x0, &y0);
		geo_to_pixel_xy(GeoTransform, xlo, ymax, &x1, &y1);
		cairo_move_to(rc->ct, x0-rc->x, y0-rc->y);
		cairo_line_to(rc->ct, x1-rc->x, y1-rc->y);
		cairo_stroke(rc->ct);
		
		xlo -= ad->hspaces;
	}

	r = ((ad->hcolor >> 16) & 0xff) / 255.;
	g = ((ad->hcolor >> 8) & 0xff) / 255.;
	b = ((ad->hcolor >> 0) & 0xff) / 255.;
	cairo_set_source_rgb(rc->ct, r, g, b);
	while(ylo >= ymin) {
		geo_to_pixel_xy(GeoTransform, xmin, ylo, &x0, &y0);
		geo_to_pixel_xy(GeoTransform, xmax, ylo, &x1, &y1);
		cairo_move_to(rc->ct, x0-rc->x, y0-rc->y);
		cairo_line_to(rc->ct, x1-rc->x, y1-rc->y);
		cairo_stroke(rc->ct);
		
		ylo -= ad->vspaces;
	}
}

static void
affine_grid_calc_target_data(struct Layer *layer, const struct RenderTarget *target) { }

static const struct LayerOps affine_grid_layer_ops = {
	affine_grid_render_layer,
	affine_grid_calc_target_data,
	NULL,
};

void
affine_grid_init_layer(struct Layer *layer, enum LayerType type, struct AffineGridData *data)  {
	layer->type = type;
	layer->ops = &affine_grid_layer_ops;
	layer->flags = LAYER_IS_VISIBLE;
	layer->data = data;
}
