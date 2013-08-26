/*
 * calibrate.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"
#include <ogr_api.h>
#include <cpl_conv.h>

/* Layers are hard-coded! */
#define CAL_LAYER_SOLID 0
#define CAL_LAYER_MAPSET 1
#define CAL_LAYER_SOLID_ALPHA 2
#define CAL_LAYER_AFFINEGRID 3
#define CAL_LAYER_CALIBRATE 4
#define CAL_LAYER_MAX 5

struct Calibration {
	struct Map *map;
	struct MapSet *mapset;
	struct Map *tmpmap;
	struct MapSet *tmpmapset;
	struct MapView *mapview;
	int current_calibration_point;
	int active;
	int mapno;
	bool usewgs84;
	void *towgs84;
	void *fromwgs84;

	GtkWidget *pointselect ;
	GtkWidget *map_x ;
	GtkWidget *map_y ;
	GtkWidget *geo_x ;
	GtkWidget *geo_y ;
	GtkWidget *isvalid ;
	GtkWidget *is_wgs84;
	GtkWidget *add_point_button ;
	GtkWidget *remove_point_button ;
	//GtkWidget *prev_mep ;
	GtkWidget *map_select_combo ;
	//GtkWidget *next_map ;
	GtkWidget *add_map_button ;
	GtkWidget *remove_map_button ;
	GtkWidget *save_mapset ;
	GtkWidget *calibrate_button;
	GtkWidget *rect_table;
	GtkWidget *rect_left;
	GtkWidget *rect_right;
	GtkWidget *rect_top;
	GtkWidget *rect_bottom;

	double GeoTransform[6];

	bool ignore_signalls;
};

static void set_calibration_map(struct Calibration *cal, int mapno);

static struct AffineGridData calibration_grid = {
	0x0000800000, 0x0080000000,	/* hcolor, vcolor */
	10, 10,			/* hspaces, vspaces */
	1,				/* line_width */
	NULL,				/* GeoTransform */
};

static void
calibration_point_changed_xy(struct Calibration *cal, double x, double y) {
	GdkRectangle rect;
	pixel_to_geo_xy(cal->tmpmap->GeoTransform, x, y, &x, &y);
	geo_to_pixel_xy(cal->mapview->rt.GeoTransform, x, y, &x, &y);
	//gtk_widget_queue_draw_area(cal->mapview->layout, (int)x-23, (int)y-23, 46, 46);
	rect.x = (int)x-23;
	rect.y = (int)y-23;
	rect.width = 46;
	rect.height = 46;
	gdk_window_invalidate_rect(GTK_LAYOUT(cal->mapview->layout)->bin_window, &rect, FALSE);
}

static void
calibration_point_changed(struct Calibration *cal, int n) {
	if(n < 0 || n >= cal->map->n_calibration_points)
		return;
	calibration_point_changed_xy(cal,
		cal->map->calibration_points[n].mapx,
		cal->map->calibration_points[n].mapy);
}

static void
calibrate_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	struct Calibration *cal = (struct Calibration *)layer->data;
	double xx, yy;
	int i;

	if(cal->map == NULL)
		return;

	cairo_set_source_rgba(rc->ct, 0, 0, 0, 0.3);
	cairo_set_fill_rule(rc->ct, CAIRO_FILL_RULE_EVEN_ODD);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, 0, 0, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_move_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->width, 0, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->width, cal->map->height, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, 0, cal->map->height, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, 0, 0, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	/*
	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Rect.x, cal->map->Rect.y, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_move_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Rect.x+cal->map->Rect.w, cal->map->Rect.y, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Rect.x+cal->map->Rect.w, cal->map->Rect.y+cal->map->Rect.h, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Rect.x, cal->map->Rect.y+cal->map->Rect.h, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);

	geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Rect.x, cal->map->Rect.y, &xx, &yy);
	geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
	cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);
	*/
	if(cal->map->CropPoints > 0) {
		geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Crop[0].X, cal->map->Crop[0].Y, &xx, &yy);
		geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
		cairo_move_to(rc->ct, xx-rc->x, yy-rc->y);
		int i;	
		for(i = cal->map->CropPoints; --i >= 0;) {
			geo_to_pixel_xy(cal->tmpmap->GeoTransform, cal->map->Crop[i].X, cal->map->Crop[i].Y, &xx, &yy);
			geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
			cairo_line_to(rc->ct, xx-rc->x, yy-rc->y);
		}
	}

	cairo_fill(rc->ct);

	for(i = 0; i < cal->map->n_calibration_points; i++) {
		/* paint_calibration_point(
			cal->mapvew.rt.GeoTransform,
			cal->map.GeoTransform;
			cal->map->calibration_points[i].mapx,
			cal->map->calibration_points[i].mapy,
			mp, x, y, x+w, y+h); */
		pixel_to_geo_xy(cal->tmpmap->GeoTransform,
			cal->map->calibration_points[i].mapx,
			cal->map->calibration_points[i].mapy,
			&xx, &yy);

		geo_to_pixel_xy(rc->rt->GeoTransform, xx, yy, &xx, &yy);
		xx -= rc->x;
		yy -= rc->y;

		/* draw a cross-hair */
		cairo_set_line_width(rc->ct, 1.0);
		cairo_set_source_rgb(rc->ct, 0.0, 0.0, 0.0);
		cairo_move_to(rc->ct, xx-20, yy);
		cairo_line_to(rc->ct, xx+20, yy);
		cairo_move_to(rc->ct, xx, yy-20);
		cairo_line_to(rc->ct, xx, yy+20);
		cairo_stroke(rc->ct);

		/* draw two circles */
		if(cal->map->calibration_points[i].valid)
			cairo_set_source_rgb(rc->ct, 0.0, 0.0, 1.0);
		else
			cairo_set_source_rgb(rc->ct, 1.0, 0.0, 0.0);

		if(i == cal->current_calibration_point)
			cairo_set_line_width(rc->ct, 3.0);
		else
			cairo_set_line_width(rc->ct, 1.0);

		cairo_arc(rc->ct, xx, yy, 10.0, 0.0, 2*M_PI);
		cairo_stroke(rc->ct);
		cairo_arc(rc->ct, xx, yy, 20.0, 0.0, 2*M_PI);
		cairo_stroke(rc->ct);
	}
}

static bool
Check_map_is_calibrated(struct Calibration *cal) {
	int i;
	calibration_grid.GeoTransform = NULL;
	cal->mapview->rt.layers[CAL_LAYER_AFFINEGRID].flags &= ~LAYER_IS_VISIBLE;
	if(!cal->map)
		return FALSE;
	if(is_unity_geotransform(cal->map->GeoTransform))
		return FALSE;
	for(i = 0; i < 6; i++)
		cal->GeoTransform[i] = cal->map->GeoTransform[i];
	calibration_grid.GeoTransform = cal->GeoTransform;
	// cal->mapview->rt.layers[CAL_LAYER_AFFINEGRID].flags |= LAYER_IS_VISIBLE;
	return TRUE;
}

static void
calibrate_calc_mapview_data(struct Layer *layer, const struct RenderTarget *target) {
}

static const struct LayerOps calibrate_layer_ops = {
	calibrate_render_layer,
	calibrate_calc_mapview_data,
	NULL,
};

static void
calibrate_init_layer(struct Layer *layer, enum LayerType type, struct Calibration *cal)  {
	layer->type = type;
	layer->ops = &calibrate_layer_ops;
	layer->flags |= LAYER_IS_VISIBLE;
	layer->data = cal;
}

static void
new_point_selected(struct Calibration *cal) {
	struct cpoint *cpoint;
	double xx, yy;
	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;

	cpoint = &cal->map->calibration_points[cal->current_calibration_point];

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->map_x), cpoint->mapx);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->map_y), cpoint->mapy);
	
	xx = cpoint->geox;
	yy = cpoint->geoy;
	if(cal->usewgs84)
		OCTTransform(cal->towgs84, 1, &xx, &yy, NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->geo_x), xx);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->geo_y), yy);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cal->isvalid), cpoint->valid);
}

static void
new_map_selected(struct Calibration *cal) {
	bool sensitive = (cal->map && cal->map->n_calibration_points > 0);
	cal->ignore_signalls = TRUE;
	gtk_widget_set_sensitive(cal->pointselect, sensitive);
	gtk_widget_set_sensitive(cal->map_x, sensitive);
	gtk_widget_set_sensitive(cal->map_y, sensitive);
	gtk_widget_set_sensitive(cal->geo_x, sensitive);
	gtk_widget_set_sensitive(cal->geo_y, sensitive);
	gtk_widget_set_sensitive(cal->isvalid, sensitive);
	gtk_widget_set_sensitive(cal->calibrate_button, sensitive);
	gtk_widget_set_sensitive(cal->remove_point_button, sensitive);
	gtk_widget_set_sensitive(cal->add_point_button, (cal->map != NULL));

	gtk_widget_set_sensitive(cal->rect_table, (cal->map != NULL));
	gtk_widget_set_sensitive(cal->map_select_combo, (cal->mapset->count > 0));

	if(cal->map) {
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->pointselect), 0.0, (double)(cal->map->n_calibration_points-1));
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->map_x), 0.0, (double)cal->map->width);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->map_y), 0.0, (double)cal->map->height);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_left), 0.0, (double)cal->map->width);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_right), 0.0, (double)cal->map->width);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_top), 0.0, (double)cal->map->height);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_bottom), 0.0, (double)cal->map->height);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->rect_left), (double)cal->map->Rect.x);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->rect_right), (double)cal->map->Rect.x+cal->map->Rect.w);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->rect_top), (double)cal->map->Rect.y);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->rect_bottom), (double)cal->map->Rect.y+cal->map->Rect.h);
	}
	else {
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->pointselect), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->map_x), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->map_y), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_left), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_right), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_top), 0.0, 0.0);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(cal->rect_bottom), 0.0, 0.0);
	}

	Check_map_is_calibrated(cal);

	if(cal->map)
		new_point_selected(cal);
	cal->ignore_signalls = FALSE;
}

static void
calibrate_select_or_add(struct MapView *mapview, int x, int y, int button, int mask, void *tool_data) {
	struct Calibration *cal = (struct Calibration *)tool_data;
	struct cpoint cp;
	int i;

	int point = -1;
	double distance, near_distance;

	if(!cal->map)
		return;

	/* find the nearest point */
	for(i = 0; i < cal->map->n_calibration_points; i++) {
		double xx, yy;
		pixel_to_geo_xy(cal->tmpmap->GeoTransform,
			cal->map->calibration_points[i].mapx,
			cal->map->calibration_points[i].mapy, &xx, &yy);
		geo_to_pixel_xy(cal->mapview->rt.GeoTransform, xx, yy, &xx, &yy);
		xx -= x;
		yy -= y;
		distance = hypot(xx, yy);
		if(point == -1 || distance < near_distance) {
			point = i;
			near_distance = distance;
		}
	}

	/* If a near point is not found, add a new point */
	if(point == -1 || near_distance > 20.0) {
		double x0, y0;
		pixel_to_geo_xy(mapview->rt.GeoTransform, (double)x, (double)y, &x0, &y0);
		if(x0 < 0)
			x0 = 0;
		if(x0 > cal->map->width)
			x0 = cal->map->width;
		if(y0 < 0)
			y0 = 0;
		if(y0 > cal->map->height)
			y0 = cal->map->height;
		cp.mapx = x0;
		cp.mapy = y0;
		pixel_to_geo_xy(cal->map->GeoTransform, cp.mapx, cp.mapy, &cp.geox, &cp.geoy);

		cp.valid = FALSE;
		point = map_add_cpoint(cal->map->mapset, cal->map, &cp);
	}

	if(point != cal->current_calibration_point) {
		/* force a refresh if something has changed */
		cal->ignore_signalls = TRUE;
		calibration_point_changed(cal, cal->current_calibration_point);
		cal->current_calibration_point = point;
		new_map_selected(cal);
		//gtk_widget_hide(mapview->layout);
		//gtk_widget_show(mapview->layout);
		//gtk_widget_queue_draw(mapview->layout);
		calibration_point_changed(cal, cal->current_calibration_point);
		cal->ignore_signalls = FALSE;
	}

	gtk_spin_button_set_range (GTK_SPIN_BUTTON(cal->pointselect), 0.0, (gdouble)(cal->map->n_calibration_points-1));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->pointselect), (gdouble)point);

  	gtk_grab_add (mapview->layout);

	cal->active = TRUE;
}

static void
calibrate_release(struct MapView *mapview, int x, int y, int button, int mask, void *tool_data) {
	struct Calibration *cal = (struct Calibration *)tool_data;
	cal->active = FALSE;
	cal->ignore_signalls = TRUE;
	new_point_selected(cal);
	cal->ignore_signalls = FALSE;
	gtk_grab_remove(mapview->layout);
}

static void
calibrate_move(struct MapView *mapview, int x, int y, void *tool_data) {
	struct Calibration *cal = (struct Calibration *)tool_data;
	double x0, y0;
	pixel_to_geo_xy(mapview->rt.GeoTransform, (double)x, (double)y, &x0, &y0);
	
	if(!cal->active)
		return;

	cal->ignore_signalls = TRUE;
	calibration_point_changed(cal, cal->current_calibration_point);

	if(x0 < 0)
		x0 = 0;
	if(x0 > cal->map->width)
		x0 = cal->map->width;
	if(y0 < 0)
		y0 = 0;
	if(y0 > cal->map->height)
		y0 = cal->map->height;

	cal->map->calibration_points[cal->current_calibration_point].mapx = x0;
	cal->map->calibration_points[cal->current_calibration_point].mapy = y0;
	calibration_point_changed(cal, cal->current_calibration_point);

	new_point_selected(cal);

	//gtk_widget_hide(mapview->layout);
	//gtk_widget_show(mapview->layout);
	//gtk_widget_queue_draw(mapview->layout);
	cal->ignore_signalls = FALSE;
}

struct Tool calibrate_tool = {
	"Calibrate", NULL, "Select calibration points", "<CTRL>P", NULL,
	calibrate_select_or_add,
	calibrate_release,
	calibrate_move,
	NULL, NULL,
};

static void
pointselect_value_changed(GtkWidget *widget, struct Calibration *cal) {
	int point;
	if(cal->ignore_signalls)
		return;

	point = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	g_message("pointselect_value_changed: %d", point);
	if(point != cal->current_calibration_point) {
		struct cpoint *cpoint;

		calibration_point_changed(cal, cal->current_calibration_point);
		calibration_point_changed(cal, point);
		cal->ignore_signalls = TRUE;
		cal->current_calibration_point = point;
		new_point_selected(cal);
		//gtk_widget_hide(cal->mapview->layout);
		cpoint = &cal->map->calibration_points[cal->current_calibration_point];
		mapview_point_to_view(cal->mapview, cpoint->mapx, cpoint->mapy);
		//gtk_widget_show(cal->mapview->layout);
		cal->ignore_signalls = FALSE;
	}
}

static void
point_map_xy_value_changed(GtkWidget *widget, struct Calibration *cal) {
	struct cpoint *cpoint;
	if(cal->ignore_signalls)
		return;

	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;

	g_message("point_map_xy_value_changed");

	calibration_point_changed(cal, cal->current_calibration_point);
	cpoint = &cal->map->calibration_points[cal->current_calibration_point];

	cpoint->mapx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->map_x));
	cpoint->mapy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->map_y));

	//gtk_widget_hide(cal->mapview->layout);
	//gtk_widget_show(cal->mapview->layout);
	//gtk_widget_queue_draw(cal->mapview->layout);
	calibration_point_changed(cal, cal->current_calibration_point);
}

static void
point_geo_xy_value_changed(GtkWidget *widget, struct Calibration *cal) {
	struct cpoint *cpoint;
	if(cal->ignore_signalls)
		return;

	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;

	g_message("point_geo_xy_value_changed");

	cpoint = &cal->map->calibration_points[cal->current_calibration_point];

	cpoint->geox = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->geo_x));
	cpoint->geoy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->geo_y));

	if(cal->usewgs84)
		OCTTransform(cal->fromwgs84, 1, &cpoint->geox, &cpoint->geoy, NULL);
}

static void
add_map_button_clicked(GtkWidget *widget, struct Calibration *cal) {
	GtkWidget *dialog;
	GtkFileFilter *filter;

	g_message("add_map_button_clicked");

	dialog = gtk_file_chooser_dialog_new ("Open Map Files",
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Image files");
	gtk_file_filter_add_pixbuf_formats(filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		char *basename;
		int next_mapno = cal->mapset->count;
		GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (dialog));
		while(filenames != NULL) {
			filename = (char *)filenames->data;
			basename = g_path_get_basename(filename);
			// g_message("filename = '%s'", filename);
			new_map(cal->mapset, filename);
			gtk_combo_box_append_text(GTK_COMBO_BOX(cal->map_select_combo), basename);
			g_free(basename);
			g_free(filename);
			filenames = g_slist_delete_link(filenames, filenames);
		}
		// open_file (filename);

		/* after 'new_map' pointers have changed! */
		if(cal->mapno >= 0)
			cal->map = &cal->mapset->maps[cal->mapno];
		if(cal->mapset->count > next_mapno) {
			set_calibration_map(cal, next_mapno);
			gtk_combo_box_set_active(GTK_COMBO_BOX(cal->map_select_combo), next_mapno);
			new_map_selected(cal);
		}
	}

	gtk_widget_destroy (dialog);

}

static void
remove_map_button_clicked(GtkWidget *widget, struct Calibration *cal) {
	g_message("remove_map_button_clicked");
}

static void
bounding_rect_changed(GtkWidget *widget, struct Calibration *cal) {
	int a, b;
	if(!cal->ignore_signalls) {
		a = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->rect_left));
		b = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->rect_right));
		cal->map->Rect.x = MIN(a, b);
		cal->map->Rect.w = MAX(a, b) - cal->map->Rect.x;
		a = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->rect_top));
		b = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(cal->rect_bottom));
		cal->map->Rect.y = MIN(a, b);
		cal->map->Rect.h = MAX(a, b) - cal->map->Rect.y;

		map_set_croprect(cal->map->mapset, cal->map, cal->map->Rect.x, cal->map->Rect.y, cal->map->Rect.w, cal->map->Rect.h);

		gtk_widget_queue_draw(cal->mapview->layout);
	}
}

static void
save_mapset_clicked(GtkWidget *widget, struct Calibration *cal) {
	GtkWidget *dialog;
	GtkFileFilter *filter;
	dialog = gtk_file_chooser_dialog_new ("Save MapSet Files",
					NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "MapSet (XML) files");
	gtk_file_filter_add_pattern(filter, "*.xml");
	gtk_file_filter_add_pattern(filter, "*.XML");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
	if(cal->mapset->filename != NULL)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), cal->mapset->filename);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));;
		g_message("filename = %s", filename);
		// open_file (filename);

#if 1
		mapset_recalculate_bounds(cal->mapset);
		mapset_recalculate_preferred_scale(cal->mapset);
		mapset_save(cal->mapset, filename);
#endif
		g_free(filename);
	}

	gtk_widget_destroy (dialog);

}

static void
set_calibration_map(struct Calibration *cal, int mapno) {
	cal->mapno = mapno;

	/* discard previous data */
	map_uncache(cal->tmpmap);
	gmap_free(cal->tmpmap->filename);
	cal->tmpmap->filename = NULL;
	gmap_free(cal->tmpmap->fullpath);
	cal->tmpmap->fullpath = NULL;

	cal->map = NULL;
	cal->tmpmap->width = 0;
	cal->tmpmap->height = 0;
	cal->tmpmap->bpp = 0;
	cal->tmpmap->Rect.x = 0;
	cal->tmpmap->Rect.y = 0;
	cal->tmpmap->Rect.w = 0;
	cal->tmpmap->Rect.h = 0;
	cal->tmpmap->visible = FALSE;
	cal->current_calibration_point = -1;

	if(mapno >= 0 && mapno < cal->mapset->count)
		cal->map = &cal->mapset->maps[mapno];

	if(cal->map) {
		cal->tmpmap->filename = gmap_strdup(cal->map->filename);
		map_cache(cal->tmpmap);
	}

	if(cal->map && cal->tmpmap->cached) {
		/* If it's the first time we open this map, get some data from
		   the cached file */
		if(cal->map->width <= 0)
			cal->map->width = gdk_pixbuf_get_width(cal->tmpmap->cached);
		if(cal->map->height <= 0)
			cal->map->height = gdk_pixbuf_get_height(cal->tmpmap->cached);
		if(cal->map->bpp <= 0)
			cal->map->bpp = gdk_pixbuf_get_bits_per_sample(cal->tmpmap->cached) *
					gdk_pixbuf_get_n_channels(cal->tmpmap->cached);

		if(cal->map->Rect.w <= 0)
			cal->map->Rect.w = cal->map->width;
		if(cal->map->Rect.h <= 0)
			cal->map->Rect.h = cal->map->height;

		cal->tmpmap->width = cal->map->width;
		cal->tmpmap->height = cal->map->height;
		cal->tmpmap->bpp = cal->map->bpp;

		/* we want to see all of it ! */
		cal->tmpmap->Rect.x = 0;
		cal->tmpmap->Rect.y = 0;
		cal->tmpmap->Rect.w = cal->tmpmap->width;
		cal->tmpmap->Rect.h = cal->tmpmap->height;
		cal->tmpmap->visible = TRUE;
	}
	if(cal->map != NULL)
		cal->mapview->rt.layers[1].flags |= LAYER_IS_VISIBLE;
	else
		cal->mapview->rt.layers[1].flags &= ~LAYER_IS_VISIBLE;

	set_unity_geotransform(cal->tmpmap->GeoTransform);

	cal->tmpmapset->left =   0;
	cal->tmpmapset->top =    0;
	cal->tmpmapset->right =  cal->tmpmap->width;
	cal->tmpmapset->bottom = cal->tmpmap->height;

	mapview_set_projection_and_scale_from_mapset(cal->tmpmapset,  cal->mapview);
}

static void
add_calibration_point(GtkWidget *widget, struct Calibration *cal) {
	struct cpoint cpoint;
	if(!cal->map)
		return;
	mapview_get_center(cal->mapview, &cpoint.mapx, &cpoint.mapy);
	pixel_to_geo_xy(cal->map->GeoTransform, cpoint.mapx, cpoint.mapy, &cpoint.geox, &cpoint.geoy);
	cpoint.valid = FALSE;

	calibration_point_changed(cal, cal->current_calibration_point);

	cal->current_calibration_point = map_add_cpoint(cal->mapset, cal->map, &cpoint);

	gtk_spin_button_set_range (GTK_SPIN_BUTTON(cal->pointselect), 0.0, (gdouble)(cal->map->n_calibration_points-1));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cal->pointselect), cal->current_calibration_point);

	new_map_selected(cal);		/* this will call new_point_selected() */
	calibration_point_changed(cal, cal->current_calibration_point);
}

static void
remove_calibration_point(GtkWidget *widget, struct Calibration *cal) {
	double x, y;
	if(!cal->map)
		return;
	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;

	x = cal->map->calibration_points[cal->current_calibration_point].mapx;
	y = cal->map->calibration_points[cal->current_calibration_point].mapy;

	map_remove_cpoint(cal->mapset, cal->map, cal->current_calibration_point);
	cal->current_calibration_point = -1;

	new_map_selected(cal);		/* this will call new_point_selected() */
	calibration_point_changed_xy(cal, x, y);
}

static void
calibrate_button_clicked(GtkWidget *widget, struct Calibration *cal) {
	if(!cal->map)
		return;
	if(!map_calc_GeoReference(cal->map))
		set_unity_geotransform(cal->map->GeoTransform);
	if(Check_map_is_calibrated(cal))
		cal->map->visible = TRUE;;

	/* To force a refresh */
	gtk_widget_queue_draw(cal->mapview->layout);
}

static void
isvalid_toggled(GtkToggleButton *togglebutton, struct Calibration *cal) {
	struct cpoint *cpoint;

	if(!cal->map)
		return;

	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;

	cpoint = &cal->map->calibration_points[cal->current_calibration_point];
	cpoint->valid = gtk_toggle_button_get_active(togglebutton);

	mapview_point_to_view(cal->mapview, cpoint->mapx, cpoint->mapy);
	calibration_point_changed(cal, cal->current_calibration_point);
}

static void
iswgs84_toggled(GtkToggleButton *togglebutton, struct Calibration *cal) {
	//struct cpoint *cpoint;

	if(!cal->map)
		return;

	if(cal->current_calibration_point < 0 || cal->current_calibration_point >= cal->map->n_calibration_points)
		return;
	/* Change display of geo-coords to/from wgs84 */
	cal->usewgs84 = gtk_toggle_button_get_active(togglebutton);

	cal->ignore_signalls = TRUE;
	/* update values in spin-buttons */

	new_point_selected(cal);	/* this will update spinbuttons */

	cal->ignore_signalls = FALSE;
}


static void
map_select_combo_changed(GtkWidget *widget, struct Calibration *cal) {
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(cal->map_select_combo));
	// g_message("map_select_combo_changed: %d '%s'", i, cal->mapset->maps[i].filename);
	set_calibration_map(cal, i);
	new_map_selected(cal);		/* this will call new_point_selected() */

	gtk_widget_hide(cal->mapview->layout);
	mapview_goto_xy(cal->mapview, 0, 0, 0, 0);
	gtk_widget_show(cal->mapview->layout);
}

static void
calibration_tool(struct MainWindow *mainwindow, struct MapView *mapview, struct MapSet *mapset, struct Calibration *cal) {
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *widget;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	int i;
	
	vbox = gtk_vbox_new(FALSE, 1);

	frame = gtk_frame_new("Calibrate map");

	vbox2 = gtk_vbox_new(FALSE, 1);

	table = gtk_table_new(FALSE, 2, 5);

	widget = gtk_label_new("Point #");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 0, 1);
	widget = gtk_spin_button_new_with_range(0, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 0, 1);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(widget), TRUE);
	cal->pointselect = widget;

	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(pointselect_value_changed), cal);

	widget = gtk_label_new("Map X");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 1, 2);
	widget = gtk_spin_button_new_with_range(0, 0, 0.01);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 1, 2);
	cal->map_x = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(point_map_xy_value_changed), cal);

	widget = gtk_label_new("Map Y");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 2, 3);
	widget = gtk_spin_button_new_with_range(0, 0, 0.01);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 2, 3);
	cal->map_y = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(point_map_xy_value_changed), cal);

	widget = gtk_label_new("Geo X");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 3, 4);
	widget = gtk_spin_button_new(NULL, 0.01, 6);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), (double)-100000000, (double)100000000);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 3, 4);
	cal->geo_x = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(point_geo_xy_value_changed), cal);

	widget = gtk_label_new("Geo Y");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 4, 5);
	widget = gtk_spin_button_new(NULL, 0.01, 6);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(widget), (double)-100000000, (double)100000000);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 4, 5);
	cal->geo_y = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(point_geo_xy_value_changed), cal);

	gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 1);
	widget = gtk_check_button_new_with_label("Valid");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->isvalid = widget;
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(isvalid_toggled), cal);

	widget = gtk_check_button_new_with_label("WGS84");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->is_wgs84 = widget;
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(iswgs84_toggled), cal);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 1);
	widget = gtk_label_new("Calibration point");
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	widget = gtk_button_new_with_label("Add");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->add_point_button = widget;
	g_signal_connect(G_OBJECT(widget), "clicked",  G_CALLBACK(add_calibration_point), cal);

	widget = gtk_button_new_with_label("Remove");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->remove_point_button = widget;
	g_signal_connect(G_OBJECT(widget), "clicked",  G_CALLBACK(remove_calibration_point), cal);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, TRUE, 0);

	widget = gtk_button_new_with_label("Calibrate!");
	gtk_box_pack_start (GTK_BOX (vbox2), widget, FALSE, TRUE, 0);
	cal->calibrate_button = widget;
	g_signal_connect(G_OBJECT(widget), "clicked",  G_CALLBACK(calibrate_button_clicked), cal);

	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new("Bounding Rect");

	table = gtk_table_new(FALSE, 2, 4);
	cal->rect_table = table;

	widget = gtk_label_new("Left");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 0, 1);
	widget = gtk_spin_button_new_with_range(0, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 0, 1);
	cal->rect_left = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(bounding_rect_changed), cal);

	widget = gtk_label_new("Right");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 1, 2);
	widget = gtk_spin_button_new_with_range(0, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 1, 2);
	cal->rect_right = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(bounding_rect_changed), cal);

	widget = gtk_label_new("Top");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 2, 3);
	widget = gtk_spin_button_new_with_range(0, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 2, 3);
	cal->rect_top = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(bounding_rect_changed), cal);

	widget = gtk_label_new("Botom");
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 3, 4);
	widget = gtk_spin_button_new_with_range(0, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 3, 4);
	cal->rect_bottom = widget;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(bounding_rect_changed), cal);

	gtk_container_add(GTK_CONTAINER(frame), table);

	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new("Select map");

	vbox2 = gtk_vbox_new(FALSE, 1);

	hbox = gtk_hbox_new(FALSE, 1);
#if 0
	widget = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(widget), gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE));
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->prev_mep = widget;
#endif

	widget = gtk_combo_box_new_text();
	for(i = 0; i < mapset->count; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), mapset->maps[i].filename);
	}
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	cal->map_select_combo = widget;
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(map_select_combo_changed), cal);

#if 0
	widget = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(widget), gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE));
	gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->next_map = widget;
#endif
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 1);
	widget = gtk_label_new("Map");
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	widget = gtk_button_new_with_label("Add (multi)");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	cal->add_map_button = widget;
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(add_map_button_clicked), cal);

	widget = gtk_button_new_with_label("Remove");
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, TRUE, 0);
	cal->remove_map_button = widget;
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remove_map_button_clicked), cal);

	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 1);

	widget = gtk_button_new_with_label("Save Changes to mapset");
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, FALSE, 0);
	cal->save_mapset = widget;
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(save_mapset_clicked), cal);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

	gtk_widget_show_all(vbox);

	gtk_paned_pack2(GTK_PANED(mapview->hpane), vbox, FALSE, TRUE);
}

static void
init_transformations(struct Calibration *cal) {
	if(cal->towgs84 != NULL)
		OCTDestroyCoordinateTransformation((OGRCoordinateTransformationH)(cal->towgs84));
	if(cal->fromwgs84 != NULL)
		OCTDestroyCoordinateTransformation((OGRCoordinateTransformationH)(cal->fromwgs84));
	cal->towgs84 = cal->fromwgs84 = NULL;
		
	if(cal->mapset->WKT != NULL) {
		OGRSpatialReferenceH osrsSrc, osrsDst;
		osrsSrc = OSRNewSpatialReference(cal->mapset->WKT);
		osrsDst = OSRNewSpatialReference(NULL);
		OSRSetWellKnownGeogCS(osrsDst, "WGS84");
		cal->towgs84 = (void *)OCTNewCoordinateTransformation(osrsSrc, osrsDst);
		cal->fromwgs84 = (void *)OCTNewCoordinateTransformation(osrsDst, osrsSrc);
		OSRDestroySpatialReference(osrsDst);
		OSRDestroySpatialReference(osrsSrc);
	}
}

struct MapView *
calibrate_map(struct MainWindow *mainwindow, struct MapSet *mapset, int mapno) {

	struct Calibration *cal = (struct Calibration *)gmap_malloc(sizeof(struct Calibration));

	cal->mapset = mapset;
	cal->map = NULL;
	cal->towgs84 = cal->fromwgs84 = NULL;

	cal->tmpmapset = new_mapset();
	cal->tmpmapset->basedir = gmap_strdup(mapset->basedir);
	cal->tmpmapset->preferred_scale = 1.0;

	cal->tmpmap = new_map(cal->tmpmapset, NULL);
	cal->usewgs84 = FALSE;

	/* Now create a MapView */
	cal->mapview = create_map_window(mainwindow);
	mapview_set_name(cal->mapview, "Map Calibration");

	/* Attach the dialog on the right */
	calibration_tool(mainwindow, cal->mapview, cal->mapset, cal);

	init_transformations(cal);

	cal->active = FALSE;
	cal->current_calibration_point = -1;
	cal->ignore_signalls = FALSE;

	/* register the calibration tool and make is default */
	mapwindow_register_tool(cal->mapview, &calibrate_tool, cal, TRUE, TRUE, TRUE);

	cal->mapview->rt.n_layers = CAL_LAYER_MAX;
	cal->mapview->rt.layers = (struct Layer *)gmap_malloc0(cal->mapview->rt.n_layers * sizeof(struct Layer));

	/* Create layers */
	solid_fill_init_layer(&cal->mapview->rt.layers[CAL_LAYER_SOLID], LAYER_SOLID, 1.0, 0.5, 1.0, 0.5);
	mapset_init_layer(&cal->mapview->rt.layers[CAL_LAYER_MAPSET], LAYER_MAPSET, cal->tmpmapset);
	cal->mapview->rt.layers[CAL_LAYER_MAPSET].flags = LAYER_FLAGS_NONE;
	solid_fill_init_layer(&cal->mapview->rt.layers[CAL_LAYER_SOLID_ALPHA], LAYER_SOLID_ALPHA, 0.5, 1.0, 1.0, 1.0);
	cal->mapview->rt.layers[CAL_LAYER_SOLID_ALPHA].flags = LAYER_FLAGS_NONE;
	affine_grid_init_layer(&cal->mapview->rt.layers[CAL_LAYER_AFFINEGRID], LAYER_AFFINEGRID, &calibration_grid);
	cal->mapview->rt.layers[CAL_LAYER_AFFINEGRID].flags = LAYER_FLAGS_NONE;
	calibrate_init_layer(&cal->mapview->rt.layers[CAL_LAYER_CALIBRATE], LAYER_CALIBRATE, cal);

	mapset_set_projection(cal->tmpmapset, NULL);
	// set_calibration_map(cal, mapno);
	set_calibration_map(cal, -1);

	if(mapno >= 0) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(cal->map_select_combo), mapno);
	}

	new_map_selected(cal);		/* this will call new_point_selected() */

	return cal->mapview;
}
