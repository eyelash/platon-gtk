#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLATON_TYPE_EDITOR_WIDGET platon_editor_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(PlatonEditorWidget, platon_editor_widget, PLATON, EDITOR_WIDGET, GtkWidget)

struct _PlatonEditorWidgetClass {
	GtkWidgetClass parent_class;
};

PlatonEditorWidget* platon_editor_widget_new(GFile* file);

G_END_DECLS
