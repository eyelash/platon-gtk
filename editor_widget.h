#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLATON_TYPE_EDITOR_WIDGET platon_editor_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(PlatonEditorWidget, platon_editor_widget, PLATON, EDITOR_WIDGET, GtkWidget)

struct _PlatonEditorWidgetClass {
	GtkWidgetClass parent_class;
	void (*insert_newline)(PlatonEditorWidget* self);
	void (*delete_backward)(PlatonEditorWidget* self);
	void (*delete_forward)(PlatonEditorWidget* self);
	void (*move_left)(PlatonEditorWidget* self, gboolean extend_selection);
	void (*move_right)(PlatonEditorWidget* self, gboolean extend_selection);
	void (*move_up)(PlatonEditorWidget* self, gboolean extend_selection);
	void (*move_down)(PlatonEditorWidget* self, gboolean extend_selection);
	void (*move_to_beginning_of_line)(PlatonEditorWidget* self, gboolean extend_selection);
	void (*move_to_end_of_line)(PlatonEditorWidget* self, gboolean extend_selection);
};

PlatonEditorWidget* platon_editor_widget_new(GFile* file);

G_END_DECLS
