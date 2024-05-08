#include "application.h"
#include "window.h"

struct _PlatonApplication {
	GtkApplication parent_instance;
};

G_DEFINE_TYPE(PlatonApplication, platon_application, GTK_TYPE_APPLICATION)

static void platon_application_startup(GApplication* application) {
	PlatonApplication* self = PLATON_APPLICATION(application);
	G_APPLICATION_CLASS(platon_application_parent_class)->startup(application);
	gtk_application_set_accels_for_action(GTK_APPLICATION(application), "win.save", (const gchar*[]){"<Primary>S", NULL});
}

static void platon_application_activate(GApplication* application) {
	PlatonApplication* self = PLATON_APPLICATION(application);
	PlatonWindow* window = platon_window_new(GTK_APPLICATION(self));
	platon_window_open_file(window, NULL);
	gtk_window_present(GTK_WINDOW(window));
}

static void platon_application_open(GApplication* application, GFile** files, gint n_files, const gchar* hint) {
	PlatonApplication* self = PLATON_APPLICATION(application);
	for (gint i = 0; i < n_files; ++i) {
		PlatonWindow* window = platon_window_new(GTK_APPLICATION(self));
		platon_window_open_file(window, files[i]);
		gtk_window_present(GTK_WINDOW(window));
	}
}

static void platon_application_class_init(PlatonApplicationClass* klass) {
	G_APPLICATION_CLASS(klass)->startup = platon_application_startup;
	G_APPLICATION_CLASS(klass)->activate = platon_application_activate;
	G_APPLICATION_CLASS(klass)->open = platon_application_open;
}

static void platon_application_init(PlatonApplication* self) {

}

PlatonApplication* platon_application_new(void) {
	return g_object_new(PLATON_TYPE_APPLICATION, "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}

int main(int argc, char** argv) {
	PlatonApplication* application = platon_application_new();
	int result = g_application_run(G_APPLICATION(application), argc, argv);
	g_object_unref(application);
	return result;
}
