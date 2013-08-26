/*
 * mapwindow.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"

void
scroll_to(struct MapView *mapview, double x, double y) {
	if(x < 0)
		x = 0;
	if(x > mapview->rt.width - mapview->allocation_width)
		x = mapview->rt.width - mapview->allocation_width;

	if(y < 0)
		y = 0;
	if(y > mapview->rt.height - mapview->allocation_height)
		y = mapview->rt.height - mapview->allocation_height;

	gtk_adjustment_set_value(mapview->hadjustment, x);
	gtk_adjustment_set_value(mapview->vadjustment, y);
}

static void
hand_drag(struct MapView *mapview, double x, double y) {
	printf("hand_drag: dx = %f; dy = %f\n", mapview->hand_x - x, mapview->hand_y - y);

	double nx = gtk_adjustment_get_value(mapview->hadjustment) + mapview->hand_x - x;
	double ny = gtk_adjustment_get_value(mapview->vadjustment) + mapview->hand_y - y;

	scroll_to(mapview, nx, ny);

	mapview->hand_x = x;
	mapview->hand_y = y;
}

void
destroy_near_objects_window(struct MapView *mapview) {
	if(mapview->near_objects_window != NULL)
		gtk_widget_destroy(mapview->near_objects_window);
	mapview->near_objects_window = NULL;
}

gboolean
mapwindow_show_near_objects(struct MapView *mapview) {
	GtkWidget *label;
	GtkWidget *window;
	char *str;

	//g_message("idle_proc!");
	mapview->show_near_objects_proc = 0;

	destroy_near_objects_window(mapview);

	str = get_near_object(mapview, mapview->last_x, mapview->last_y);

	if(str != NULL) {
		//PangoAttrList *p = pango_attr_list_new();
		int x, y;
		GdkColor color;

		gdk_window_get_pointer(NULL, &x, &y, NULL);

		label = gtk_label_new(str);

		//gtk_label_set_attributes(GTK_LABEL(label), p);
		gtk_misc_set_padding (GTK_MISC(label), 5, 5);
		window = gtk_window_new(GTK_WINDOW_POPUP);
		//gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
		gtk_window_move(GTK_WINDOW(window), x+5, y-5);

		color.red = 65535;
		color.green = 65535;
		color.blue = 40000;
		gdk_colormap_alloc_color(gtk_widget_get_colormap(window) , &color, FALSE, TRUE);
		gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);

		mapview->near_objects_window = window;

		gtk_container_add(GTK_CONTAINER(window), label);
		gtk_widget_show_all(window);
	}

	return FALSE;
}

static gboolean
mouse_move(GtkWidget *widget, GdkEventMotion *event, struct MapView *v) {
	gdouble _x , _y;
	GdkModifierType state;
	double geo_x, geo_y;
	char s[256];

	destroy_near_objects_window(v);

	if(v->scrolling) {
		hand_drag(v, event->x_root, event->y_root);
		return TRUE;
	}

	_x = event->x;
	_y = event->y;

	if(v->current_tool != NULL && v->current_tool->tool->MouseMove != NULL)
		v->current_tool->tool->MouseMove(v, (int)_x, (int)_y, v->current_tool->tooldata);

	pixel_to_geo_xy(v->rt.GeoTransform, _x, _y, &geo_x, &geo_y);
	if(v->geographic_ref) {
		snprintf(s, sizeof(s), "%10.6f%s%c %10.6f%s%c",
			fabs(geo_x), UTF8[UTF8_DEGREES], geo_x>=0?'E':'W',
			fabs(geo_y), UTF8[UTF8_DEGREES], geo_y>=0?'N':'S');
	}
	else {
		geo_x = floor(geo_x + 0.5);
		geo_y = floor(geo_y + 0.5);

		snprintf(s, sizeof(s), "%12.0f %12.0f", geo_x, geo_y);
	}

	gtk_label_set_text(GTK_LABEL(v->ViewXY), s);

	pixel_to_geo_xy(v->rt.GeoTransform, _x, _y, &geo_x, &geo_y);
	if(v->towgs84 != NULL &&
			OCTTransform(v->towgs84, 1, &geo_x, &geo_y, NULL)) {

		snprintf(s, sizeof(s), "%10.6f%s%c %10.6f%s%c",
			fabs(geo_x), UTF8[UTF8_DEGREES], geo_x>=0?'E':'W',
			fabs(geo_y), UTF8[UTF8_DEGREES], geo_y>=0?'N':'S');
		gtk_label_set_text(GTK_LABEL(v->OtherXY), s);
	}
	else {
		gtk_label_set_text(GTK_LABEL(v->OtherXY), "Unknown");
	}

	if(v->show_near_objects_proc != 0)
		g_source_remove (v->show_near_objects_proc);
	v->show_near_objects_proc = 0;

	v->last_x = (int)_x;
	v->last_y = (int)_y;
	v->show_near_objects_proc = g_timeout_add(300, (GSourceFunc)mapwindow_show_near_objects, v);

	return TRUE;
}

static gboolean
mouse_leave(GtkWidget *widget, GdkEventCrossing *event, struct MapView *v) {

	if(v->show_near_objects_proc != 0)
		g_source_remove (v->show_near_objects_proc);
	v->show_near_objects_proc = 0;
	destroy_near_objects_window(v);
	return FALSE;
}


void
mapview_goto_xy(struct MapView *mapview,
	double geo_x, double geo_y,
	double dpy_x, double dpy_y) {

	double win_x, win_y;

	if(!geo_to_pixel_xy(mapview->rt.GeoTransform, geo_x, geo_y, &win_x, &win_y))
		return;

	scroll_to(mapview, win_x - dpy_x, win_y - dpy_y);
}

void
mapview_point_to_view(struct MapView *mapview,
		double geo_x, double geo_y) {
	double win_x, win_y;
	double x = gtk_adjustment_get_value(mapview->hadjustment);
	double y = gtk_adjustment_get_value(mapview->vadjustment);

	if(!geo_to_pixel_xy(mapview->rt.GeoTransform, geo_x, geo_y, &win_x, &win_y))
		return;

	if(win_x >= x && win_x < x + mapview->allocation_width &&
	   win_y >= y && win_y < y + mapview->allocation_height)
		return;

	scroll_to(mapview, win_x-mapview->allocation_width/2, win_y-mapview->allocation_height/2);
}

void
mapview_get_center(struct MapView *mapview, double *geo_x, double *geo_y) {
	double win_x, win_y;

	win_x = gtk_adjustment_get_value(mapview->hadjustment) + mapview->allocation_width / 2;
	win_y = gtk_adjustment_get_value(mapview->vadjustment) + mapview->allocation_height /2;
	pixel_to_geo_xy(mapview->rt.GeoTransform, win_x, win_y, geo_x, geo_y);
}

static void
map_window_zoom_in(GtkAction *action, struct MapView *mapview)
{
	double zoom_scale = mapview->rt.scale / 1.5;

	double dx = mapview->allocation_width / 2.0;
	double dy = mapview->allocation_height / 2.0;
	double x =  dx + gtk_adjustment_get_value(mapview->hadjustment);
	double y =  dy + gtk_adjustment_get_value(mapview->vadjustment);
	double geo_x, geo_y;
	pixel_to_geo_xy(mapview->rt.GeoTransform, x, y, &geo_x, &geo_y);

	gtk_widget_hide(mapview->layout);
	mapview_set_scale(mapview, zoom_scale);
	mapview_goto_xy(mapview, geo_x, geo_y, dx, dy);
	gtk_widget_show(mapview->layout);
}

static void
map_window_zoom_out(GtkAction *action, struct MapView *mapview)
{
	double zoom_scale = mapview->rt.scale * 1.5;

	double dx = mapview->allocation_width / 2.0;
	double dy = mapview->allocation_height / 2.0;
	double x =  dx + gtk_adjustment_get_value(mapview->hadjustment);
	double y =  dy + gtk_adjustment_get_value(mapview->vadjustment);
	double geo_x, geo_y;
	pixel_to_geo_xy(mapview->rt.GeoTransform, x, y, &geo_x, &geo_y);

	gtk_widget_hide(mapview->layout);
	mapview_set_scale(mapview, zoom_scale);
	mapview_goto_xy(mapview, geo_x, geo_y, dx, dy);
	gtk_widget_show(mapview->layout);
}

static void
map_window_zoom_100(GtkAction *action, struct MapView *mapview)
{
	double zoom_scale = mapview->preferred_scale;

	double dx = mapview->allocation_width / 2.0;
	double dy = mapview->allocation_height / 2.0;
	double x =  dx + gtk_adjustment_get_value(mapview->hadjustment);
	double y =  dy + gtk_adjustment_get_value(mapview->vadjustment);
	double geo_x, geo_y;
	pixel_to_geo_xy(mapview->rt.GeoTransform, x, y, &geo_x, &geo_y);

	gtk_widget_hide(mapview->layout);
	mapview_set_scale(mapview, zoom_scale);
	mapview_goto_xy(mapview, geo_x, geo_y, dx, dy);
	gtk_widget_show(mapview->layout);
}

void
target_free_data(struct RenderTarget *target) {
	int i;
	gmap_free(target->WKT);
	for(i = 0; i < target->n_layers; i++) {
		/* must free layer private data */
		if(target->layers[i].ops->free_target_data)
			(*target->layers[i].ops->free_target_data)(&target->layers[i], target);
	}
	gmap_free(target->layers);
}

static void
map_window_close_window(GtkAction *action, struct MapView *mapview)
{
	gtk_widget_destroy(GTK_WIDGET(mapview->window)); /* XXX */
	target_free_data(&mapview->rt);
	gmap_free(mapview);
}

struct Layer *
target_add_layer(struct RenderTarget *target) {
	struct Layer *layer;
	target->layers = (struct Layer *)gmap_realloc(target->layers, (1+target->n_layers)*sizeof(struct Layer));
	layer = &target->layers[target->n_layers];
	target->n_layers++;
	layer->type = LAYER_NONE;
	layer->ops = NULL;
	layer->flags = LAYER_FLAGS_NONE;
	layer->data = NULL;
	layer->priv = NULL;
	return layer;
};

static void
add_layers_from_gpx_file(struct MapView *mapview, char *filename) {
	struct TrackSet *trackset;
	struct RouteSet *routeset;
	struct WayPointSet *waypointset;
	struct Layer *layer;

	if(load_from_gpx(filename, &trackset, &routeset, &waypointset)) {
		if(trackset != NULL) {
			layer = target_add_layer(&mapview->rt);
			trackset_init_layer(layer, LAYER_TRACKSET, trackset);
			(*layer->ops->calc_target_data)(layer, &mapview->rt);
		}
		if(routeset != NULL) {
			layer = target_add_layer(&mapview->rt);
			routeset_init_layer(layer, LAYER_ROUTESET, routeset);
			(*layer->ops->calc_target_data)(layer, &mapview->rt);
		}
		if(waypointset != NULL) {
			layer = target_add_layer(&mapview->rt);
			waypointset_init_layer(layer, LAYER_WAYPOINTSET, waypointset);
			(*layer->ops->calc_target_data)(layer, &mapview->rt);
		}
		struct GeoRect rect;
		if(trackset != NULL && trackset_calc_extents(trackset, &mapview->rt, &rect))
			mapview_center_map_region(mapview, rect.left, rect.right, rect.top, rect.bottom);
		gtk_widget_queue_draw(mapview->layout);
	}
	else {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mapview->window),
						0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Could not open %s", filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

static void
map_window_open_gpx(GtkAction *action, struct MapView *mapview)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;

	g_message("open_gpx_action");

	dialog = gtk_file_chooser_dialog_new ("Open GPX File",
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "GPX files");
	gtk_file_filter_add_pattern(filter, "*.gpx");
	gtk_file_filter_add_pattern(filter, "*.GPX");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));;
		g_message("filename = %s", filename);
		add_layers_from_gpx_file(mapview, filename);
		g_free(filename);
	}
	gtk_widget_destroy (dialog);
}


static gboolean
mouse_button_press(GtkWidget *widget, GdkEventButton *event, struct MapView *mapview) {
	mapview->hand_x = event->x_root;
	mapview->hand_y = event->y_root;

	g_message ("mouse_button_press %d %g %g", event->button, event->x_root, event->y_root);

//	gdouble rx,ry;
//	if(gdk_event_get_root_coords((GdkEvent *)event, &rx, &ry))
//		g_message ("mouse_button_press root %g %g", rx, ry);


	if(event->button == 1) {
		//zoom_in(mapview, event->x, event->y);
		if(mapview->current_tool != NULL && mapview->current_tool->tool->ButtonPress != NULL)
			mapview->current_tool->tool->ButtonPress(mapview, (int)event->x, (int)event->y, event->button, event->state, mapview->current_tool->tooldata);
	}
	else if(event->button == 2) {
		GdkCursor *cursor;

		mapview->scrolling = TRUE;
  		gtk_grab_add (mapview->layout);
		cursor = gdk_cursor_new(GDK_HAND2);
		gdk_window_set_cursor (GTK_LAYOUT(mapview->layout)->bin_window, cursor);
		gdk_cursor_destroy(cursor);
	}
	else if(event->button == 3) {
		//zoom_out(mapview, event->x, event->y);
		mapview->x0 = (int)event->x;
		mapview->y0 = (int)event->y;
		gtk_menu_popup (GTK_MENU(mapview->popup), NULL, NULL, NULL, NULL,
				event->button, event->time);
	}

	return FALSE;
}

static gboolean
mouse_button_release(GtkWidget *widget, GdkEventButton *event, struct MapView *mapview) {
	g_message ("mouse_button_release %d %g %g", event->button, event->x_root, event->y_root);
//	gdouble rx,ry;
//	if(gdk_event_get_root_coords((GdkEvent *)event, &rx, &ry))
//		g_message ("mouse_button_release root %g %g", rx, ry);

	if(mapview->scrolling) {
		mapview->scrolling = FALSE;
		gtk_grab_remove(mapview->layout);
		gdk_window_set_cursor (GTK_LAYOUT(mapview->layout)->bin_window, NULL);
	}
	else
		if(mapview->current_tool != NULL && mapview->current_tool->tool->ButtonRelease != NULL)
			mapview->current_tool->tool->ButtonRelease(mapview, (int)event->x, (int)event->y, event->button, event->state, mapview->current_tool->tooldata);

	return FALSE;
}

static gboolean
size_allocation_changed(GtkWidget *widget, GtkAllocation *allocation, struct MapView *mapview)  {
	GtkWidget *sb;
	/* g_message("size_allocation_changed: %d %d %d %d",
		allocation->x, allocation->y, allocation->width, allocation->height); */

	mapview->allocation_width = allocation->width;
	mapview->allocation_height = allocation->height;

	sb = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(mapview->scrolledwindow));
	gtk_range_set_increments(GTK_RANGE(sb), 5, allocation->width);
	sb = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(mapview->scrolledwindow));
	gtk_range_set_increments(GTK_RANGE(sb), 5, allocation->height);

	return FALSE;
}

/* Redraw the screen from the backing pixmap */
static gboolean
expose_event(GtkWidget *widget, GdkEventExpose *event, struct MapView *v)
{
	int i;
	struct RenderContext rc;
	cairo_t *tempx;

	cairo_surface_t *temp_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, event->area.width, event->area.height);
	cairo_t *temp = cairo_create(temp_surface);

	rc.rt = &v->rt;
	rc.cs = temp_surface;
	rc.ct = temp;
	rc.x = event->area.x;
	rc.y = event->area.y;
	rc.w = event->area.width;
	rc.h = event->area.height;

	for(i = 0; i < v->rt.n_layers; i++) {
		if(!(v->rt.layers[i].flags & LAYER_IS_VISIBLE))
			continue;

		(*v->rt.layers[i].ops->render_layer)(&v->rt.layers[i], &rc);
	}

	tempx = gdk_cairo_create(GTK_LAYOUT(widget)->bin_window);
	cairo_set_source_surface(tempx, temp_surface, event->area.x, event->area.y);
	cairo_paint(tempx);

	cairo_destroy(tempx);
	cairo_destroy(temp);
	cairo_surface_destroy(temp_surface);

	/* g_message ("expose_event %d x=%d, y=%d w=%d h=%d\n",
				counter++,
				event->area.x, event->area.y,
				event->area.width, event->area.height); */
	return TRUE;
}

void
target_set_scale(struct RenderTarget *target, double scale) {
	int i;
	double hscale, vscale;
	double sinrot, cosrot;

	/* XXX must be fixed to allow rotated target */
	/* To prevent view window from growing to a nngative size.
	   Even for a world map, 31 bits give a fair resolution of
	   about 2 cm/pixel. */

#define MAXWINDOWSIZE ((double)0x7FFFFFFF)
	scale = MAX(scale, fabs(target->right-target->left)/MAXWINDOWSIZE);
	scale = MAX(scale, fabs(target->bottom-target->top)/MAXWINDOWSIZE);
	// g_message("mapview_set_zoom: zoom_scale=%f", zoom_scale);

	target->scale = scale;

	sinrot = sin(target->rotation * M_PI/180);
	cosrot = cos(target->rotation * M_PI/180);

	hscale = (target->left <= target->right) ? scale : -scale;
	vscale = (target->top <= target->bottom) ? scale : -scale;
	vscale *= (target->x_resulution / target->y_resulution);

	target->GeoTransform[0] = target->left;
	target->GeoTransform[1] = hscale * cosrot;
	target->GeoTransform[2] = vscale * sinrot;
	target->GeoTransform[3] = target->top;
	target->GeoTransform[4] = hscale * -sinrot;
	target->GeoTransform[5] = vscale * cosrot;

	/* Set the size for the viewport */
	target->width = (int)ceil((target->right-target->left)/target->GeoTransform[1]);
	target->height =  (int)ceil((target->bottom-target->top)/target->GeoTransform[5]);

	for(i = 0; i < target->n_layers; i++) {
		if(!(target->layers[i].flags & LAYER_IS_VISIBLE))
			continue;

		(*target->layers[i].ops->calc_target_data)(&target->layers[i], target);
	}
}

void
mapview_set_scale(struct MapView *mapview, double scale) {
	char str[100]; int l;
	target_set_scale(&mapview->rt, scale);

	// g_message("Set the layout size to %d X %d", mapview->width, mapview->height);
	gtk_layout_set_size(GTK_LAYOUT(mapview->layout), mapview->rt.width, mapview->rt.height);
	l = snprintf(str, sizeof(str), "Scale 1/%f",
		mapview->rt.scale * (mapview->rt.x_resulution * 1000 / 25.4));
	if(mapview->rt.rotation != 0.0)
		snprintf(str+l, sizeof(str)-l, " rotation %.0f", mapview->rt.rotation);
	gtk_label_set_text(GTK_LABEL(mapview->MapXY), str);
}

static void
tool_select_callback(GtkRadioAction *action, GtkRadioAction *current, struct MapView *mapview) {
	mapview->current_tool_num = gtk_radio_action_get_current_value(action);

	if(mapview->current_tool_num < 0 || mapview->current_tool_num >= mapview->n_tools) {
		g_message("tool_select_callback: invalid tool selected %d", mapview->current_tool_num);
		mapview->current_tool_num = 0;
		// return;
	}

	if(mapview->current_tool != NULL && mapview->current_tool->tool->UnSelect != NULL)
		mapview->current_tool->tool->UnSelect(mapview, mapview->current_tool->tooldata);

	mapview->current_tool = &mapview->tools[mapview->current_tool_num];

	if(mapview->current_tool->tool->Select != NULL)
		mapview->current_tool->tool->Select(mapview, mapview->current_tool->tooldata);

	gtk_label_set_text(GTK_LABEL(mapview->ToolName), mapview->current_tool->tool->label);
	g_message("changed tool NEW=%d", mapview->current_tool_num);
}

static void
map_window_print(GtkAction *action, struct MapView *mapview)
{
	g_message("print");
	do_print(mapview);
}

static void
map_window_page_setup(GtkAction *action, struct MapView *mapview)
{
	g_message("page setup");
	do_page_setup(mapview);
}

static void
map_window_set_projection(GtkAction *action, struct MapView *mapview)
{
	g_message("set projection");
	do_set_projection(mapview);
}

static void
map_window_copy_coords_map(GtkAction *action, struct MapView *mapview)
{
	char str[128];
	double geo_x, geo_y;
	g_message("Copy coorvdinates");
	pixel_to_geo_xy(mapview->rt.GeoTransform, mapview->x0, mapview->y0, &geo_x, &geo_y);
	if(mapview->geographic_ref) {
		snprintf(str, sizeof(str), "%1.6f%s%c %1.6f%s%c",
			fabs(geo_x), UTF8[UTF8_DEGREES], geo_x>=0?'E':'W',
			fabs(geo_y), UTF8[UTF8_DEGREES], geo_y>=0?'N':'S');
	}
	else {
		geo_x = floor(geo_x + 0.5);
		geo_y = floor(geo_y + 0.5);

		snprintf(str, sizeof(str), "%1.0f/%1.0f", geo_x, geo_y);
	}
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, str, strlen(str));
}

static void
map_window_copy_coords_wgs84dec(GtkAction *action, struct MapView *mapview)
{
	char str[128];
	double geo_x, geo_y;
	g_message("Copy coorvdinates");
	pixel_to_geo_xy(mapview->rt.GeoTransform, mapview->x0, mapview->y0, &geo_x, &geo_y);
	if(mapview->towgs84 != NULL &&
			OCTTransform(mapview->towgs84, 1, &geo_x, &geo_y, NULL)) {

		snprintf(str, sizeof(str), "%10.6f%s%c %10.6f%s%c",
			fabs(geo_x), UTF8[UTF8_DEGREES], geo_x>=0?'E':'W',
			fabs(geo_y), UTF8[UTF8_DEGREES], geo_y>=0?'N':'S');
	}
	else {
		snprintf(str, sizeof(str), "Unknown");
	}
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, str, strlen(str));
}

static void
map_window_copy_coords_wgs84dms(GtkAction *action, struct MapView *mapview)
{
	char str[128];
	double geo_x, geo_y;
	g_message("Copy coorvdinates");
	pixel_to_geo_xy(mapview->rt.GeoTransform, mapview->x0, mapview->y0, &geo_x, &geo_y);
	if(mapview->towgs84 != NULL &&
			OCTTransform(mapview->towgs84, 1, &geo_x, &geo_y, NULL)) {

		int geo_x_s, geo_x_deg, geo_x_min, geo_y_s, geo_y_deg, geo_y_min;

		geo_x_s = (geo_x >= 0); geo_x = fabs(geo_x);
		geo_x_deg = (int)floor(geo_x); geo_x = 60*(geo_x - geo_x_deg);
		geo_x_min = (int)floor(geo_x); geo_x = 60*(geo_x - geo_x_min);
		geo_y_s = (geo_y >= 0); geo_y = fabs(geo_y);
		geo_y_deg = (int)floor(geo_y); geo_y = 60*(geo_y - geo_y_deg);
		geo_y_min = (int)floor(geo_y); geo_y = 60*(geo_y - geo_y_min);

		snprintf(str, sizeof(str), "%d%s%02d%s%02.2f%s%c %d%s%02d%s%02.2f%s%c",
			geo_x_deg, UTF8[UTF8_DEGREES], geo_x_min, UTF8[UTF8_MINUTES], geo_x, UTF8[UTF8_SECONDS], geo_x_s?'E':'W',
			geo_y_deg, UTF8[UTF8_DEGREES], geo_y_min, UTF8[UTF8_MINUTES], geo_y, UTF8[UTF8_SECONDS], geo_y_s?'N':'S');
	}
	else {
		snprintf(str, sizeof(str), "Unknown");
	}
	GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, str, strlen(str));
}

static GtkActionEntry ui_entries[] = {
// { "ContextMenu", NULL,		"Menu", },
  { "FileMenu",			NULL,			"_File", },
  { "OpenGPX",			GTK_STOCK_OPEN,		NULL,				NULL,	NULL,  G_CALLBACK(map_window_open_gpx) },
  { "Print",			GTK_STOCK_PRINT,	NULL,				NULL,	NULL,  G_CALLBACK(map_window_print) },
  { "PageSetup",		NULL,			"Page Setup...",		NULL,	NULL,  G_CALLBACK(map_window_page_setup) },
  { "Close",			GTK_STOCK_CLOSE,	NULL,				NULL,	NULL, G_CALLBACK(map_window_close_window) },
  { "HelpMenu",			NULL,			"_Help", },
  { "ViewMenu",			NULL,			"_View", },
  { "ZoomIn",			GTK_STOCK_ZOOM_IN, 	NULL,				"plus",	NULL,  G_CALLBACK(map_window_zoom_in) },
  { "ZoomOut",			GTK_STOCK_ZOOM_OUT,	NULL,				"minus",NULL,  G_CALLBACK(map_window_zoom_out) },
  { "SetProj",			NULL,       		"Set Projection",		NULL,   NULL,  G_CALLBACK(map_window_set_projection) },
  { "Zoom100",			GTK_STOCK_ZOOM_100,	NULL,				"equal",NULL,  G_CALLBACK(map_window_zoom_100) },
  { "ToolMenu",			NULL,			"_Tools", },
  { "CopyCoords",		NULL,			"Copy", },
  { "CopyCoordsMap",		NULL,			"117705/1045743",		NULL,	NULL,  G_CALLBACK(map_window_copy_coords_map) },
  { "CopyCoordsWGS84Dec",	NULL,			"34.671243°E  31.000776°N",	NULL,	NULL,  G_CALLBACK(map_window_copy_coords_wgs84dec) },
  { "CopyCoordsWGS84DMS",	NULL,			"34°40′16.47″E 31°00′3.07″N",	NULL,	NULL,  G_CALLBACK(map_window_copy_coords_wgs84dms) },
};
static guint n_ui_entries = G_N_ELEMENTS (ui_entries);

/* These actions are not specific to this window */
static GtkActionEntry ui_global_entries[] = {
  { "About",	   GTK_STOCK_ABOUT,	NULL,	NULL,	NULL, G_CALLBACK(show_about_dialog) },
};
static guint n_ui_global_entries = G_N_ELEMENTS (ui_global_entries);
#if 0
/* std Tools Radio items */
static const GtkRadioActionEntry std_tool_radio_entries[] = {
  { "ZoomInTool",  GTK_STOCK_ZOOM_IN,	"Zoom-_In Tool",  NULL, "Zoom-in on region selected by the mouse", 0 },
  { "ZoomOutTool", GTK_STOCK_ZOOM_OUT,	"Zoom-_Out Tool", NULL, "Zoom-out to region selected by the mouse", 1 },
};
static guint n_std_tool_radio_entries = G_N_ELEMENTS (std_tool_radio_entries);
#endif

static const gchar *ui_info =
"<ui>"
"  <menubar action='map-window-main_menu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='OpenGPX' />"
"      <separator />"
"      <menuitem action='PageSetup'  />"
"      <menuitem action='Print'  />"
"      <separator />"
"      <menuitem action='Close'  />"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ZoomIn'  />"
"      <menuitem action='ZoomOut' />"
"      <separator/>"
"      <menuitem action='SetProj'/>"
"    </menu>"
"    <menu action='ToolMenu'>"
"      <placeholder name='ToolsRadio'>"
//"      <menuitem action='ZoomInTool'  />"
//"      <menuitem action='ZoomOutTool' />"
"      </placeholder>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'  />"
"    </menu>"
"  </menubar>"
"  <popup name='map-window-context-menu'>"
"    <menuitem action='Zoom100'  />"
"    <menuitem action='ZoomIn'  />"
"    <menuitem action='ZoomOut' />"
"    <placeholder name='PopupToolsAddons'>"
"    </placeholder>"
"    <separator/>"
"    <placeholder name='PopupToolsRadio'>"
"    </placeholder>"
"  </popup>"
//"  <menu name='copy-submenu'>"
//"    <menuitem action='CopyCoordsMap'>"
//"    <menuitem action='CopyCoordsWGS84Dec'>"
//"    <menuitem action='CopyCoordsWGS84DMS'>"
//"  </menu>"
"</ui>";

/* UI paths */
#define MAIN_MENUBAR_PATH			"/map-window-main_menu"
#define TOOL_MENU_TOOLS_PATH			MAIN_MENUBAR_PATH "/ToolMenu/ToolsRadio"

#define MAIN_CONTEXT_MENU_PATH			"/map-window-context-menu"
#define CONTEXT_MENU_TOOLS_PATH			MAIN_CONTEXT_MENU_PATH "/PopupToolsRadio"
#define CONTEXT_MENU_TOOLS_ADDONS_PATH		MAIN_CONTEXT_MENU_PATH "/PopupToolsAddons"
#define CONTEXT_MENU_TOOLS_COPYCOORDS_PATH	CONTEXT_MENU_TOOLS_ADDONS_PATH "/CopyCoords"

void
mapwindow_register_tool(struct MapView *mapview,
		struct Tool *tool,
		void *tooldata,
		bool add_to_tools_menu,
		bool add_to_context_menu,
		bool make_it_current_tool) {

	GtkRadioAction *action;
	guint mergeid;
	int toolnum;

	toolnum = mapview->n_tools;
	mapview->tools = (struct ToolItem *)gmap_realloc(mapview->tools, (1+mapview->n_tools)*sizeof(struct ToolItem));
	mapview->tools[toolnum].tool = tool;
	mapview->tools[toolnum].tooldata = tooldata;
	mapview->n_tools ++;

	mapview->current_tool = mapview->current_tool_num < 0 ? NULL :
			&mapview->tools[mapview->current_tool_num];	/* refresh pointer after malloc! */

	action = gtk_radio_action_new(tool->name, tool->label, tool->tooltip, tool->stock_id, toolnum);
	if(mapview->tool_action_group == NULL)
		g_signal_connect(G_OBJECT(action), "changed", G_CALLBACK(tool_select_callback), mapview);

	gtk_radio_action_set_group(action, mapview->tool_action_group);
	mapview->tool_action_group = gtk_radio_action_get_group (action);

	gtk_action_group_add_action_with_accel(mapview->actions, GTK_ACTION(action), tool->accelerator);

	mergeid = gtk_ui_manager_new_merge_id (mapview->ui);
	if(add_to_tools_menu)
		gtk_ui_manager_add_ui(mapview->ui, mergeid, TOOL_MENU_TOOLS_PATH, tool->name, tool->name, GTK_UI_MANAGER_MENUITEM, FALSE);

	if(add_to_context_menu)
		gtk_ui_manager_add_ui(mapview->ui, mergeid, CONTEXT_MENU_TOOLS_PATH, tool->name, tool->name, GTK_UI_MANAGER_MENUITEM, FALSE);

	gtk_ui_manager_ensure_update(mapview->ui);

	if(make_it_current_tool)
		gtk_radio_action_set_current_value(action, toolnum);
}

static void
register_standard_tools(struct MapView *mapview) {
	extern struct Tool zoom_in_tool;
	extern struct Tool zoom_out_tool;

	mapwindow_register_tool(mapview, &zoom_in_tool, mapview, TRUE, FALSE, TRUE);
	mapwindow_register_tool(mapview, &zoom_out_tool, mapview, TRUE, FALSE, FALSE);
}

/*
void cb1(GtkWidget *w, struct MapView *v) {
	g_message("cb1");

	GtkWidget *m;

	m = gtk_ui_manager_get_widget(v->ui, CONTEXT_MENU_TOOLS_ADDONS_PATH "/CopyCoords/CopyCoordsMap");
	if(m)
		gtk_menu_item_set_label(GTK_MENU_ITEM(m), "replced label...");
}
*/

void
mapview_register_copy_coord_tool(struct MapView *mapview) {
	guint mergeid;
	mergeid = gtk_ui_manager_new_merge_id (mapview->ui);
	gtk_ui_manager_add_ui(mapview->ui, mergeid, CONTEXT_MENU_TOOLS_ADDONS_PATH, "CopyCoords",  "CopyCoords",GTK_UI_MANAGER_MENU,     FALSE);
	gtk_ui_manager_add_ui(mapview->ui, mergeid, CONTEXT_MENU_TOOLS_COPYCOORDS_PATH, "CopyCoordsMap",	  "CopyCoordsMap",      GTK_UI_MANAGER_MENUITEM, FALSE);
	gtk_ui_manager_add_ui(mapview->ui, mergeid, CONTEXT_MENU_TOOLS_COPYCOORDS_PATH, "CopyCoordsWGS84Dec", "CopyCoordsWGS84Dec", GTK_UI_MANAGER_MENUITEM, FALSE);
	gtk_ui_manager_add_ui(mapview->ui, mergeid, CONTEXT_MENU_TOOLS_COPYCOORDS_PATH, "CopyCoordsWGS84DMS", "CopyCoordsWGS84DMS", GTK_UI_MANAGER_MENUITEM, FALSE);

	gtk_ui_manager_ensure_update(mapview->ui);

/*
	GtkWidget *w = gtk_ui_manager_get_widget(mapview->ui, CONTEXT_MENU_TOOLS_ADDONS_PATH "/CopyCoords");
	if(w) {
		g_signal_connect(G_OBJECT(w), "activate", G_CALLBACK (cb1), mapview);
	}
	else
		g_message("no widget found");
*/
}

void
mapview_set_name(struct MapView *mapview, char *name) {
	gmap_free(mapview->name);	/* de-allocate old-name */
	mapview->name = gmap_strdup(name);
	gtk_window_set_title(GTK_WINDOW(mapview->window), mapview->name);
}

void before_scrolling(GtkAdjustment *a, struct MapView *mapview) {
	destroy_near_objects_window(mapview);
}

void
get_screen_resolution(struct MapView *v) {
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(v->window));
	double xr = gdk_screen_get_width(screen) * 25.4 / gdk_screen_get_width_mm(screen);
	double yr = gdk_screen_get_height(screen) * 25.4 / gdk_screen_get_height_mm(screen);

	fprintf(stderr, "res: x: %f y: %f\n", xr, yr);

        v->rt.x_resulution = xr;
        v->rt.y_resulution = yr;
}

struct MapView *
create_map_window(struct MainWindow *mainwindow) {
	struct MapView *v;
	GError *error = NULL;
	GtkWidget *vbox;

	v = (struct MapView *)gmap_malloc(sizeof(struct MapView));

	v->mainwindow = mainwindow;

	v->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// g_signal_connect(G_OBJECT(v->window), "destroy", G_CALLBACK (gtk_main_quit), v);
	gtk_window_set_title(GTK_WINDOW(v->window), "Gmap map display");
	// gtk_widget_set_size_request (GTK_WIDGET(v->window), 640, 480);
	gtk_window_set_default_size(GTK_WINDOW(v->window), 800, 600);

	v->rt.WKT = NULL;
	v->geographic_ref = FALSE;
	v->cache = NULL;
	v->name = NULL;
	v->scrolling = FALSE;
	v->current_tool = NULL;
	v->current_tool_num = -1;
	v->n_tools = 0;
	v->tools = NULL;
	v->tool_action_group = NULL;
	v->rt.n_layers = 0;
	v->rt.layers = NULL;
	/* init fields used by tools */
	v->on_display = FALSE;
	v->active = FALSE;
	v->gc = NULL;
	v->towgs84 = NULL;
	v->preferred_scale = 1;
	v->rt.scale = v->preferred_scale;
	v->rt.rotation = 0;

	get_screen_resolution(v);

	v->set_projection = NULL;

	v->near_objects_window = NULL;
	v->show_near_objects_proc = 0;

	/* Create actions (and popup_menu) */
	v->actions = gtk_action_group_new ("Actions");
	gtk_action_group_add_actions (v->actions, ui_entries, n_ui_entries, v);
	gtk_action_group_add_actions (v->actions, ui_global_entries, n_ui_global_entries, v->mainwindow);

	/* UI Manager */
	v->ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (v->ui, v->actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (v->window),
				  gtk_ui_manager_get_accel_group (v->ui));

	if (!gtk_ui_manager_add_ui_from_string (v->ui, ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	v->popup = gtk_ui_manager_get_widget (v->ui, MAIN_CONTEXT_MENU_PATH);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER(vbox), 1);

	v->menu = gtk_ui_manager_get_widget(v->ui, MAIN_MENUBAR_PATH);
	if(v->menu != NULL)
		gtk_box_pack_start(GTK_BOX(vbox), v->menu, FALSE, FALSE, 0);

	v->hpane = gtk_hpaned_new();

	/* scrolled window */
	v->scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(v->scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	v->layout = gtk_layout_new(NULL, NULL);
	gtk_widget_set_app_paintable(v->layout, TRUE);			/* ??? */
	//gtk_widget_set_double_buffered (v->layout, FALSE);		/* ??? */
	gtk_widget_set_redraw_on_allocate(v->layout, FALSE);		/* ??? */
	gtk_container_add(GTK_CONTAINER(v->scrolledwindow), v->layout);

	v->hadjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(v->scrolledwindow));
	v->vadjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(v->scrolledwindow));

	g_signal_connect(G_OBJECT(v->vadjustment), "value-changed", G_CALLBACK(before_scrolling), v);
	g_signal_connect(G_OBJECT(v->hadjustment), "value-changed", G_CALLBACK(before_scrolling), v);

	gtk_widget_set_events(v->layout,
				GDK_POINTER_MOTION_MASK |
				GDK_POINTER_MOTION_HINT_MASK |
				GDK_BUTTON_PRESS_MASK |
				GDK_BUTTON_RELEASE_MASK|
				GDK_LEAVE_NOTIFY_MASK);

	gtk_widget_show(v->layout);
	gtk_widget_show(v->scrolledwindow);

	g_signal_connect(G_OBJECT(v->layout), "expose_event", G_CALLBACK(expose_event), v);
	g_signal_connect(G_OBJECT(v->layout), "motion_notify_event", G_CALLBACK(mouse_move), v);
	g_signal_connect(G_OBJECT(v->layout), "leave_notify_event", G_CALLBACK(mouse_leave), v);
	g_signal_connect(G_OBJECT(v->layout), "button_press_event", G_CALLBACK(mouse_button_press), v);
	g_signal_connect(G_OBJECT(v->layout), "button_release_event", G_CALLBACK(mouse_button_release), v);
	g_signal_connect(G_OBJECT(v->layout), "size-allocate", G_CALLBACK(size_allocation_changed), v);

	gtk_paned_pack1(GTK_PANED(v->hpane), v->scrolledwindow, TRUE, TRUE);

	gtk_widget_show(v->hpane);

	gtk_box_pack_start(GTK_BOX(vbox), v->hpane, TRUE, TRUE, 0);

	/* Status bar */
	v->statusbar = gtk_hbox_new(FALSE, 20);
	v->ViewXY = gtk_label_new("ViewXY");
	gtk_widget_show(v->ViewXY);
	gtk_box_pack_start(GTK_BOX(v->statusbar), v->ViewXY, FALSE, TRUE, 0);
	v->MapXY = gtk_label_new("MapXY");
	gtk_widget_show(v->MapXY);
	gtk_box_pack_start(GTK_BOX(v->statusbar), v->MapXY, FALSE, TRUE, 0);
	v->OtherXY = gtk_label_new("OtherXY");
	gtk_widget_show(v->OtherXY);
	gtk_box_pack_start(GTK_BOX(v->statusbar), v->OtherXY, FALSE, TRUE, 0);

	v->ToolName = gtk_label_new("No Tool");
	gtk_widget_show(v->ToolName);
	gtk_box_pack_end(GTK_BOX(v->statusbar), v->ToolName, FALSE, TRUE, 1);

	gtk_widget_show(v->statusbar);
	gtk_widget_show(vbox);

	gtk_box_pack_end(GTK_BOX(vbox), v->statusbar, FALSE, TRUE, 0);

	register_standard_tools(v);

	gtk_container_add(GTK_CONTAINER(v->window), vbox);
	gtk_widget_show(v->window);

	return v;
}
