#include "gmap.h"

enum GrabLocation {
	NONE,
	TOP,
	BOTTOM,
	RIGHT,
	LEFT,
	TOP_RIGHT,
	TOP_LEFT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT,
};

struct SelectRegion {
	struct MapView *mapview;
	double r, g, b, a;
	double top, bottom, right, left;
	bool active;
	enum GrabLocation loc;
	double GeoTransform[6];
};

/* copied from tree.c */
static double
point0_to_segment_dist(double x0, double y0, double x1, double y1, double *u) {
	double dx = x1-x0;
	double dy = y1-y0;
	double yy0 = x0*dx + y0*dy;
	double yy1 = x1*dx + y1*dy;

	if((yy0 >= 0) !=  (yy1 >= 0)) {
		*u = yy0 / (yy0 - yy1); 
		return fabs(x0*y1-x1*y0)/hypot(dx, dy);
	}
	else if(fabs(yy0) < fabs(yy1)) {
		*u = 0;
		return hypot(x0, y0);
	}
	else {
		*u = 1;
		return hypot(x1, y1);
	}
}

static void
get_corners(struct SelectRegion *r, double result[8]) {
	struct MapView *v = r->mapview;
	double xx, yy;

	pixel_to_geo_xy(r->GeoTransform, r->left, r->top, &xx, &yy);
	geo_to_pixel_xy(v->rt.GeoTransform, xx, yy, &result[0], &result[1]);
	pixel_to_geo_xy(r->GeoTransform, r->right, r->top, &xx, &yy);
	geo_to_pixel_xy(v->rt.GeoTransform, xx, yy, &result[2], &result[3]);
	pixel_to_geo_xy(r->GeoTransform, r->right, r->bottom, &xx, &yy);
	geo_to_pixel_xy(v->rt.GeoTransform, xx, yy, &result[4], &result[5]);
	pixel_to_geo_xy(r->GeoTransform, r->left, r->bottom, &xx, &yy);
	geo_to_pixel_xy(v->rt.GeoTransform, xx, yy, &result[6], &result[7]);
}

static void
select_region_tool_button_press(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata) {
	struct SelectRegion *r = (struct SelectRegion *)tooldata;
	bool bt, bb, br, bl;
	double corners[8];
	double u;

	get_corners(r, corners);

	bt = (abs(point0_to_segment_dist(corners[0]-x, corners[1]-y, corners[2]-x, corners[3]-y, &u)) < 3);
	br = (abs(point0_to_segment_dist(corners[2]-x, corners[3]-y, corners[4]-x, corners[5]-y, &u)) < 3);
	bb = (abs(point0_to_segment_dist(corners[5]-x, corners[5]-y, corners[6]-x, corners[7]-y, &u)) < 3);
	bl = (abs(point0_to_segment_dist(corners[6]-x, corners[7]-y, corners[0]-x, corners[1]-y, &u)) < 3);

	g_message("bt=%d bb=%d br=%d bl=%d", bt, bb, br, bl);

	r->loc = NONE;
	if(bt && br) r->loc = TOP_RIGHT;
	else if(bt && bl) r->loc = TOP_LEFT;
	else if(bb && br) r->loc = BOTTOM_RIGHT;
	else if(bb && bl) r->loc = BOTTOM_LEFT;
	else if(br) r->loc = RIGHT;
	else if(bl) r->loc = LEFT;
	else if(bt) r->loc = TOP;
	else if(bb) r->loc = BOTTOM;

	g_message("r->loc=%d", r->loc);

	if(r->loc != NONE)
		r->active = TRUE;
}

static void
select_region_tool_button_release(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata) {
	struct SelectRegion *r = (struct SelectRegion *)tooldata;
	if(!r->active)
		return;

	g_message("top %f bot %f rig %f lef %f",
		r->top, r->bottom, r->right, r->left);

	gtk_widget_queue_draw(mapview->layout);

	r->active = FALSE;
}

static void
select_region_tool_mouse_move(struct MapView *mapview, int x, int y, void *tooldata) {
	struct SelectRegion *r = (struct SelectRegion *)tooldata;
	double geo_x, geo_y;
	if(!r->active)
		return;

	pixel_to_geo_xy(mapview->rt.GeoTransform, x, y, &geo_x, &geo_y);
	geo_to_pixel_xy(r->GeoTransform, geo_x, geo_y, &geo_x, &geo_y);

	switch(r->loc) {
	case TOP:
		r->top = geo_y;
		break;
	case BOTTOM:
		r->bottom = geo_y;
		break;
	case RIGHT:
		r->right = geo_x;
		break;
	case LEFT:
		r->left = geo_x;
		break;
	case TOP_RIGHT:
		r->top = geo_y;
		r->right = geo_x;
		break;
	case TOP_LEFT:
		r->top = geo_y;
		r->left = geo_x;
		break;
	case BOTTOM_RIGHT:
		r->bottom = geo_y;
		r->right = geo_x;
		break;
	case BOTTOM_LEFT:
		r->bottom = geo_y;
		r->left = geo_x;
		break;
	default:
		;
	}
	gtk_widget_queue_draw(mapview->layout);
}

static void
select_region_render_layer(const struct Layer *layer, const struct RenderContext *rc) {
	struct RenderTarget *target = rc->rt;
	struct SelectRegion *r = (struct SelectRegion *)layer->data;
	double corners[8];

	get_corners(r, corners);

#if 1
	if((corners[0] < rc->x && corners[2] < rc->x && corners[4] < rc->x && corners[6] < rc->x) ||
	   (corners[0] > rc->x+rc->w && corners[2] > rc->x+rc->w && corners[4] > rc->x+rc->w && corners[6] > rc->x+rc->w) ||
	   (corners[1] < rc->y && corners[3] < rc->y && corners[5] < rc->y && corners[7] < rc->y) ||
	   (corners[1] > rc->y+rc->h && corners[3] > rc->y+rc->h && corners[5] > rc->y+rc->h && corners[7] > rc->y+rc->h)) {
		/* "hole" does not intersect region. paint everything */
		cairo_set_source_rgb(rc->ct, r->r, r->g, r->b);
		cairo_paint_with_alpha(rc->ct, r->a);
		return;
	}
#endif

	cairo_set_fill_rule(rc->ct, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_set_source_rgb(rc->ct, r->r, r->g, r->b);
	cairo_set_antialias(rc->ct, CAIRO_ANTIALIAS_NONE);

	cairo_rectangle(rc->ct, -rc->x, -rc->y, target->width, target->height);
	// cairo_rectangle(rc->ct, 0, 0, rc->w, rc->h);

	cairo_move_to(rc->ct, corners[0]-rc->x, corners[1]-rc->y);
	cairo_line_to(rc->ct, corners[2]-rc->x, corners[3]-rc->y);
	cairo_line_to(rc->ct, corners[4]-rc->x, corners[5]-rc->y);
	cairo_line_to(rc->ct, corners[6]-rc->x, corners[7]-rc->y);
	cairo_close_path(rc->ct);

	cairo_clip (rc->ct);

	cairo_paint_with_alpha(rc->ct, r->a);
}

static void
select_region_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
}

static const struct LayerOps select_region_layer_ops = {
	select_region_render_layer,
	select_region_calc_target_data,
	NULL,
};

void
select_region_init_layer(struct Layer *layer, enum LayerType type, struct SelectRegion *r)  {
	layer->type = type;
	layer->ops = &select_region_layer_ops;
	layer->flags = LAYER_IS_VISIBLE;
	layer->data = r;
	layer->priv = NULL;
}

static struct Tool select_region_tool = {
	"SelectRegion", GTK_STOCK_ZOOM_IN, "Select a region", "<CTRL>R", "Select a region on the map",
	select_region_tool_button_press,
	select_region_tool_button_release,
	select_region_tool_mouse_move,
	NULL, NULL,
};

void
select_region_tool_start(struct MapView *mapview) {
	struct SelectRegion *r = (struct SelectRegion *)gmap_malloc(sizeof(struct SelectRegion));
	r->mapview = mapview;
	r->r = r->g = r->b = 0.0;
	r->a = 0.5;
	r->top = mapview->rt.top - 2000;
	r->bottom = mapview->rt.bottom + 2000;
	r->right = mapview->rt.right - 2000;
	r->left = mapview->rt.left + 2000;
	r->active = FALSE;
	set_unity_geotransform(r->GeoTransform);

	r->GeoTransform[2]=0.1;
	r->GeoTransform[4]=-0.1;

	select_region_init_layer(target_add_layer(&mapview->rt), 999, r);

	mapwindow_register_tool(mapview, &select_region_tool, r, TRUE, TRUE, TRUE);
}
