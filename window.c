#include "window.h"
#include "editor_widget.h"

struct _PlatonWindow {
	GtkApplicationWindow parent_instance;
};

G_DEFINE_TYPE(PlatonWindow, platon_window, GTK_TYPE_APPLICATION_WINDOW)

static void platon_window_class_init(PlatonWindowClass* klass) {

}

static void platon_window_init(PlatonWindow* self) {
	gtk_window_set_default_size(GTK_WINDOW(self), 800, 550);
	GtkWidget* header_bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
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
