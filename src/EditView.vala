class EditView: Gtk.DrawingArea, Gtk.Scrollable {
	private Platon.Editor editor;
	private Theme theme;
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
		theme = new Theme(Json.from_string(editor.get_theme()).get_object());
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
			lines[i] = new Line(get_pango_context(), json_line, char_width, theme);
		}
		queue_draw();
	}

	public override bool draw(Cairo.Context cr) {
		Gdk.cairo_set_source_rgba(cr, theme.background);
		cr.paint();
		for (size_t i = 0; i < last_line - first_line; ++i) {
			double y = (first_line + i) * line_height - _vadjustment.value;
			lines[i].draw(cr, y, line_height, theme);
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
		if (event.keyval == Gdk.Key.Left) {
			editor.move_left();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Right) {
			editor.move_right();
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
