class Line: Object {
	private Pango.Layout layout;
	private double[] cursors;

	public Line(Pango.Context pango_context, Json.Object json_line, double char_width) {
		layout = new Pango.Layout(pango_context);
		layout.set_text(json_line.get_string_member("text"), -1);
		var json_cursors = json_line.get_array_member("cursors");
		cursors.resize((int)json_cursors.get_length());
		for (uint i = 0; i < json_cursors.get_length(); ++i) {
			cursors[i] = json_cursors.get_int_element(i) * char_width;
		}
	}

	public void draw(Cairo.Context cr, double y, double line_height) {
		cr.move_to(0, y);
		Pango.cairo_show_layout(cr, layout);
		foreach (double cursor in cursors) {
			cr.rectangle(cursor - 1, y, 2, line_height);
			cr.fill();
		}
	}
}

class EditView: Gtk.DrawingArea, Gtk.Scrollable {
	private Platon.Editor editor;
	private Gtk.IMContext im_context;
	private double char_width;
	private double line_height;
	private Line[] lines;
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
				value.value_changed.connect(scroll);
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
		im_context = new Gtk.IMMulticontext();
		im_context.commit.connect(commit);
		get_style_context().add_class(Gtk.STYLE_CLASS_MONOSPACE);
		var metrics = get_pango_context().get_metrics(get_pango_context().get_font_description(), null);
		char_width = Pango.units_to_double(metrics.get_approximate_char_width());
		line_height = Pango.units_to_double(metrics.get_ascent() + metrics.get_descent());
		can_focus = true;
		add_events(Gdk.EventMask.SMOOTH_SCROLL_MASK | Gdk.EventMask.BUTTON_PRESS_MASK);
	}
	
	private void update() {
		unowned string json = editor.render(first_line, last_line);
		var json_lines = Json.from_string(json).get_array();
		for (size_t i = 0; i < last_line - first_line; ++i) {
			var json_line = json_lines.get_object_element((uint)i);
			lines[i] = new Line(get_pango_context(), json_line, char_width);
		}
		queue_draw();
	}

	public override bool draw(Cairo.Context cr) {
		for (size_t i = 0; i < last_line - first_line; ++i) {
			double y = (first_line + i) * line_height - _vadjustment.value;
			lines[i].draw(cr, y, line_height);
		}
		return Gdk.EVENT_STOP;
	}

	private void scroll() {
		size_t new_first_line = (size_t)(_vadjustment.value / line_height);
		size_t new_last_line = (size_t)((_vadjustment.value + get_allocated_height()) / line_height) + 1;
		if (new_first_line == first_line && new_last_line == last_line) {
			return;
		}
		first_line = new_first_line;
		last_line = new_last_line;
		lines.resize((int)(last_line - first_line));
		update();
	}

	public override void size_allocate(Gtk.Allocation allocation) {
		base.size_allocate(allocation);
		_vadjustment.page_size = allocation.height;
		scroll();
	}

	private void commit(string text) {
		editor.insert(text);
		update();
	}
	public override bool key_press_event(Gdk.EventKey event) {
		if (base.key_press_event(event) || im_context.filter_keypress(event)) {
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.BackSpace) {
			editor.backspace();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Return) {
			editor.insert("\n");
			update();
			return Gdk.EVENT_STOP;
		}
		return Gdk.EVENT_PROPAGATE;
	}
	public override bool key_release_event(Gdk.EventKey event) {
		if (base.key_release_event(event) || im_context.filter_keypress(event)) {
			return Gdk.EVENT_STOP;
		}
		return Gdk.EVENT_PROPAGATE;
	}

	public override bool button_press_event(Gdk.EventButton event) {
		if (focus_on_click && !has_focus) {
			grab_focus();
		}
		size_t column = (size_t)Math.round(event.x / char_width);
		size_t row = (size_t)((event.y + _vadjustment.value) / line_height);
		bool modify_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.MODIFY_SELECTION)) != 0;
		if (modify_selection)
			editor.toggle_cursor(column, row);
		else
			editor.set_cursor(column, row);
		update();
		return Gdk.EVENT_STOP;
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
