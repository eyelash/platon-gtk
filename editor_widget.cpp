#include "editor_widget.h"
#include "core/editor.hpp"
#include <cmath>

#if !GLIB_CHECK_VERSION(2, 73, 2)
#define G_CONNECT_DEFAULT ((GConnectFlags)0)
#endif

#define LINE_HEIGHT_FACTOR 1.5

typedef struct {
	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;
	GtkScrollablePolicy hscroll_policy;
	GtkScrollablePolicy vscroll_policy;
	Editor* editor;
	PangoFontDescription* font_description;
	double ascent;
	double line_height;
	double char_width;
	GtkIMContext* im_context;
	GtkGesture* multipress_gesture;
	GtkGesture* drag_gesture;
} PlatonEditorWidgetPrivate;

G_DEFINE_TYPE_WITH_CODE(PlatonEditorWidget, platon_editor_widget, GTK_TYPE_WIDGET,
	G_ADD_PRIVATE(PlatonEditorWidget)
	G_IMPLEMENT_INTERFACE(GTK_TYPE_SCROLLABLE, NULL)
)

typedef enum {
	PROP_0,
	PROP_HADJUSTMENT,
	PROP_VADJUSTMENT,
	PROP_HSCROLL_POLICY,
	PROP_VSCROLL_POLICY,
	N_PROPERTIES
} PlatonEditorWidgetProperty;

static void update(PlatonEditorWidget* self) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	if (priv->vadjustment) {
		const double page_size = gtk_widget_get_allocated_height(GTK_WIDGET(self));
		const double upper = std::max(priv->editor->get_total_lines() * priv->line_height, page_size);
		const double max_value = std::max(upper - page_size, 0.0);
		g_object_freeze_notify(G_OBJECT(priv->vadjustment));
		gtk_adjustment_set_page_size(priv->vadjustment, page_size);
		gtk_adjustment_set_upper(priv->vadjustment, upper);
		if (gtk_adjustment_get_value(priv->vadjustment) > max_value) {
			gtk_adjustment_set_value(priv->vadjustment, max_value);
		}
		g_object_thaw_notify(G_OBJECT(priv->vadjustment));
	}
}

static void platon_editor_widget_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(object);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	switch (property_id) {
	case PROP_HADJUSTMENT:
		g_value_set_object(value, priv->hadjustment);
		break;
	case PROP_VADJUSTMENT:
		g_value_set_object(value, priv->vadjustment);
		break;
	case PROP_HSCROLL_POLICY:
		g_value_set_enum(value, priv->hscroll_policy);
		break;
	case PROP_VSCROLL_POLICY:
		g_value_set_enum(value, priv->vscroll_policy);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void platon_editor_widget_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(object);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	switch (property_id) {
	case PROP_HADJUSTMENT:
		priv->hadjustment = GTK_ADJUSTMENT(g_value_get_object(value));
		break;
	case PROP_VADJUSTMENT:
		priv->vadjustment = GTK_ADJUSTMENT(g_value_get_object(value));
		update(self);
		break;
	case PROP_HSCROLL_POLICY:
		priv->hscroll_policy = (GtkScrollablePolicy)g_value_get_enum(value);
		break;
	case PROP_VSCROLL_POLICY:
		priv->vscroll_policy = (GtkScrollablePolicy)g_value_get_enum(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void platon_editor_widget_realize(GtkWidget* widget) {
	gtk_widget_set_realized(widget, TRUE);
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	GdkWindowAttr attributes;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
	GdkWindow* window = gdk_window_new(gtk_widget_get_parent_window(widget), &attributes, GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL);
	gtk_widget_register_window(widget, window);
	gtk_widget_set_window(widget, window);
}

static void platon_editor_widget_unrealize(GtkWidget* widget) {
	GdkWindow* window = gtk_widget_get_window(widget);
	gtk_widget_unregister_window(widget, window);
	gdk_window_destroy(window);
	gtk_widget_set_realized(widget, FALSE);
}

static void platon_editor_widget_size_allocate(GtkWidget* widget, GtkAllocation* allocation) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(widget);
	gtk_widget_set_allocation(widget, allocation);
	if (gtk_widget_get_realized(widget)) {
		gdk_window_move_resize(gtk_widget_get_window(widget), allocation->x, allocation->y, allocation->width, allocation->height);
	}
	update(self);
}

static void set_source(cairo_t* cr, const Color& color) {
	cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
}

static PangoLayout* create_layout(PlatonEditorWidget* self, const Theme& theme, const RenderedLine& line) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	PangoLayout* layout = pango_layout_new(gtk_widget_get_pango_context(GTK_WIDGET(self)));
	pango_layout_set_font_description(layout, priv->font_description);
	pango_layout_set_text(layout, line.text.c_str(), -1);
	PangoAttrList* attrs = pango_attr_list_new();
	for (const Span& span: line.spans) {
		const Style& style = theme.styles[span.style - Style::DEFAULT];
		PangoAttribute* attr = pango_attr_foreground_new(style.color.r * G_MAXUINT16, style.color.g * G_MAXUINT16, style.color.b * G_MAXUINT16);
		attr->start_index = span.start;
		attr->end_index = span.end;
		pango_attr_list_insert(attrs, attr);
		attr = pango_attr_foreground_alpha_new(style.color.a * G_MAXUINT16);
		attr->start_index = span.start;
		attr->end_index = span.end;
		pango_attr_list_insert(attrs, attr);
		if (style.bold) {
			attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
			attr->start_index = span.start;
			attr->end_index = span.end;
			pango_attr_list_insert(attrs, attr);
		}
		if (style.italic) {
			attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
			attr->start_index = span.start;
			attr->end_index = span.end;
			pango_attr_list_insert(attrs, attr);
		}
	}
	pango_layout_set_attributes(layout, attrs);
	pango_attr_list_unref(attrs);
	return layout;
}

static double index_to_x(PangoLayoutLine* layout_line, std::size_t index) {
	int x_pos;
	pango_layout_line_index_to_x(layout_line, index, false, &x_pos);
	return pango_units_to_double(x_pos);
}

static std::size_t x_to_index(const char* text, PangoLayoutLine* layout_line, double x) {
	int index, trailing;
	pango_layout_line_x_to_index(layout_line, pango_units_from_double(x), &index, &trailing);
	const char* pointer = text + index;
	for (; trailing > 0; trailing--) {
		pointer = g_utf8_next_char(pointer);
	}
	return pointer - text;
}

static std::size_t x_to_index(PangoLayout* layout, double x) {
	const char* text = pango_layout_get_text(layout);
	PangoLayoutLine* layout_line = pango_layout_get_line_readonly(layout, 0);
	return x_to_index(text, layout_line, x);
}

static gboolean platon_editor_widget_draw(GtkWidget* widget, cairo_t* cr) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(widget);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	const double allocated_height = gtk_widget_get_allocated_height(widget);
	const double vadjustment = gtk_adjustment_get_value(priv->vadjustment);
	const double max_row = priv->editor->get_total_lines();
	const size_t start_row = std::clamp(std::floor(vadjustment / priv->line_height), 0.0, max_row);
	const size_t end_row = std::clamp(std::ceil((allocated_height + vadjustment) / priv->line_height), 0.0, max_row);
	const Theme& theme = priv->editor->get_theme();
	const auto lines = priv->editor->render(start_row, end_row);
	// background
	set_source(cr, theme.background);
	cairo_paint(cr);
	for (size_t row = start_row; row < end_row; ++row) {
		const double y = row * priv->line_height - vadjustment;
		PangoLayout* layout = create_layout(self, theme, lines[row - start_row]);
		PangoLayoutLine* layout_line = pango_layout_get_line_readonly(layout, 0);
		// selections
		set_source(cr, theme.selection);
		for (const Range& selection: lines[row - start_row].selections) {
			const double x = index_to_x(layout_line, selection.start);
			const double width = index_to_x(layout_line, selection.end) - x;
			cairo_rectangle(cr, x, y, width, priv->line_height);
			cairo_fill(cr);
		}
		// text
		cairo_move_to(cr, 0.0, y + priv->ascent);
		set_source(cr, theme.styles[0].color);
		pango_cairo_show_layout_line(cr, layout_line);
		// cursors
		set_source(cr, theme.cursor);
		for (std::size_t cursor: lines[row - start_row].cursors) {
			const double x = index_to_x(layout_line, cursor);
			cairo_rectangle(cr, x - 1.0, y, 2.0, priv->line_height);
			cairo_fill(cr);
		}
		g_object_unref(layout);
	}
	return GDK_EVENT_STOP;
}

static gboolean platon_editor_widget_key_press_event(GtkWidget* widget, GdkEventKey* event) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(widget);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	if (GTK_WIDGET_CLASS(platon_editor_widget_parent_class)->key_press_event(widget, event) || gtk_im_context_filter_keypress(priv->im_context, event)) {
		return GDK_EVENT_STOP;
	}
	return GDK_EVENT_PROPAGATE;
}

static gboolean platon_editor_widget_key_release_event(GtkWidget* widget, GdkEventKey* event) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(widget);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	if (GTK_WIDGET_CLASS(platon_editor_widget_parent_class)->key_release_event(widget, event) || gtk_im_context_filter_keypress(priv->im_context, event)) {
		return GDK_EVENT_STOP;
	}
	return GDK_EVENT_PROPAGATE;
}

static void handle_commit(GtkIMContext* im_context, gchar* text, gpointer user_data) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(user_data);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->insert_text(text);
	update(self);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void handle_pressed(GtkGestureMultiPress* multipress_gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(user_data);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	const double vadjustment = gtk_adjustment_get_value(priv->vadjustment);
	gtk_widget_grab_focus(GTK_WIDGET(self));
	GdkEventSequence* sequence = gtk_gesture_single_get_current_sequence(GTK_GESTURE_SINGLE(multipress_gesture));
	const GdkEvent* event = gtk_gesture_get_last_event(GTK_GESTURE(multipress_gesture), sequence);
	GdkModifierType state;
	gdk_event_get_state(event, &state);
	const bool modify_selection = state & gtk_widget_get_modifier_mask(GTK_WIDGET(self), GDK_MODIFIER_INTENT_MODIFY_SELECTION);
	const bool extend_selection = state & gtk_widget_get_modifier_mask(GTK_WIDGET(self), GDK_MODIFIER_INTENT_EXTEND_SELECTION);
	const std::size_t line = std::max((y + vadjustment) / priv->line_height, 0.0);
	if (line < priv->editor->get_total_lines()) {
		PangoLayout* layout = create_layout(self, priv->editor->get_theme(), priv->editor->render(line, line + 1)[0]);
		const std::size_t column = x_to_index(layout, x);
		g_object_unref(layout);
		if (extend_selection) {
			priv->editor->extend_selection(column, line);
		}
		else {
			if (modify_selection) {
				priv->editor->toggle_cursor(column, line);
			}
			else {
				priv->editor->set_cursor(column, line);
			}
		}
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}

static void handle_released(GtkGestureMultiPress* multipress_gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {

}

static void handle_drag_update(GtkGestureDrag* drag_gesture, gdouble offset_x, gdouble offset_y, gpointer user_data) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(user_data);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	const double vadjustment = gtk_adjustment_get_value(priv->vadjustment);
	double start_x, start_y;
	gtk_gesture_drag_get_start_point(drag_gesture, &start_x, &start_y);
	const std::size_t line = std::max((start_y + offset_y + vadjustment) / priv->line_height, 0.0);
	if (line < priv->editor->get_total_lines()) {
		PangoLayout* layout = create_layout(self, priv->editor->get_theme(), priv->editor->render(line, line + 1)[0]);
		const std::size_t column = x_to_index(layout, start_x + offset_x);
		g_object_unref(layout);
		priv->editor->extend_selection(column, line);
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}

static void platon_editor_widget_insert_newline(PlatonEditorWidget* self) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->insert_newline();
	update(self);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_delete_backward(PlatonEditorWidget* self) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->delete_backward();
	update(self);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_delete_forward(PlatonEditorWidget* self) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->delete_forward();
	update(self);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_left(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_left(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_right(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_right(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_up(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_up(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_down(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_down(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_to_beginning_of_line(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_to_beginning_of_line(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_move_to_end_of_line(PlatonEditorWidget* self, gboolean extend_selection) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	priv->editor->move_to_end_of_line(extend_selection);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void platon_editor_widget_dispose(GObject* object) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(object);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	g_clear_object(&priv->drag_gesture);
	g_clear_object(&priv->multipress_gesture);
	g_clear_object(&priv->im_context);
	G_OBJECT_CLASS(platon_editor_widget_parent_class)->dispose(object);
}

static void platon_editor_widget_finalize(GObject* object) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(object);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	delete priv->editor;
	G_OBJECT_CLASS(platon_editor_widget_parent_class)->finalize(object);
}

static void platon_editor_widget_class_init(PlatonEditorWidgetClass* klass) {
	G_OBJECT_CLASS(klass)->dispose = platon_editor_widget_dispose;
	G_OBJECT_CLASS(klass)->finalize = platon_editor_widget_finalize;
	G_OBJECT_CLASS(klass)->get_property = platon_editor_widget_get_property;
	G_OBJECT_CLASS(klass)->set_property = platon_editor_widget_set_property;
	g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_HADJUSTMENT, "hadjustment");
	g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_VADJUSTMENT, "vadjustment");
	g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_HSCROLL_POLICY, "hscroll-policy");
	g_object_class_override_property(G_OBJECT_CLASS(klass), PROP_VSCROLL_POLICY, "vscroll-policy");
	GTK_WIDGET_CLASS(klass)->realize = platon_editor_widget_realize;
	GTK_WIDGET_CLASS(klass)->unrealize = platon_editor_widget_unrealize;
	GTK_WIDGET_CLASS(klass)->size_allocate = platon_editor_widget_size_allocate;
	GTK_WIDGET_CLASS(klass)->draw = platon_editor_widget_draw;
	GTK_WIDGET_CLASS(klass)->key_press_event = platon_editor_widget_key_press_event;
	GTK_WIDGET_CLASS(klass)->key_release_event = platon_editor_widget_key_release_event;
	klass->insert_newline = platon_editor_widget_insert_newline;
	klass->delete_backward = platon_editor_widget_delete_backward;
	klass->delete_forward = platon_editor_widget_delete_forward;
	klass->move_left = platon_editor_widget_move_left;
	klass->move_right = platon_editor_widget_move_right;
	klass->move_up = platon_editor_widget_move_up;
	klass->move_down = platon_editor_widget_move_down;
	klass->move_to_beginning_of_line = platon_editor_widget_move_to_beginning_of_line;
	klass->move_to_end_of_line = platon_editor_widget_move_to_end_of_line;
	g_signal_new("insert-newline", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, insert_newline), NULL, NULL, NULL, G_TYPE_NONE, 0);
	g_signal_new("delete-backward", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, delete_backward), NULL, NULL, NULL, G_TYPE_NONE, 0);
	g_signal_new("delete-forward", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, delete_forward), NULL, NULL, NULL, G_TYPE_NONE, 0);
	g_signal_new("move-left", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_left), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	g_signal_new("move-right", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_right), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	g_signal_new("move-up", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_up), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	g_signal_new("move-down", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_down), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	g_signal_new("move-to-beginning-of-line", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_to_beginning_of_line), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	g_signal_new("move-to-end-of-line", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(PlatonEditorWidgetClass, move_to_end_of_line), NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	GtkBindingSet* binding_set = gtk_binding_set_by_class(klass);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Return, (GdkModifierType)0, "insert-newline", 0);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_BackSpace, (GdkModifierType)0, "delete-backward", 0);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Delete, (GdkModifierType)0, "delete-forward", 0);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Left, (GdkModifierType)0, "move-left", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Left, GDK_SHIFT_MASK, "move-left", 1, G_TYPE_BOOLEAN, TRUE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Right, (GdkModifierType)0, "move-right", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Right, GDK_SHIFT_MASK, "move-right", 1, G_TYPE_BOOLEAN, TRUE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Up, (GdkModifierType)0, "move-up", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Up, GDK_SHIFT_MASK, "move-up", 1, G_TYPE_BOOLEAN, TRUE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Down, (GdkModifierType)0, "move-down", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Down, GDK_SHIFT_MASK, "move-down", 1, G_TYPE_BOOLEAN, TRUE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Home, (GdkModifierType)0, "move-to-beginning-of-line", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_Home, GDK_SHIFT_MASK, "move-to-beginning-of-line", 1, G_TYPE_BOOLEAN, TRUE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_End, (GdkModifierType)0, "move-to-end-of-line", 1, G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal(binding_set, GDK_KEY_End, GDK_SHIFT_MASK, "move-to-end-of-line", 1, G_TYPE_BOOLEAN, TRUE);
}

static void platon_editor_widget_init(PlatonEditorWidget* self) {
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	GSettings* settings = g_settings_new("org.gnome.desktop.interface");
	gchar* monospace_font_name = g_settings_get_string(settings, "monospace-font-name");
	g_object_unref(settings);
	priv->font_description = pango_font_description_from_string(monospace_font_name);
	g_free(monospace_font_name);
	PangoFontMetrics* metrics = pango_context_get_metrics(gtk_widget_get_pango_context(GTK_WIDGET(self)), priv->font_description, NULL);
	double font_size = pango_units_to_double(pango_font_description_get_size(priv->font_description));
	if (!pango_font_description_get_size_is_absolute(priv->font_description)) {
		font_size = font_size / 72.0 * 96.0;
	}
	const double ascent = pango_units_to_double(pango_font_metrics_get_ascent(metrics));
	const double descent = pango_units_to_double(pango_font_metrics_get_descent(metrics));
	priv->line_height = std::round(font_size * LINE_HEIGHT_FACTOR);
	priv->ascent = std::round(ascent + (priv->line_height - (ascent + descent)) / 2.0);
	priv->char_width = pango_units_to_double(pango_font_metrics_get_approximate_char_width(metrics));
	pango_font_metrics_unref(metrics);
	priv->im_context = gtk_im_multicontext_new();
	g_signal_connect_object(priv->im_context, "commit", G_CALLBACK(handle_commit), self, G_CONNECT_DEFAULT);
	priv->multipress_gesture = gtk_gesture_multi_press_new(GTK_WIDGET(self));
	g_signal_connect_object(priv->multipress_gesture, "pressed", G_CALLBACK(handle_pressed), self, G_CONNECT_DEFAULT);
	g_signal_connect_object(priv->multipress_gesture, "released", G_CALLBACK(handle_released), self, G_CONNECT_DEFAULT);
	priv->drag_gesture = gtk_gesture_drag_new(GTK_WIDGET(self));
	g_signal_connect_object(priv->drag_gesture, "drag_update", G_CALLBACK(handle_drag_update), self, G_CONNECT_DEFAULT);
	gtk_widget_set_can_focus(GTK_WIDGET(self), TRUE);
	gtk_widget_add_events(GTK_WIDGET(self), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
}

PlatonEditorWidget* platon_editor_widget_new(GFile* file) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(g_object_new(PLATON_TYPE_EDITOR_WIDGET, NULL));
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	if (file) {
		gchar* path = g_file_get_path(file);
		priv->editor = new Editor(path);
		g_free(path);
	}
	else {
		priv->editor = new Editor();
	}
	return self;
}
