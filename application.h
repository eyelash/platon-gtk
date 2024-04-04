#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLATON_TYPE_APPLICATION platon_application_get_type()
G_DECLARE_FINAL_TYPE(PlatonApplication, platon_application, PLATON, APPLICATION, GtkApplication)

PlatonApplication* platon_application_new(void);

G_END_DECLS
