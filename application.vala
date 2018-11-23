class EditView: Gtk.DrawingArea, Gtk.Scrollable {
	private Platon.Editor editor;
	private double char_width;
	private double line_height;
	private Pango.Layout[] lines;
	private size_t first_line;
	private size_t last_line;
	
	// Gtk.Scrollable implementation
	public Gtk.Adjustment hadjustment { construct set; get; }
	public Gtk.ScrollablePolicy hscroll_policy { set; get; }
	private Gtk.Adjustment _vadjustment;
	public Gtk.Adjustment vadjustment {
		construct set {
			_vadjustment = value;
			if (value != null) {
				value.value_changed.connect(update);
				value.upper = editor.get_total_lines() * line_height;
			}
		}
		get {
			return _vadjustment;
		}
	}
	public Gtk.ScrollablePolicy vscroll_policy { set; get; }
	public bool get_border(out Gtk.Border border) {
		border = Gtk.Border();
		return false;
	}
	
	public EditView(string path) {
		editor = new Platon.Editor(path);
		get_style_context().add_class(Gtk.STYLE_CLASS_MONOSPACE);
		var metrics = get_pango_context().get_metrics(get_pango_context().get_font_description(), null);
		char_width = Pango.units_to_double(metrics.get_approximate_char_width());
		line_height = Pango.units_to_double(metrics.get_ascent() + metrics.get_descent());
		add_events(Gdk.EventMask.SMOOTH_SCROLL_MASK);
	}
	
	private void update() {
		size_t new_first_line = (size_t)(_vadjustment.value / line_height);
		size_t new_last_line = (size_t)((_vadjustment.value + get_allocated_height()) / line_height) + 1;
		if (new_first_line == first_line && new_last_line == last_line) {
			return;
		}
		first_line = new_first_line;
		last_line = new_last_line;
		lines.resize((int)(last_line - first_line));
		unowned string json = editor.render(first_line, last_line);
		var array = Json.from_string(json).get_array();
		for (size_t i = 0; i < last_line - first_line; ++i) {
			var line = array.get_object_element((uint)i);
			var layout = new Pango.Layout(get_pango_context());
			layout.set_text(line.get_string_member("text"), -1);
			lines[i] = layout;
		}
	}

	public override bool draw(Cairo.Context cr) {
		for (size_t i = 0; i < last_line - first_line; ++i) {
			double y = (first_line + i) * line_height - _vadjustment.value;
			cr.move_to(0, y);
			Pango.cairo_show_layout(cr, lines[i]);
		}
		return Gdk.EVENT_STOP;
	}

	public override void size_allocate(Gtk.Allocation allocation) {
		base.size_allocate(allocation);
		_vadjustment.page_size = allocation.height;
		update();
	}
}

class Window: Gtk.ApplicationWindow {
	public Window(Gtk.Application application) {
		Object(application: application);
		set_default_size(800, 550);
	}
	public void open_file(File file) {
		var scrolled_window = new Gtk.ScrolledWindow(null, null);
		scrolled_window.add(new EditView(file.get_path()));
		scrolled_window.show_all();
		add(scrolled_window);
	}
}

class Application: Gtk.Application {
	public Application() {
		Object(flags: ApplicationFlags.HANDLES_OPEN);
	}
	public override void activate() {
		new Window(this).present();
	}
	public override void open(File[] files, string hint) {
		foreach (var file in files) {
			var window = new Window(this);
			window.open_file(file);
			window.present();
		}
	}
	public static int main(string[] args) {
		return new Application().run(args);
	}
}
