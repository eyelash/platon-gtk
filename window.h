#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLATON_TYPE_WINDOW platon_window_get_type()
G_DECLARE_FINAL_TYPE(PlatonWindow, platon_window, PLATON, WINDOW, GtkApplicationWindow)

PlatonWindow* platon_window_new(GtkApplication* application);
void platon_window_open_file(PlatonWindow* window, GFile* file);

G_END_DECLS
