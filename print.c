#include "gmap.h"

static void
begin_print_callback(GtkPrintOperation *op, GtkPrintContext *context, struct RenderTarget *target) {
	int ncols, nrows;

	gdouble width = gtk_print_context_get_width (context);
	gdouble height = gtk_print_context_get_height(context);
	gdouble dpi_x = gtk_print_context_get_dpi_x(context);
	gdouble dpi_y = gtk_print_context_get_dpi_y(context);

	g_message("begin_print_callback w=%f h=%f dpi_x=%f dpi_y=%f", width, height, dpi_x, dpi_y);

	ncols = (int)ceil(target->width/width);
	nrows = (int)ceil(target->height/height);

	gtk_print_operation_set_n_pages(op, nrows*ncols);
}

static void
draw_page_callback(GtkPrintOperation *op, GtkPrintContext *context, gint page_nr, struct RenderTarget *target) {
	cairo_t *cr;
	int ncols, col, row;
	int i;
	struct RenderContext rc;
	cairo_surface_t *temp_surface;
	cairo_t *temp;

	gdouble width = gtk_print_context_get_width (context);
	gdouble height = gtk_print_context_get_height(context);
	//gdouble dpi_x = gtk_print_context_get_dpi_x(context);
	//gdouble dpi_y = gtk_print_context_get_dpi_y(context);

	ncols = (int)ceil(target->width/width);

	col = page_nr % ncols;
	row = page_nr / ncols;
  
	// g_message("draw_page_callback page=%d col=%d row=%d", page_nr, col, row);

	cr = gtk_print_context_get_cairo_context (context);

	temp_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)ceil(width), (int)ceil(height));
	temp = cairo_create(temp_surface);

	rc.rt = target;
	rc.cs = temp_surface;
	rc.ct = temp;
	rc.x = (int)(col * width);
	rc.y = (int)(row * height);
	rc.w = (int)ceil(width);
	rc.h = (int)ceil(height);

	for(i = 0; i < target->n_layers; i++) {
		if(!(target->layers[i].flags & LAYER_IS_VISIBLE))
			continue;

		(*target->layers[i].ops->render_layer)(&target->layers[i], &rc);
	}

	// char fname[100];
	// sprintf(fname, "tmp_map_%03d_%03d_%03d.png", page_nr, col, row);
	// cairo_surface_write_to_png(temp_surface, fname);

	cairo_set_source_surface(cr, temp_surface, 0, 0);
	cairo_paint(cr);

	// cairo_destroy(cr);
	cairo_destroy(temp);
	cairo_surface_destroy(temp_surface);
}

static GtkPrintSettings *settings = NULL;
static GtkPageSetup *page_setup = NULL;

void
CopyTarget(struct RenderTarget *dest, struct RenderTarget *src) {
	int i;

	dest->left = src->left;
	dest->right = src->right;
	dest->top = src->top;
	dest->bottom = src->bottom;
	dest->WKT = gmap_strdup(src->WKT);
	dest->rotation = src->rotation;
	dest->x_resulution = src->x_resulution;
	dest->y_resulution = src->x_resulution;
	dest->n_layers = 0;
	dest->layers = NULL;

	for(i = 0; i < src->n_layers; i++) {
		struct Layer *ld;
		struct Layer *ls = &src->layers[i];
		ld = target_add_layer(dest);
		ld->ops = ls->ops;
		ld->type = ls->type;
		ld->flags = ls->flags;
		ld->data = ls->data;
		ld->priv = NULL;
	}

	target_set_scale(dest, src->scale);
}

void
do_print(struct MapView *mapview) {
	GtkPrintOperation *op;
	//GtkPrintOperationResult res;
	GError *err = NULL;
	struct RenderTarget rt;

	CopyTarget(&rt, &mapview->rt);

	op = gtk_print_operation_new();

	gtk_print_operation_set_print_settings(op, settings);
	gtk_print_operation_set_default_page_setup(op, page_setup);
	gtk_print_operation_set_show_progress(op, TRUE);

	g_signal_connect(op, "begin-print", G_CALLBACK(begin_print_callback), &rt);
	g_signal_connect(op, "draw-page", G_CALLBACK(draw_page_callback), &rt);

	/*res =*/ gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, 
					GTK_WINDOW(mapview->window), &err);

	/* DO not allow background-printing! */

	target_free_data(&rt);
} 

void
do_page_setup (struct MapView *mapview) {
	GtkPageSetup *new_page_setup;

	if (settings == NULL)
		settings = gtk_print_settings_new ();

	new_page_setup = gtk_print_run_page_setup_dialog(GTK_WINDOW(mapview->window), page_setup, settings);

	if (page_setup)
		g_object_unref (page_setup);

	page_setup = new_page_setup;
}
