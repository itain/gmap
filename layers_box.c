#include "gmap.h"

static const char *layer_type_names[] = {
	"NONE",
	"SOLID",
	"SOLID_ALPHA",
	"MAPSET",
	"CALIBRATE",
	"GRID",
	"TRACKSET",
	"ROUTESET",
	"WAYPOINTSET",
	"AFFINEGRID",
};

GtkWidget *
create_layers_box(struct MapView *mapview) {
	GtkWidget *vbox;
	GtkWidget *scrolled;
	GtkWidget *treeview;
	int i;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;

	store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);

	for(i = 0; i < mapview->rt.n_layers; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
                      0, i,
                      1, layer_type_names[mapview->rt.layers[i].type],
		      2, (mapview->rt.layers[i].flags & LAYER_IS_VISIBLE),
                      -1);
	}

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
                                               -1,      
                                               "#",  
                                               renderer,
                                               "text", 0,
                                               NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
                                               -1,      
                                               "Type",  
                                               renderer,
                                               "text", 1,
					       "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
                                               NULL);
	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
                                               -1,      
                                               "Visible",  
                                               renderer,
                                               "active", 2,
                                               NULL);

	scrolled = gtk_scrolled_window_new(0, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled), treeview);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

	gtk_widget_show_all(vbox);

	gtk_paned_pack2(GTK_PANED(mapview->hpane), vbox, FALSE, TRUE);

	return scrolled;
}
