/*
 * gmap.h
 * Copyright (C) 2007 Itai Nahshon
 */

/* For sake of MSVC... */
#define _USE_MATH_DEFINES

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>
#include <string.h>
/* #include <malloc.h> */
#include <ogr_srs_api.h>
#include <math.h>

/* USE glib memory allocator */
/* Actually use much more. Using the g_XXX is probably OK. */
#define gmap_malloc g_malloc
#define gmap_realloc g_realloc
#define gmap_malloc0 g_malloc0
#define gmap_free g_free
#define gmap_strdup g_strdup

#define COLOR_T	unsigned long

#ifndef bool
#define bool int
#endif

/* Map calibration point */
struct cpoint {
	double		mapx, mapy;
	double		geox, geoy;
#if 0
	double		errx, erry;	/* GeoTransform calculated error in geo units */
#endif
	bool		valid;
};

struct MapPoint {
	// In map coordinates
	double X, Y;
};

struct Map {
	char		*filename;
	char		*fullpath;
	double		GeoTransform[6];
	bool		visible;

	struct MapSet	*mapset;

	int		width, height;
	int		bpp;

	struct {
		int	x, y, w, h;			/* Cropping rect for map image */
	} Rect;

	int 		CropPoints;
	struct MapPoint	*Crop;			/* Cropping Polygon (to replace Rect) */

	int		n_calibration_points;
	struct cpoint	*calibration_points;
	GdkPixbuf	*cached;			/* If cached in memory */

	void		*cachedDS;			/* cached data set for gdal */
	/* To fix a feature in GDAL... if the transform is {0 1 0 0 0 1} we must pass in a NULL ! */
	void		*cachedDSNULL;			/* cached data set for gdal only if GeoTransform is not{ 0 1 0 0 0 1} */
};

struct MapSet {
	char		*name;
	char		*description;
	char		*basedir;
	char		*WKT;
	char		*filename;			/* of XML file */
	int		count;				/* How many maps */
	struct Map	*maps;
	bool		dirty;

	double		left, right, top, bottom;	/* bounding rect in own coordinates */
	double		preferred_scale;
	double		max_scale;
	double		min_scale;
};

enum LayerType {
	LAYER_NONE,
	LAYER_SOLID,
	LAYER_SOLID_ALPHA,
	LAYER_MAPSET,
	LAYER_CALIBRATE,
	LAYER_GRID,
	LAYER_TRACKSET,
	LAYER_ROUTESET,
	LAYER_WAYPOINTSET,
	LAYER_AFFINEGRID,
	LAYER_SRTM,
};

struct RenderTarget {
	double		left, right, top, bottom;	/* area boundaries in geographic coordinates */
	int		width, height;			/* enough to hold everything */
	double		GeoTransform[6];
	char		*WKT;
	int		n_layers;
	struct Layer	*layers;

	double		scale;				/* Wanted scale */
	double		rotation;			/* in degrees. 0 = no roration */
	double		x_resulution;			/* DPI */
	double		y_resulution;			/* DPI */
};

struct RenderContext {
	struct RenderTarget *rt;
	cairo_surface_t *cs;
	cairo_t *ct;
	int x, y, w, h;
};

struct MapView;

/* Layer flags */
enum LayerFlags {
	LAYER_FLAGS_NONE = 0,
	LAYER_IS_VISIBLE = 1,
	LAYER_IS_TREE_SEARCHABLE = 2,
};

struct LayerOps {
	void (*render_layer)(const struct Layer *layer, const struct RenderContext *context);
	void (*calc_target_data)(struct Layer *layer, const struct RenderTarget *target);
	void (*free_target_data)(struct Layer *layer, const struct RenderTarget *target);
};

struct Layer {
	const struct LayerOps	*ops;
	enum LayerType	type;
	//bool		visible;
	unsigned long	flags;
	//char 		*name;
	void 		*data;	/* Data shared by all instalces */
	void		*priv;	/* data calculated per-target */
};

struct MapView {
	char		*name;

	struct MainWindow *mainwindow;

	/* tansformation and view window */
	struct RenderTarget rt;
	//double		left, right, top, bottom;	/* visible-scrollable area */
	//double		GeoTransform[6];
	//char		*WKT;
	bool		geographic_ref;

	double		preferred_scale;

	/* Size (pixels) of the full layout window. Calculated when scaling the view. */
	// int		width;
	// int		height;

	/* Size (pixels) of just the visible part the layout window */
	/* Set in a "size-allocate" event handler (when the scrolledwindow resizes) */
	/* Used to find the center etc.. */
	int		allocation_width;
	int		allocation_height;

	/* Scrolling adjustments to set/get current position */
	GtkAdjustment	*hadjustment;
	GtkAdjustment	*vadjustment;

	/* Current tool */
	struct ToolItem	*current_tool;		/* The current tool in use */
	struct ToolItem	*tools;			/* Array of available tools */
	int		n_tools;		/* counter of registerred tools */
	int		current_tool_num;
	GSList		*tool_action_group;	/* Radio group used when a tool is added */

	/* MapView top-level Widgets */
	GtkWidget	*scrolledwindow;
	GtkWidget	*layout;
	GtkWidget	*statusbar;

	/* Status bar fields */
	GtkWidget	*ViewXY;	/* In display coordinates */
	GtkWidget	*MapXY;		/* In map coordinates */
	GtkWidget	*OtherXY;	/* Normally WGS84 or UTM coordinates */
	GtkWidget	*ToolName;	/* name of current tool */

	GtkUIManager	*ui;
	GtkActionGroup	*actions;
	GtkWidget	*popup;
	GtkWidget	*menu;

	GtkWidget	*hpane;
	GtkWidget	*window;

	GtkWidget	*set_projection;

	/* cached transformed maps */
	int maxcache;
	struct cache *cache;

	double		hand_x, hand_y;	/* position where middle button was clicked */
	bool		scrolling;

	/* Used by standard tools (zoom-in, zoom-out) */
	int		x0, y0, x1, y1;	/* Corners or end-points */
	bool		on_display;	/* Tool made a drawing on the display */
	bool		active;		/* Tool is active (= left button is pressed) */
	GdkGC		*gc;		/* GC used to draw temm lines by inversion */

	int		last_x;
	int		last_y;
	guint		show_near_objects_proc;
	GtkWidget	*near_objects_window;
	void		*towgs84;
};

/* input event handlers for current tool */
struct Tool {
	/* like GtkRadioActionEntry */
	const gchar	*name;
	const gchar	*stock_id;
	const gchar	*label;
	const gchar	*accelerator;	/* can this be used ? */
	const gchar	*tooltip;
	/* Following are actions for this tool */
	void		(*ButtonPress)(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata);
	void		(*ButtonRelease)(struct MapView *mapview, int x, int y, int button, int mask, void *tooldata);
	void		(*MouseMove)(struct MapView *mapview, int x, int y, void *tooldata);
	void		(*Select)(struct MapView *mapview, void *tooldata);
	void		(*UnSelect)(struct MapView *mapview, void *tooldata);
};

struct ToolItem {
	struct Tool	*tool;
	void		*tooldata;
};

struct ElevationView {
	char		*name;

	bool		seen;

	GtkWidget	*elevationSW;
	GtkWidget	*elevation;
};

struct MainWindow {
	GtkWidget	*window;
	GtkWidget	*menubar;
	GtkWidget	*main_dock;

	GtkUIManager	*ui;
	GtkActionGroup	*actions;
};

struct Point {
	double		geo_lat;
	double		geo_lon;
	double		elevation;
};

struct GeoRect {
	// Geographic coordinates
	double		left, right, top, bottom;
};

#define DEFAULT_WAYPT_SYM "*"
struct WayPoint {
	struct Point	point;
	char		*name;
	char		*comment;
	char		*description;
	char		*symbol;

	void		*image;			/* surface_t */
};

struct WayPointSet {
	int		count;
	struct WayPoint	*waypoints;
};

struct TrackPoint {
	struct Point	point;
	GTimeVal	time;
	double		distance;	/* accumulated distance from begining of track  XXX */
// 	int serial;
};

struct TrackSeg {
	int	count;
	struct TrackPoint *trackpoints;
};

struct Track {
	char	*name;
	int	number;
	int	count;
	struct TrackSeg *tracksegments;
	COLOR_T	color;
	double	width;
	void	*dashes;
};

struct TrackSet {
	int	count;
	struct Track	*tracks;
};

struct RoutePoint {
	struct Point	point;
};

struct Route {
	char	*name;
	int	count;
	struct RoutePoint *routepoints;
	COLOR_T	color;
	double	width;
	void	*dashes;
};

struct RouteSet {
	int count;
	struct Route	*routes;
};

struct AffineGridData {
	COLOR_T		hcolor, vcolor;
	double  	hspaces, vspaces;
	double		line_width;
	double		*GeoTransform;
};

#define CACHETILE 256
struct cache {
	int		left, top;
	GdkPixbuf	*pixbuf;
};

enum {
	UTF8_DEGREES,	/* degrees sigh */
	UTF8_MINUTES,	/* Minutes sign */
	UTF8_SECONDS,	/* Seconds sign */
	UTF8_LTRMARK,	/* left-to-right mark */

	/* last = array size */
	UTF8_LAST
};

extern char *UTF8[UTF8_LAST];

/* mapset.c */
struct MapSet *new_mapset();
void mapset_set_projection(struct MapSet *mapset, const char *str);
void mapset_set_description(struct MapSet *mapset, const char *str);
void mapset_set_name(struct MapSet *mapset, const char *str);
void mapset_set_basedir(struct MapSet *mapset, const char *str);
int mapset_save(struct MapSet *mapset, char *filename);
struct Map *new_map(struct MapSet *mapset, const char *filename);
int map_add_cpoint(struct MapSet *mapset, struct Map *map, const struct cpoint *cp);
void map_add_crop_point(struct MapSet *mapset, struct Map *map, int pos, double x, double y);
void map_set_croprect(struct MapSet *mapset, struct Map *map, double x, double y, double w, double h);
int map_remove_cpoint(struct MapSet *mapset, struct Map *map, int which);
struct MapSet *mapset_from_file(const char *filename);
#ifdef LIBXML_VERSION
struct MapSet *mapset_from_doc(xmlDocPtr doc);
#endif
void mapset_recalculate_preferred_scale(struct MapSet *mapset);
void mapset_recalculate_bounds(struct MapSet *mapset);

/* mapset_gui.c */
GtkWidget * mapset_view(struct MapSet *mapset, struct MapView *mapview);

/* mapset_gdal.c */
void GDAL_init_drivers();
void mapview_set_projection_and_scale_from_mapset(struct MapSet *mapset, struct MapView *mapview);
void map_cache(struct Map *map);
void mapset_init_layer(struct Layer *layer, enum LayerType type, struct MapSet *mapset);
void map_uncache(struct Map *map);
void mapview_changed_projection(struct MapView *mapview);
void mapview_center_map_region(struct MapView *mapview, double xx0, double xx1, double yy0, double yy1);

/* projection_gui.c */
GtkWidget *create_projection_tool(struct MapView *mapview);

/* gdal_utils.c */
void pixel_to_geo_xy(const double *GeoTransform, double pixel_x, double pixel_y, double *geo_x, double *geo_y);
bool geo_to_pixel_xy(const double *GeoTransform, double geo_x, double geo_y, double *pixel_x, double *pixel_y);
void multiply_geotransform(const double *G1, const double *G2, double *res);
void copy_geotransform(const double *G1, double *res);
void set_unity_geotransform(double *res);
bool is_unity_geotransform(double *GeoTransform);


/* mapwindow.c */
struct MapView *create_map_window(struct MainWindow *mainwindow);
void mapview_goto_xy(struct MapView *mapview, double geo_x, double geo_y, double dpy_x, double dpy_y);
void mapview_point_to_view(struct MapView *mapview, double geo_x, double geo_y);
void mapview_get_center(struct MapView *mapview, double *geo_x, double *geo_y);
void mapwindow_register_tool(struct MapView *mapview, struct Tool *tool, void *tooldata,
bool add_to_tools_menu, bool add_to_context_menu, bool make_it_current_tool);
void mapview_set_name(struct MapView *mapview, char *name);
struct Layer *target_add_layer(struct RenderTarget *target);
void mapview_set_scale(struct MapView *mapview, double scale);
void target_set_scale(struct RenderTarget *target, double scale);
void target_free_data(struct RenderTarget *target);
void mapview_register_copy_coord_tool(struct MapView *mapview);

/* track.c */
bool load_from_gpx(char *filename, struct TrackSet **trkset, struct RouteSet **routeset, struct WayPointSet **waypointset);

/* waypoint.c */
struct WayPointSet *new_waypointset();
struct WayPoint *new_waypoint(char *name, struct WayPointSet *waypointset);
struct RouteSet *new_routeset();
struct Route *new_route(char *name, struct RouteSet *routeset);
struct RoutePoint *new_routepoint(struct Route *route);

/* mapfit.c */
void map_calc_GeoReference_error(struct Map *map);
bool map_calc_GeoReference(struct Map *map);

/* tree.c */
struct TreeNode;
void tree_to_pixmap(struct TreeNode *t, struct RenderTarget *target, cairo_t *ct, int x, int y, int w, int h);
void free_tree(struct TreeNode *t);
struct TreeNode *new_branch(int top, int bottom, int left, int right);
void tree_add_waypoint(struct TreeNode *t, struct WayPoint *wpt, int x, int y);
void tree_add_segment(struct TreeNode *t, struct Track *trk, int x0, int y0, int x1, int y1, struct TrackPoint *p1, struct TrackPoint *p2);
char *get_near_object(struct MapView *mapview, int x, int y);

/* solid_fill.c */
void solid_fill_init_layer(struct Layer *layer, enum LayerType type, double a, double r, double g, double b);

/* point_gdal.c */
void trackset_init_layer(struct Layer *layer, enum LayerType type, struct TrackSet *trackset);
void routeset_init_layer(struct Layer *layer, enum LayerType type, struct RouteSet *routeset);
void waypointset_init_layer(struct Layer *layer, enum LayerType type, struct WayPointSet *waypointset);
bool trackset_calc_extents(struct TrackSet *trackset, const struct RenderTarget *target, struct GeoRect *rect);

/* file_utils.c */
char *get_relative_filename(const char *filename, const char *basedir);
char *get_absolute_filename(const char *filename);

/* gmap_dock.c */
void show_about_dialog (GtkAction *action, struct MainWindow *mainwindow);

/* affinegrid.c */
void affine_grid_init_layer(struct Layer *layer, enum LayerType type, struct AffineGridData *data);

/* calibrate.c */
struct MapView *calibrate_map(struct MainWindow *mainwindow, struct MapSet *mapset, int mapno);

/* waypoint_symbols.c */
void *get_image_for_symbol(char *sym);

/* layers_box.c */
GtkWidget *create_layers_box(struct MapView *mapview);

/* inverse.c */
double Distance(double lat1, double lon1, double lat2, double lon2);

/* utf8.c */
void utf8_init();

/* print.c */
void do_page_setup (struct MapView *mapview);
void do_print(struct MapView *mapview);

/* projection.c */
void do_set_projection(struct MapView *mapview);
