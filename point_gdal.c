/*
 * point_gdal.c
 * Copyright (C) 2007 Itai Nahshon
 *
 */
#include "gmap.h"
#include <ogr_api.h>
#include <cpl_conv.h>

static bool
point_calc_mapview_data(struct Point *point, const struct RenderTarget *target, OGRCoordinateTransformationH xform, int *x, int *y) {
	double geo_x, geo_y;
	double screen_x, screen_y;

	/* The point is in geographic lon/lat coordinates */
	geo_x = point->geo_lon;
	geo_y = point->geo_lat;

	/* Transfom it to screen projection */
	if(xform != NULL && !OCTTransform(xform, 1, &geo_x, &geo_y, NULL)) {
		return FALSE;
	}

	if(!geo_to_pixel_xy(target->GeoTransform, geo_x, geo_y, &screen_x, &screen_y)) {
	 	return FALSE;
	}

	*x = (int)screen_x;
	*y = (int)screen_y;

	return TRUE;
}

void
trackset_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
	int i, j, k;
	bool t1, t2;
	int x1, x2, y1, y2;
	struct TrackPoint *p1, *p2;
	OGRSpatialReferenceH osrsSrc, osrsDst;
	OGRCoordinateTransformationH xform;
	struct TrackSet *trackset = (struct TrackSet *)layer->data;

	osrsSrc = OSRNewSpatialReference(NULL);
	OSRSetWellKnownGeogCS(osrsSrc, "WGS84");
	osrsDst = OSRNewSpatialReference(target->WKT);
	xform = OCTNewCoordinateTransformation(osrsSrc, osrsDst);
	OSRDestroySpatialReference(osrsDst);
	OSRDestroySpatialReference(osrsSrc);

	free_tree(layer->priv);

	layer->priv = new_branch(0, target->height, 0, target->width);

	for(i = 0; i < trackset->count; i++) {
		for(j = 0; j < trackset->tracks[i].count; j++) {
			t1 = t2 = FALSE;
			p1 = p2 = NULL;
			for(k = 0; k < trackset->tracks[i].tracksegments[j].count; k++) {

				p2 = &trackset->tracks[i].tracksegments[j].trackpoints[k];
				t2 = point_calc_mapview_data(&p2->point, target, xform, &x2, &y2);

				if(t1 && t2) {
					if(x1 == x2 && y1 == y2)
						continue;
					tree_add_segment(
						layer->priv,
						&trackset->tracks[i],
						x1, y1, x2, y2,
						p1, p2);
				}
				x1 = x2;
				y1 = y2;
				t1 = t2;
				p1 = p2;
			}
		}
	}

	layer->flags |= LAYER_IS_TREE_SEARCHABLE;

	OCTDestroyCoordinateTransformation(xform);
}

bool
trackset_calc_extents(struct TrackSet *trackset, const struct RenderTarget *target, struct GeoRect *rect) {
	int i, j, k;
	struct TrackPoint *p;
	OGRSpatialReferenceH osrsSrc, osrsDst;
	OGRCoordinateTransformationH xform;
	double geo_x, geo_y;

	bool first = TRUE;

	osrsSrc = OSRNewSpatialReference(NULL);
	OSRSetWellKnownGeogCS(osrsSrc, "WGS84");
	osrsDst = OSRNewSpatialReference(target->WKT);
	xform = OCTNewCoordinateTransformation(osrsSrc, osrsDst);
	OSRDestroySpatialReference(osrsDst);
	OSRDestroySpatialReference(osrsSrc);

	rect->left = rect->right = rect->top = rect->bottom = 0.0;
	for(i = 0; i < trackset->count; i++) {
		for(j = 0; j < trackset->tracks[i].count; j++) {
			for(k = 0; k < trackset->tracks[i].tracksegments[j].count; k++) {
				p = &trackset->tracks[i].tracksegments[j].trackpoints[k];
				/* The point is in geographic lon/lat coordinates */
				geo_x = p->point.geo_lon;
				geo_y = p->point.geo_lat;
				/* Transfom it to screen projection */
				if(xform != NULL && !OCTTransform(xform, 1, &geo_x, &geo_y, NULL)) {
					continue;
				}
				if(first) {
					first = FALSE;
					rect->left = rect->right = geo_x;
					rect->top = rect->bottom = geo_y;
				}
				else {
					if(rect->left > geo_x)
						rect->left = geo_x;
					if(rect->right < geo_x)
						rect->right = geo_x;
					if(rect->top > geo_y)
						rect->top = geo_y;
					if(rect->bottom < geo_y)
						rect->bottom = geo_y;
				}
			}
		}
	}

	OCTDestroyCoordinateTransformation(xform);

	return !first;
}

static void
routeset_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
}

void
waypointset_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
	int i;
	int x1, y1;
	OGRSpatialReferenceH osrsSrc, osrsDst;
	OGRCoordinateTransformationH xform;
	struct WayPointSet *waypointset = (struct WayPointSet *)layer->data;

	osrsSrc = OSRNewSpatialReference(NULL);
	OSRSetWellKnownGeogCS(osrsSrc, "WGS84");
	osrsDst = OSRNewSpatialReference(target->WKT);
	xform = OCTNewCoordinateTransformation(osrsSrc, osrsDst);
	OSRDestroySpatialReference(osrsDst);
	OSRDestroySpatialReference(osrsSrc);

	free_tree(layer->priv);

	layer->priv = new_branch(0, target->height, 0, target->width);

	for(i = 0; i < waypointset->count; i++) {
		bool t1;
		t1 = point_calc_mapview_data(&waypointset->waypoints[i].point, target, xform, &x1, &y1);
				
		if(t1) {
			tree_add_waypoint(layer->priv,
				&waypointset->waypoints[i],
				x1, y1);
		}
	}

	layer->flags |= LAYER_IS_TREE_SEARCHABLE;

	OCTDestroyCoordinateTransformation(xform);
}

static void
trackset_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	// struct TrackSet *trackset = (struct TrackSet *)layer->data;
	tree_to_pixmap(layer->priv, rc->rt, rc->ct, rc->x, rc->y, rc->w, rc->h);
}

static void
routeset_render_layer(const struct Layer *layer, const struct RenderContext *rc) {};

static void
waypointset_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	// struct WayPointSet *waypointset = (struct WayPointSet *)layer->data;
	tree_to_pixmap(layer->priv, rc->rt, rc->ct, rc->x, rc->y, rc->w, rc->h);
}

static void
XXset_free_target_data(struct Layer *layer, const struct RenderTarget *target) {
	free_tree(layer->priv);
	layer->priv = NULL;
}

static const struct LayerOps trackset_layer_ops = {
	trackset_render_layer,
	trackset_calc_target_data,
	XXset_free_target_data,
};

void
trackset_init_layer(struct Layer *layer, enum LayerType type, struct TrackSet *trackset) {
	layer->type = type;
	layer->ops = &trackset_layer_ops;
	layer->data = trackset;
	layer->flags = (layer->data != NULL) ? LAYER_IS_VISIBLE : LAYER_FLAGS_NONE;
	layer->priv = NULL;
}

static const struct LayerOps routeset_layer_ops = {
	routeset_render_layer,
	routeset_calc_target_data,
	XXset_free_target_data,
};

void
routeset_init_layer(struct Layer *layer, enum LayerType type, struct RouteSet *routeset) {
	layer->type = type;
	layer->ops = &routeset_layer_ops;
	layer->data = routeset;
	layer->flags = (layer->data != NULL) ? LAYER_IS_VISIBLE : LAYER_FLAGS_NONE;
	layer->priv = NULL;
}

static const struct LayerOps waypointset_layer_ops = {
	waypointset_render_layer,
	waypointset_calc_target_data,
	XXset_free_target_data,
};

void
waypointset_init_layer(struct Layer *layer, enum LayerType type, struct WayPointSet *waypointset) {
	layer->type = type;
	layer->ops = &waypointset_layer_ops;
	layer->data = waypointset;
	layer->flags = (layer->data != NULL) ? LAYER_IS_VISIBLE : LAYER_FLAGS_NONE;
	layer->priv = NULL;
}
