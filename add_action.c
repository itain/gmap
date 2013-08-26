#include "gmap.h"

static void
show_hide_window_action (GtkAction *action)
{
	g_message ("Window \"%s\" activated", gtk_action_get_name (action));
}


static GtkActionEntry ui_entries[] = {
  { "win1", NULL,		/* name, stock id */
    "Window 1", NULL,		/* label, accelerator */
    "Show / Hide window",	/* tooltip */
    G_CALLBACK (show_hide_window_action) },
};
static guint n_ui_entries = G_N_ELEMENTS (ui_entries);

static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='EditMenu'>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menu action='WindowsMenu'>"
"        <placeholder name='all_windows'>"
"          <menuitem action='win1' />"
"        </placeholder>"
"      </menu>"
"    </menu>"
"  </menubar>"
"</ui>";

void
add_to_ui_manager(struct MainWindow *mainwindow) {
	GError *error = NULL;
	gtk_action_group_add_actions(mainwindow->actions, ui_entries, n_ui_entries, mainwindow);
	if (!gtk_ui_manager_add_ui_from_string (mainwindow->ui, ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}
}
