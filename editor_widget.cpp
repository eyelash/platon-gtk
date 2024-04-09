#include "editor_widget.h"
#include "core/editor.hpp"
#include <cmath>

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
	set_source(cr, theme.background);
	cairo_paint(cr);
	set_source(cr, theme.styles[0].color);
	for (size_t row = start_row; row < end_row; ++row) {
		const double y = row * priv->line_height - vadjustment;
		PangoLayout* layout = pango_layout_new(gtk_widget_get_pango_context(GTK_WIDGET(self)));
		pango_layout_set_font_description(layout, priv->font_description);
		pango_layout_set_text(layout, lines[row - start_row].text.c_str(), -1);
		PangoLayoutLine* layout_line = pango_layout_get_line_readonly(layout, 0);
		cairo_move_to(cr, 0.0, y + priv->ascent);
		pango_cairo_show_layout_line(cr, layout_line);
		g_object_unref(layout);
	}
	return GDK_EVENT_STOP;
}

static void platon_editor_widget_dispose(GObject* object) {
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
