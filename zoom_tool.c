#include "gmap.h"

#include <gdk/gdk.h>

static void
zoom_tool_do_zoom_in(struct MapView *mapview) {
	double xx0, yy0, xx1, yy1;
	/* find points in geo space */
	if(mapview->x0 == mapview->x1 && mapview->y0 == mapview->y1) {
	//	pixel_to_geo_xy(mapview->GeoTransform, (double)mapview->x0, (double)mapview->y0, &xx0, &yy0);
	//	mapview_goto_xy(mapview, xx0, yy0, mapview->allocation_width/2.0, mapview->allocation_height/2.0);
		return;
	}

	pixel_to_geo_xy(mapview->rt.GeoTransform, (double)mapview->x0, (double)mapview->y0, &xx0, &yy0);
	pixel_to_geo_xy(mapview->rt.GeoTransform, (double)mapview->x1, (double)mapview->y1, &xx1, &yy1);

	mapview_center_map_region(mapview, xx0, xx1, yy0, yy1);
}

static void
zoom_tool_do_zoom_out(struct MapView *mapview) {
	double scale;
	double x0, y0, xx0, yy0, xx1, yy1;
	/* find points in geo space */
	if(mapview->x0 == mapview->x1 && mapview->y0 == mapview->y1) {
	//	pixel_to_geo_xy(mapview->GeoTransform, (double)mapview->x0, (double)mapview->y0, &xx0, &yy0);
	//	mapview_goto_xy(mapview, xx0, yy0, mapview->allocation_width/2.0, mapview->allocation_height/2.0);
		return;
	}

	/* Find the visible region */
	x0 = gtk_adjustment_get_value(mapview->hadjustment);
	y0 = gtk_adjustment_get_value(mapview->vadjustment);
	pixel_to_geo_xy(mapview->rt.GeoTransform, x0, y0, &xx0, &yy0);
	pixel_to_geo_xy(mapview->rt.GeoTransform, x0+mapview->allocation_width, y0+mapview->allocation_height, &xx1, &yy1);

	/* scale that fits that region into the rectangle */
	scale = MAX(ABS(yy1-yy0)/ABS(mapview->y0-mapview->y1), ABS(xx1-xx0)/ABS(mapview->x0-mapview->x1));

	/* center to region */
	xx0 = (xx0 + xx1) / 2;
	yy0 = (yy0 + yy1) / 2;

	gtk_widget_hide(mapview->layout);
	mapview_set_scale(mapview, scale);
	mapview_goto_xy(mapview, xx0, yy0, mapview->allocation_width/2.0, mapview->allocation_height/2.0);
	gtk_widget_show(mapview->layout);
}

static void
zoom_tool_draw_rect(struct MapView *mapview) {
	int x = MIN(mapview->x0, mapview->x1);
	int y = MIN(mapview->y0, mapview->y1);
	int w = ABS(mapview->x1-mapview->x0);
	int h = ABS(mapview->y1-mapview->y0);

	gdk_draw_rectangle(GTK_LAYOUT(mapview->layout)->bin_window, mapview->gc, FALSE,
		x, y, w, h);

	// g_message("zoom_tool_draw_rect: %d %d %d %d", x, y, w, h);
}

static void
zoom_tool_button_press(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata) {
	GdkGCValues values;

	if(button != 1)
		return;

	//g_message("zoom_tool_button_press %d %d", x, y);
	if(mapview->gc == NULL) {
		values.function = GDK_INVERT;
		values.line_width = 1;
		values.line_style = GDK_LINE_SOLID;

		mapview->gc = gdk_gc_new_with_values(GTK_LAYOUT(mapview->layout)->bin_window, &values,
			GDK_GC_FUNCTION | GDK_GC_LINE_WIDTH | GDK_GC_LINE_STYLE);
	}

	mapview->x0 = x;
	mapview->x1 = x;
	mapview->y0 = y;
	mapview->y1 = y;
	mapview->on_display = FALSE;
	mapview->active = TRUE;
}

static void
zoom_in_tool_button_release(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata) {
	if(!mapview->active)
		return;

	//g_message("zoom_in_tool_button_release %d %d", x, y);

	if(mapview->on_display)
		zoom_tool_draw_rect(mapview);
	mapview->on_display = FALSE;;

	zoom_tool_do_zoom_in(mapview);

	mapview->active = FALSE;
}

static void
zoom_out_tool_button_release(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata) {
	if(!mapview->active)
		return;

	if(mapview->on_display)
		zoom_tool_draw_rect(mapview);
	mapview->on_display = FALSE;;

	zoom_tool_do_zoom_out(mapview);

	mapview->active = FALSE;
}

static void
zoom_tool_mouse_move(struct MapView *mapview, int x, int y, void *tooldata) {
	if(!mapview->active)
		return;

	//g_message("zoom_tool_mouse_move %d %d", x, y);

	if(mapview->on_display)
		zoom_tool_draw_rect(mapview);

	mapview->x1 = x;
	mapview->y1 = y;
	zoom_tool_draw_rect(mapview);
	mapview->on_display = TRUE;
}

struct Tool zoom_in_tool = {
	"ZoomInTool", GTK_STOCK_ZOOM_IN, "Zoom In", "<CTRL>plus", "Zoom-in on region selected by the mouse",
	zoom_tool_button_press,
	zoom_in_tool_button_release,
	zoom_tool_mouse_move,
	NULL, NULL,
};

struct Tool zoom_out_tool = {
	"ZoomOutTool", GTK_STOCK_ZOOM_OUT, "Zoom Out", "<CTRL>minus", "Zoom-out to region selected by the mouse",
	zoom_tool_button_press,
	zoom_out_tool_button_release,
	zoom_tool_mouse_move,
	NULL, NULL,
};
