#include "editor_widget.h"
#include "core/editor.hpp"

typedef struct {
	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;
	GtkScrollablePolicy hscroll_policy;
	GtkScrollablePolicy vscroll_policy;
	Editor* editor;
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
	GTK_WIDGET_CLASS(platon_editor_widget_parent_class)->realize(widget);
}

static void platon_editor_widget_unrealize(GtkWidget* widget) {
	GTK_WIDGET_CLASS(platon_editor_widget_parent_class)->unrealize(widget);
}

static void platon_editor_widget_size_allocate(GtkWidget* widget, GtkAllocation* allocation) {
	GTK_WIDGET_CLASS(platon_editor_widget_parent_class)->size_allocate(widget, allocation);
}

static void set_source(cairo_t* cr, const Color& color) {
	cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
}

static gboolean platon_editor_widget_draw(GtkWidget* widget, cairo_t* cr) {
	PlatonEditorWidget* self = PLATON_EDITOR_WIDGET(widget);
	PlatonEditorWidgetPrivate* priv = (PlatonEditorWidgetPrivate*)platon_editor_widget_get_instance_private(self);
	const Theme& theme = priv->editor->get_theme();
	set_source(cr, theme.background);
	cairo_paint(cr);
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
	gtk_widget_set_has_window(GTK_WIDGET(self), FALSE);
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
