#include "window.h"
#include "editor_widget.h"

struct _PlatonWindow {
	GtkApplicationWindow parent_instance;
};

G_DEFINE_TYPE(PlatonWindow, platon_window, GTK_TYPE_APPLICATION_WINDOW)

static PlatonEditorWidget* get_editor_widget(PlatonWindow* self) {
	GtkWidget* scrolled_window = gtk_bin_get_child(GTK_BIN(self));
	return PLATON_EDITOR_WIDGET(gtk_bin_get_child(GTK_BIN(scrolled_window)));
}

static void save(GSimpleAction* action, GVariant* parameter, gpointer user_data) {
	PlatonWindow* self = PLATON_WINDOW(user_data);
	PlatonEditorWidget* editor_widget = get_editor_widget(self);
	platon_editor_widget_save(editor_widget);
}

static void platon_window_class_init(PlatonWindowClass* klass) {

}

static void platon_window_init(PlatonWindow* self) {
	gtk_window_set_default_size(GTK_WINDOW(self), 800, 550);

	GSimpleAction* save_action = g_simple_action_new("save", NULL);
	g_signal_connect_object(save_action, "activate", G_CALLBACK(save), self, 0);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(save_action));
	g_object_unref(save_action);

	GtkWidget* header_bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Platon");
	GtkWidget* save_button = gtk_button_new_from_icon_name("document-save-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_tooltip_text(save_button, "Save File");
	gtk_actionable_set_action_name(GTK_ACTIONABLE(save_button), "win.save");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), save_button);
	gtk_widget_show_all(header_bar);
	gtk_window_set_titlebar(GTK_WINDOW(self), header_bar);
}

PlatonWindow* platon_window_new(GtkApplication* application) {
	return g_object_new(PLATON_TYPE_WINDOW, "application", application, NULL);
}

void platon_window_open_file(PlatonWindow* self, GFile* file) {
	GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	PlatonEditorWidget* editor_widget = platon_editor_widget_new(file);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(editor_widget));
	gtk_widget_show_all(scrolled_window);
	gtk_container_add(GTK_CONTAINER(self), scrolled_window);
}
