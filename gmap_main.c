/*
 * gmap_dock.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"

#if 0
static struct AffineGridData default_grid = {
	0x0000800000, 0x0080000000,	/* hcolor, vcolor */
	5000, 5000,			/* hspaces, vspaces */
	1,				/* line_width */
	NULL,				/* GeoTransform */
};
#endif


void
show_about_dialog (GtkAction *action, struct MainWindow *mainwindow) {
	GdkPixbuf *logo = gdk_pixbuf_new_from_file ("gmap_logo.png", NULL);
	gtk_show_about_dialog(GTK_WINDOW(mainwindow->window),
		"name", "gmap", 
		"version", "0.1",
		"copyright", "(C) 2007 Itai Nahshon",
		"comments", "Program to display maps.",
		"logo", logo,
		NULL);
	g_object_unref(logo);
}

static void
activate_action (GtkAction *action, struct MainWindow *mainwindow)
{
	g_message ("Action \"%s\" activated", gtk_action_get_name(action));
}

struct MapView *
new_mapview_open_mapset(struct MainWindow *mainwindow, const char *filename) {
	struct MapView *mapview;
	struct MapSet *mapset;
	struct Layer *layer;

	mapset = mapset_from_file(filename);
	if(mapset == NULL) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow->window),
						0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Could not open %s", filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}

	mapview = create_map_window(mainwindow);
	layer = target_add_layer(&mapview->rt);
	solid_fill_init_layer(layer, LAYER_SOLID, 1.0, 1.0, 1.0, 1.0);
	layer = target_add_layer(&mapview->rt);
	mapset_init_layer(layer, LAYER_MAPSET, mapset);

	mapview_set_projection_and_scale_from_mapset(mapset,  mapview);

 	mapview_register_copy_coord_tool(mapview);

	mapview_set_name(mapview, mapset->name);

	return mapview;
}

struct MapView *
open_mapset_for_calibration(struct MainWindow *mainwindow, const char *filename) {
	struct MapSet *mapset;

	mapset = mapset_from_file(filename);
	if(mapset == NULL) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow->window),
						0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Counld not open %s", filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
	return calibrate_map(mainwindow, mapset, mapset->count > 0?0:-1);
}

void
open_mapset_action(GtkAction *action, struct MainWindow *mainwindow) {
	GtkWidget *dialog;
	GtkFileFilter *filter;
	const char *action_name = gtk_action_get_name (action);

	g_message("open_mapset_action");
	g_message ("Action \"%s\" activated", action_name);

	dialog = gtk_file_chooser_dialog_new ("Open MapSet Files",
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
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

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));;
		g_message("filename = %s", filename);
		// open_file (filename);
		if(!strcmp(action_name, "Calibrate"))
			open_mapset_for_calibration(mainwindow, filename);
		else
			new_mapview_open_mapset(mainwindow, filename);
		g_free(filename);
	}

	gtk_widget_destroy (dialog);
}

static GtkActionEntry ui_entries[] = {
  { "FileMenu",	NULL, "_File" },		/* name, stock id, label */
  { "EditMenu", NULL, "_Edit" },		/* name, stock id, label */
  { "ViewMenu", NULL, "_View"  },		/* name, stock id, label */
  { "HelpMenu", NULL, "_Help" },		/* name, stock id, label */
  { "WindowsMenu", NULL, "Windows" },		/* name, stock id, label */

  { "New", GTK_STOCK_NEW,			/* name, stock id */
    NULL, "<control>N",				/* label, accelerator */
    "Create a new file",			/* tooltip */
    G_CALLBACK (activate_action) },
  { "Open", GTK_STOCK_OPEN,			/* name, stock id */
    NULL, "<control>N",				/* label, accelerator */
    "open a mapset in a new window",		/* tooltip */
    G_CALLBACK (open_mapset_action) },
  { "Calibrate", NULL,				/* name, stock id */
    "Calibrate mapset..", "<control>K",		/* label, accelerator */
    "open a mapset for calibration",		/* tooltip */
    G_CALLBACK (open_mapset_action) },
  { "Quit", GTK_STOCK_QUIT,			/* name, stock id */
    NULL, "<control>Q",				/* label, accelerator */
    "Close all winows and exit",		/* tooltip */
    G_CALLBACK (gtk_main_quit) },
  { "About", GTK_STOCK_ABOUT,			/* name, stock id */
    NULL, NULL,					/* label, accelerator */
    "Something about gmap",			/* tooltip */
    G_CALLBACK (show_about_dialog) },
};

static guint n_ui_entries = G_N_ELEMENTS (ui_entries);

static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='Calibrate'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
//"      <menuitem action='Open'/>"
//"      <menuitem action='Save'/>"
//"      <menuitem action='SaveAs'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menu action='WindowsMenu'>"
"        <placeholder name='all_windows'>"
"        </placeholder>"
"      </menu>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

/* Returns a menubar widget made from the above menu */
GtkWidget *
get_menubar_menu(struct MainWindow *mainwindow)
{
	GtkUIManager *ui;
	GtkActionGroup *actions;
	GError *error = NULL;
	GtkWidget *menubar;

	actions = gtk_action_group_new ("Actions");
	gtk_action_group_add_actions (actions, ui_entries, n_ui_entries, mainwindow);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (mainwindow->window),
				  gtk_ui_manager_get_accel_group (ui));

	if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	menubar = gtk_ui_manager_get_widget (ui, "/MenuBar");
	gtk_widget_show_all(menubar);

	mainwindow->ui = ui;
	mainwindow->actions = actions;

	// add_to_ui_manager(mainwindow);

	return menubar;
}

#if 0
void add_extra_tools(struct MainWindow *v) {
	GtkWidget *item2, *item3;

	/* represents toolbox */
	v->toolbox = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (v->toolbox), GTK_SHADOW_IN);
	gtk_widget_set_size_request(v->toolbox, 100, 100);
	gtk_widget_show(v->toolbox);

	/* Make dockable window */
	item3 = gdl_dock_item_new("tool1", "Toolbox 1", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item3), v->toolbox);
	gdl_dock_add_item (GDL_DOCK (v->main_dock), GDL_DOCK_ITEM (item3), 
			   GDL_DOCK_FLOATING);
	gtk_widget_show (item3);


	/* Elevation window */
	v->elevation[0] = (struct ElevationView *)gmap_malloc(sizeof (struct ElevationView));
	v->elevation[0]->name = gmap_strdup("Elevation");
	v->elevation[0]->seen = TRUE;
	v->elevation[0]->elevationSW = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(v->elevation[0]->elevationSW), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	v->elevation[0]->elevation = gtk_drawing_area_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(v->elevation[0]->elevationSW), v->elevation[0]->elevation);
	gtk_widget_show(v->elevation[0]->elevation);
	gtk_widget_show(v->elevation[0]->elevationSW);

	/* Make dockable window */
	item2 = gdl_dock_item_new("elevation", "Elevation Window", GDL_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item2), v->elevation[0]->elevationSW);
	gdl_dock_add_item (GDL_DOCK (v->main_dock), GDL_DOCK_ITEM (item2), 
			   GDL_DOCK_FLOATING);
	gtk_widget_show (item2);

}
#endif

struct MainWindow *
create_main_window() {
	struct MainWindow *v;
	GtkWidget *main_vbox, *menubar;

	v = (struct MainWindow *)gmap_malloc(sizeof(struct MainWindow));

	v->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	g_signal_connect(G_OBJECT(v->window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_window_set_title(GTK_WINDOW(v->window), "Gmap main window");
	gtk_widget_set_size_request (GTK_WIDGET(v->window), 400, 100);

	/* Make a vbox */
	main_vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER(main_vbox), 1);
	gtk_container_add(GTK_CONTAINER(v->window), main_vbox);

	/* Create the menubar */
	menubar = get_menubar_menu(v);
	gtk_widget_show(menubar);
	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);

	/* Show the widgets */
	gtk_widget_show(main_vbox);
	gtk_widget_show(v->window);

	return v;
}

int main(int argc, char **argv) {
	struct MainWindow *mainwindow;
	struct MapView *mapview;
	/* data layers */
	struct MapSet *mapset;
	// struct TrackSet *trackset;
	// struct RouteSet *routeset;
	// struct WayPointSet *waypointset;
	
	/* Initialize GTK */
	gtk_init (&argc, &argv);
	GDAL_init_drivers();
	utf8_init();
	

	mainwindow = create_main_window();

#if 1
	/* First mapview */
	mapset = mapset_from_file(
		//"refmaps/maps50.xml"
		//"maps250.xml"
		//"world_map.xml"
		"refmaps/political_world2.xml"
		//"physical_world.xml"
		);
	if(mapset) {
		mapview = create_map_window(mainwindow);
		mapview_set_name(mapview, mapset->name);
		solid_fill_init_layer(target_add_layer(&mapview->rt), LAYER_SOLID, 1.0, 1.0, 0.5, 0.5);
		mapset_init_layer(target_add_layer(&mapview->rt), LAYER_MAPSET, mapset);
		//solid_fill_init_layer(target_add_layer(&mapview->rt), LAYER_SOLID_ALPHA, 0.75, 1.0, 1.0, 1.0);
		//affine_grid_init_layer(target_add_layer(&mapview->rt), LAYER_AFFINEGRID, &default_grid);
#if 0
		load_from_gpx(
			//"20070602.gpx",
			"backup-2007-03-11.gpx",
			&trackset, &routeset, &waypointset);
		//waypointset = NULL;
		trackset_init_layer(target_add_layer(&mapview->rt), LAYER_TRACKSET, trackset);
		routeset_init_layer(target_add_layer(&mapview->rt), LAYER_ROUTESET, routeset);
		waypointset_init_layer(target_add_layer(&mapview->rt), LAYER_WAYPOINTSET, waypointset);
#endif

#if 1
		mapview_set_projection_and_scale_from_mapset(mapset,  mapview);
#else
		{
		OGRSpatialReferenceH osrs = OSRNewSpatialReference(NULL);
		OSRSetOrthographic(osrs, 50., 70., 0, 0.);
		//OSRSetRobinson(osrs, 10., 0., 0.);
		char *wkt;
		OSRExportToWkt(osrs, &wkt);
		mapview->rt.WKT = gmap_strdup(wkt);
		mapview->rt.left =   -6400000;
		mapview->rt.right =   6400000;
		mapview->rt.top =     6400000;
		mapview->rt.bottom = -6400000;
		mapview_set_scale(mapview, 20000.);
		mapview_changed_projection(mapview);
		g_message("WKT='%s'", wkt);
		}
#endif
	}
#endif

	// select_region_tool_start(mapview);

	//create_layers_box(mapview);

	/* add_extra_tools(mainwindow);*/

#if 0
 	calibrate_map(mainwindow, mapset_from_file(
		// "maps250.xml"
		"physical_world.xml"
		//"political_world.xml"
		//"world_map.xml"
		), 0);
#endif

	/* Finished! */
	gtk_main ();

	return 0;
}
