/*
 * mapset_gdal.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"
#include <gdal.h>
#include <gdalwarper.h>
#include <ogr_api.h>
#include <cpl_conv.h>

extern void GDALRegister_gdal_pixbuf();
extern void GDALRegister_gdal_cairo();
extern GDALDatasetH GDALOpenPixbuf(GdkPixbuf *, GDALAccess);
extern GDALDatasetH GDALOpenPixbuf2(GdkPixbuf *, GDALAccess, int x, int y, int w, int h);
extern GDALDatasetH GDALOpenCairo(cairo_surface_t *, GDALAccess);

struct MapTargetdata {
	bool visible;
	struct {
		int left, right, top, bottom;	/* bounding rect in Target's pixels coordinates */
	} Bounds;
};

static void  proj_hack() {
	if(access("/usr/lib/libproj.so", R_OK|X_OK) &&	!access("/usr/lib/libproj.so.0", R_OK|X_OK)) {
		setenv("PROJSO", "/usr/lib/libproj.so.0", FALSE);
	}
}

void
GDAL_init_drivers() {
	proj_hack();
	GDALAllRegister();
	GDALRegister_gdal_cairo();
	GDALRegister_gdal_pixbuf();
}

#if 1
static bool
map_set_bounds(struct Map *map, struct MapTargetdata *data, const struct RenderTarget *target, OGRCoordinateTransformationH xform,
				bool *first, double map_x, double map_y) {
	double geo_x, geo_y;
	double screen_x, screen_y;

	pixel_to_geo_xy(map->GeoTransform, map_x, map_y, &geo_x, &geo_y);

	/* We should transfom it to screen projection but for now these are the same */
	if(xform != NULL && !OCTTransform(xform, 1, &geo_x, &geo_y, NULL)) {
		return FALSE;
	}

	if(!geo_to_pixel_xy(target->GeoTransform, geo_x, geo_y, &screen_x, &screen_y))
	 	return FALSE;

	if(*first) {
		data->Bounds.left = (int)floor(screen_x);
		data->Bounds.top =  (int)floor(screen_y);
		data->Bounds.right = (int)ceil(screen_x);
		data->Bounds.bottom = (int)ceil(screen_y);
	}
	else {
		data->Bounds.left =   MIN(data->Bounds.left, (int)floor(screen_x));
		data->Bounds.top =    MIN(data->Bounds.top, (int)floor(screen_y));
		data->Bounds.right =  MAX(data->Bounds.right, (int)ceil(screen_x));
		data->Bounds.bottom = MAX(data->Bounds.bottom, (int)ceil(screen_y));
	}
	*first = FALSE;

	data->visible = TRUE;
	return TRUE;
}

static void
mapset_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
	struct MapSet *mapset = (struct MapSet *)layer->data;
	OGRSpatialReferenceH osrsSrc, osrsDst;
	OGRCoordinateTransformationH xform = NULL;
	struct MapTargetdata *data;
	int i;

	if(mapset->WKT != NULL && target->WKT != NULL) {
		osrsSrc = OSRNewSpatialReference(mapset->WKT);
		osrsDst = OSRNewSpatialReference(target->WKT);
		xform = OCTNewCoordinateTransformation(osrsSrc, osrsDst);
		OSRDestroySpatialReference(osrsDst);
		OSRDestroySpatialReference(osrsSrc);
	}

	layer->priv = gmap_realloc(layer->priv, mapset->count * sizeof(struct MapTargetdata)); 
	data = (struct MapTargetdata *)layer->priv;

	for(i = 0; i < mapset->count; i++) {
		struct Map *map = &mapset->maps[i];
		bool b;
		bool first = TRUE;

		data[i].visible = FALSE;
		if(!map->visible)
			continue;

		/* 4 corners */
		b  = map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x, map->Rect.y);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w, map->Rect.y);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x, map->Rect.y + map->Rect.h);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w, map->Rect.y + map->Rect.h);

		/* Centers */
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x, map->Rect.y + map->Rect.h/2.0);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w, map->Rect.y + map->Rect.h/2.0);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w/2.0, map->Rect.y);
		b &= map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w/2.0, map->Rect.y + map->Rect.h);

		if(!b) {
			int j;

			map_set_bounds(map, &data[i], target, xform,  &first, map->Rect.x + map->Rect.w/2.0, map->Rect.y + map->Rect.h/2.0);

			for(j = 0; j < 100; j++) {
				/* pick 100 random points within the map... must hav a better way to do that */
				map_set_bounds(map, &data[i], target, xform,  &first,
					g_random_double_range(map->Rect.x, map->Rect.x+map->Rect.w),
					g_random_double_range(map->Rect.y, map->Rect.y+map->Rect.h));
			}
		}
	}

	if(xform != NULL)
		OCTDestroyCoordinateTransformation(xform);
}
#endif

#if 0
static void
mapset_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
	struct MapSet *mapset = (struct MapSet *)layer->data;
	struct MapTargetdata *data;
	int i;

	layer->priv = gmap_realloc(layer->priv, mapset->count * sizeof(struct MapTargetdata)); 
	data = (struct MapTargetdata *)layer->priv;


	for(i = 0; i < mapset->count; i++) {
		GDALDatasetH fakesrc;
		int width, height;
		double extents[4];
		double GeoTransform[6];
		void *hTransformArg;
		GdkPixbuf *fake;
		struct Map *map = &mapset->maps[i];
		fake = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, map->width, map->height);
		fakesrc = GDALOpenPixbuf(fake, GA_ReadOnly);
		GDALSetProjection(fakesrc, mapset->WKT);
		GDALSetGeoTransform(fakesrc, map->GeoTransform);
		hTransformArg = GDALCreateGenImgProjTransformer(fakesrc, mapset->WKT, NULL, target->WKT, FALSE, 0, 1 );
		if(hTransformArg == NULL) {
			g_error("hTransformArg == NULL");
			continue;
		}
		if(OGRERR_NONE != GDALSuggestedWarpOutput2(fakesrc, GDALGenImgProjTransform, hTransformArg, GeoTransform, &width, &height, extents, 0)) {
			g_error("GDALSuggestedWarpOutput2 failed");
			continue;
	
		}

		g_message("GDAL suggested: w=%d h=%d ext=%f %f %f %f)", width, height, extents[0], extents[1], extents[2], extents[3]);

		data[i].visible = TRUE;
		data[i].Bounds.left = extents[0];
		data[i].Bounds.right = extents[2];
		data[i].Bounds.top = extents[3];
		data[i].Bounds.bottom = extents[1];
		
	}
}
#endif

/* XXX set target's parameters from this mapset */
void
target_set_projection_and_scale_from_mapset(struct MapSet *mapset, struct RenderTarget *target) {
	gmap_free(target->WKT);
	target->WKT = gmap_strdup(mapset->WKT);

	target->left = mapset->left;
	target->right = mapset->right;
	target->top = mapset->top;
	target->bottom = mapset->bottom;

	/* set scale to mapset's preferred value */
	target_set_scale(target, mapset->preferred_scale);;
}

/* Call after changing the WKT for a mapview. This will set-up
a translator from the new CS to WGS84, and also set/clear
a flag telling is the coordinates should be shown in
degrees or not. */
void
mapview_changed_projection(struct MapView *mapview) {
	OGRSpatialReferenceH osrsSrc, osrsDst;
	/* The parameter that are calculated here are used when displaying coordinated on the
	   statusbar (updated by mouse_move */
	if(mapview->towgs84)
		OCTDestroyCoordinateTransformation((OGRCoordinateTransformationH)(mapview->towgs84));

	mapview->towgs84 = NULL;
	mapview->geographic_ref = FALSE;

	if(mapview->rt.WKT != NULL) {
		osrsSrc = OSRNewSpatialReference(mapview->rt.WKT);
		mapview->geographic_ref = OSRIsGeographic(osrsSrc);
		osrsDst = OSRNewSpatialReference(NULL);
		OSRSetWellKnownGeogCS(osrsDst, "WGS84");
		mapview->towgs84 = (void *)OCTNewCoordinateTransformation(osrsSrc, osrsDst);
		OSRDestroySpatialReference(osrsDst);
		OSRDestroySpatialReference(osrsSrc);
	}
}

/* XXX set mapview parameters from this mapset */
void
mapview_set_projection_and_scale_from_mapset(struct MapSet *mapset, struct MapView *mapview) {

	/* first set the target */
	target_set_projection_and_scale_from_mapset(mapset, &mapview->rt);

	gtk_layout_set_size(GTK_LAYOUT(mapview->layout), mapview->rt.width, mapview->rt.height);

	/* reset to this scale value when hitting '=' */
	mapview->preferred_scale = mapview->rt.scale;

	mapview_changed_projection(mapview);
}

void
map_cache(struct Map *map) {
	GError *err = NULL;

	if(!map->fullpath) {
		if(g_path_is_absolute(map->filename)) {
			map->fullpath = gmap_strdup(map->filename);
		}
		else {
			map->fullpath = g_build_filename(map->mapset->basedir, map->filename, NULL);
		}
	}

	// Open input file
	if(!map->cached) {
		map->cached = gdk_pixbuf_new_from_file(map->fullpath, &err);
		g_message("caching of \"%s\" %s", map->fullpath, map->cached ? "success" : err->message);
	}

	if(!map->cached) {
		map->visible = FALSE;
	}
}

void
map_uncache(struct Map *map) {
	if(map->cachedDS) {
		GDALClose(map->cachedDS);
		map->cachedDS = NULL;
		map->cachedDSNULL = NULL;
	}
	if(map->cached) {
		g_object_unref(map->cached);
		map->cached = NULL;
	}
}

static int
myProgressFunc(double dfComplete, const char *pszMessage, void *pProgressArg) {
	return TRUE; 	/* indicating process should continue */
}

static int
open_merge_map(struct Map *map, GDALDatasetH hDstDS) {
	GDALDatasetH  hSrcDS;
	GDALDatasetH  hSrcDSNULL;
	GDALWarpOptions *psWarpOptions;
	GDALWarpOperationH oOperation;

	map_cache(map);

	if(map->cached == NULL)
		return 1;	/* XXX Error.. must emit a message in map_cache */

	if(map->cachedDS != NULL) {
		hSrcDS = (GDALDatasetH)map->cachedDS;
		hSrcDSNULL = (GDALDatasetH)map->cachedDSNULL;
	}
	else {
		double GeoTransform[6];
		hSrcDS = GDALOpenPixbuf2(map->cached, GA_ReadOnly,
			map->Rect.x, map->Rect.y, map->Rect.w, map->Rect.h);

		GeoTransform[0] = map->GeoTransform[0] + map->Rect.x * map->GeoTransform[1] + map->Rect.y * map->GeoTransform[2];
		GeoTransform[1] = map->GeoTransform[1];
		GeoTransform[2] = map->GeoTransform[2];
		GeoTransform[3] = map->GeoTransform[3] + map->Rect.x * map->GeoTransform[4] + map->Rect.y * map->GeoTransform[5];
		GeoTransform[4] = map->GeoTransform[4];
		GeoTransform[5] = map->GeoTransform[5];

		GDALSetProjection(hSrcDS, map->mapset->WKT);
		GDALSetGeoTransform(hSrcDS, GeoTransform);

		hSrcDSNULL = hSrcDS;
		map->cachedDS = hSrcDS;
		map->cachedDSNULL = hSrcDS;

		if(is_unity_geotransform(GeoTransform)) {
			hSrcDSNULL = NULL;
			map->cachedDSNULL = NULL;
		}
	}

	/* Setup warp options.  */
	psWarpOptions = GDALCreateWarpOptions();

	psWarpOptions->hSrcDS = hSrcDS;
	psWarpOptions->hDstDS = hDstDS;

	psWarpOptions->nBandCount = 1;
	psWarpOptions->panSrcBands = (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
	psWarpOptions->panSrcBands[0] = 1;

	psWarpOptions->panDstBands = (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
	psWarpOptions->panDstBands[0] = 1;

	psWarpOptions->pfnProgress = (GDALProgressFunc)myProgressFunc; /* was GDALTermProgress;   */

	/* Establish reprojection transformer.  */
	psWarpOptions->pTransformerArg = 
		GDALCreateGenImgProjTransformer( hSrcDSNULL, GDALGetProjectionRef(hSrcDS), 
						 hDstDS, GDALGetProjectionRef(hDstDS), 
						 FALSE, 0.0, 1 );
	psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

	/* Initialize and execute the warp operation. */
	oOperation = GDALCreateWarpOperation(psWarpOptions);;

	if(oOperation == NULL)
		return 1;	/* XXX error! That should not happen! */

	GDALChunkAndWarpImage(oOperation, 0, 0,
				  GDALGetRasterXSize( hDstDS ), 
				  GDALGetRasterYSize( hDstDS ) );

	GDALDestroyWarpOperation(oOperation);

	GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
	GDALDestroyWarpOptions( psWarpOptions );

	return 0;
}

static bool
is_visible(struct MapTargetdata *data, int left, int right, int top, int bottom) {
	if(!data->visible) return FALSE;
	if(data->Bounds.top > bottom) return FALSE;
	if(data->Bounds.bottom < top) return FALSE;
	if(data->Bounds.left > right) return FALSE;
	if(data->Bounds.right < left) return FALSE;
	return TRUE;
}

static void
mapset_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	struct MapSet *mapset = (struct MapSet *)layer->data;
	GDALDatasetH  hDstDS;
	double GeoTransform[6];
	struct MapTargetdata *data;

	int i;

	cairo_surface_flush(rc->cs);

	hDstDS = GDALOpenCairo(rc->cs, GA_Update);

	GeoTransform[0] = rc->rt->GeoTransform[0] + rc->x * rc->rt->GeoTransform[1] + rc->y * rc->rt->GeoTransform[2];
	GeoTransform[1] = rc->rt->GeoTransform[1];
	GeoTransform[2] = rc->rt->GeoTransform[2];
	GeoTransform[3] = rc->rt->GeoTransform[3] + rc->x * rc->rt->GeoTransform[4] + rc->y * rc->rt->GeoTransform[5];
	GeoTransform[4] = rc->rt->GeoTransform[4];
	GeoTransform[5] = rc->rt->GeoTransform[5];

	GDALSetProjection(hDstDS, rc->rt->WKT);
	GDALSetGeoTransform(hDstDS, GeoTransform);

	data = (struct MapTargetdata *)layer->priv;

	for(i = 0; i < mapset->count; i++) {
		if(!mapset->maps[i].visible)
			continue;
		if(is_visible(&data[i], rc->x, rc->x+rc->w, rc->y, rc->y+rc->h)) {

/*
			g_message("%s: %d, %d, %d, %d", mapset->maps[i].filename,
				mapset->maps[i].mapview_data[mapview->index].Bounds.left,
				mapset->maps[i].mapview_data[mapview->index].Bounds.right,
				mapset->maps[i].mapview_data[mapview->index].Bounds.top,
				mapset->maps[i].mapview_data[mapview->index].Bounds.bottom);
*/

			open_merge_map(&mapset->maps[i], hDstDS);
		}
	}

	GDALClose( hDstDS );
	cairo_surface_mark_dirty(rc->cs);
}

static void
mapset_free_target_data(struct Layer *layer, const struct RenderTarget *target) {
	gmap_free(layer->priv);
	layer->priv = NULL;
}

static const
struct LayerOps mapset_layer_ops = {
	mapset_render_layer,
	mapset_calc_target_data,
	mapset_free_target_data,
};

void
mapset_init_layer(struct Layer *layer, enum LayerType type, struct MapSet *mapset) {
	layer->type = type;
	layer->ops = &mapset_layer_ops;
	layer->data = mapset;
	layer->flags = (layer->data != NULL) ? LAYER_IS_VISIBLE : LAYER_FLAGS_NONE;
}

void
mapview_center_map_region(struct MapView *mapview, double xx0, double xx1, double yy0, double yy1) {
	double scale;

	/* scale that fits the rectangle into the alloctted rectangle */
	scale = MAX(ABS(yy1-yy0)/mapview->allocation_height, ABS(xx1-xx0)/mapview->allocation_width);

	/* center to region */
	xx0 = (xx0 + xx1) / 2;
	yy0 = (yy0 + yy1) / 2;

	gtk_widget_hide(mapview->layout);
	mapview_set_scale(mapview, scale);
	mapview_goto_xy(mapview, xx0, yy0, mapview->allocation_width/2.0, mapview->allocation_height/2.0);
	gtk_widget_show(mapview->layout);
}
