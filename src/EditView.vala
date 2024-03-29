class EditView: Gtk.DrawingArea, Gtk.Scrollable {
	private const double PADDING = 12;
	private File file;
	private Platon.Editor editor;
	private Theme theme;
	private Gtk.IMContext im_context;
	private Gtk.GestureMultiPress multipress_gesture;
	private Gtk.GestureDrag drag_gesture;
	private Pango.FontDescription font_description;
	private double ascent;
	private double line_height;
	private double gutter_width;
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

	public EditView(File? file) {
		this.file = file;
		if (file != null) {
			editor = new Platon.Editor.from_file(file.get_path());
		} else {
			editor = new Platon.Editor();
		}
		theme = new Theme(Json.from_string(editor.get_theme()).get_object());
		im_context = new Gtk.IMMulticontext();
		im_context.commit.connect(commit);
		multipress_gesture = new Gtk.GestureMultiPress(this);
		multipress_gesture.pressed.connect(handle_pressed);
		drag_gesture = new Gtk.GestureDrag(this);
		drag_gesture.drag_update.connect(handle_drag_update);
		get_style_context().add_class(Gtk.STYLE_CLASS_MONOSPACE);
		var settings = new Settings("org.gnome.desktop.interface");
		font_description = Pango.FontDescription.from_string(settings.get_string("monospace-font-name"));
		var metrics = get_pango_context().get_metrics(font_description, null);
		ascent = Pango.units_to_double(metrics.get_ascent());
		line_height = Pango.units_to_double(metrics.get_ascent() + metrics.get_descent());
		can_focus = true;
		add_events(Gdk.EventMask.SMOOTH_SCROLL_MASK);
	}

	private static uint get_digits(size_t number) {
		uint digits = 1;
		while (number >= 10) {
			digits += 1;
			number /= 10;
		}
		return digits;
	}

	private size_t get_line(double y) {
		return (size_t)double.max((y - PADDING) / line_height, 0);
	}

	private size_t get_column(double x, size_t line) {
		if (line - first_line >= lines.length) {
			return 0;
		}
		return lines[line - first_line].x_to_index(x - (PADDING * 2 + gutter_width + PADDING * 2));
	}

	private void update_gutter_width() {
		uint digits = get_digits(editor.get_total_lines());
		var layout = new Pango.Layout(get_pango_context());
		layout.set_text("%0*d".printf(digits, 0), -1);
		layout.set_font_description(font_description);
		Pango.Rectangle extents;
		layout.get_line_readonly(0).get_pixel_extents(null, out extents);
		gutter_width = extents.width;
	}

	private void update(bool update_lines = true) {
		size_t total_lines = editor.get_total_lines();
		update_gutter_width();
		_vadjustment.upper = double.max(total_lines * line_height + PADDING * 2, get_allocated_height());
		if (_vadjustment.value > _vadjustment.upper - _vadjustment.page_size) {
			// this will cause update to be called recursively
			_vadjustment.value = _vadjustment.upper - _vadjustment.page_size;
		}
		size_t new_first_line = size_t.min(get_line(_vadjustment.value), total_lines);
		size_t new_last_line = size_t.min(get_line(_vadjustment.value + get_allocated_height()) + 1, total_lines);
		if (new_first_line != first_line || new_last_line != last_line) {
			first_line = new_first_line;
			last_line = new_last_line;
			lines.resize((int)(last_line - first_line));
			update_lines = true;
		}
		if (update_lines) {
			unowned string json = editor.render(first_line, last_line);
			var json_lines = Json.from_string(json).get_array();
			for (uint i = 0; i < lines.length; ++i) {
				var json_line = json_lines.get_object_element(i);
				lines[i] = new Line(get_pango_context(), font_description, json_line, theme);
			}
			queue_draw();
		}
	}

	public override bool draw(Cairo.Context cr) {
		Gdk.cairo_set_source_rgba(cr, theme.background);
		cr.paint();
		double x = PADDING * 2 + gutter_width + PADDING * 2;
		double line_x = PADDING * 2 + gutter_width;
		Gdk.cairo_set_source_rgba(cr, theme.number_background);
		cr.rectangle(0, 0, x, get_allocated_height());
		cr.fill();
		for (uint i = 0; i < lines.length; ++i) {
			double y = PADDING + (first_line + i) * line_height - _vadjustment.value;
			lines[i].draw_background(cr, x, y, get_allocated_width() - x, line_height, theme);
			lines[i].draw_number_background(cr, y, x, line_height, theme);
		}
		for (uint i = 0; i < lines.length; ++i) {
			double y = PADDING + (first_line + i) * line_height - _vadjustment.value;
			lines[i].draw(cr, x, y + ascent, theme);
			lines[i].draw_cursors(cr, x, y, line_height, theme);
			lines[i].draw_number(cr, line_x, y + ascent, theme);
		}
		return Gdk.EVENT_STOP;
	}

	private void scroll() {
		update(false);
	}

	public override void size_allocate(Gtk.Allocation allocation) {
		base.size_allocate(allocation);
		_vadjustment.page_size = allocation.height;
		update(false);
	}

	private void commit(string text) {
		editor.insert_text(text);
		update();
	}
	public override bool key_press_event(Gdk.EventKey event) {
		if (base.key_press_event(event) || im_context.filter_keypress(event)) {
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.BackSpace) {
			editor.delete_backward();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Delete) {
			editor.delete_forward();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Return) {
			editor.insert_newline();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Left) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			if ((event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
				editor.move_to_beginning_of_word(extend_selection);
			}
			else {
				editor.move_left(extend_selection);
			}
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Right) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			if ((event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
				editor.move_to_end_of_word(extend_selection);
			}
			else {
				editor.move_right(extend_selection);
			}
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Up) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			editor.move_up(extend_selection);
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Down) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			editor.move_down(extend_selection);
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.Home) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			editor.move_to_beginning_of_line(extend_selection);
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.End) {
			bool extend_selection = (event.state & get_modifier_mask(Gdk.ModifierIntent.EXTEND_SELECTION)) != 0;
			editor.move_to_end_of_line(extend_selection);
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.a && (event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			editor.select_all();
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.c && (event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			get_clipboard(Gdk.SELECTION_CLIPBOARD).set_text(editor.copy(), -1);
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.x && (event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			get_clipboard(Gdk.SELECTION_CLIPBOARD).set_text(editor.cut(), -1);
			update();
			return Gdk.EVENT_STOP;
		}
		if (event.keyval == Gdk.Key.v && (event.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			get_clipboard(Gdk.SELECTION_CLIPBOARD).request_text((clipboard, text) => {
				if (text != null) {
					editor.paste(text);
					update();
				}
			});
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

	private void handle_pressed(int n_press, double x, double y) {
		grab_focus();
		var event = multipress_gesture.get_last_event(multipress_gesture.get_current_sequence());
		Gdk.ModifierType state;
		event.get_state(out state);
		size_t line = get_line(y + _vadjustment.value);
		size_t column = get_column(x, line);
		bool modify_selection = (state & get_modifier_mask(Gdk.ModifierIntent.MODIFY_SELECTION)) != 0;
		if (modify_selection)
			editor.toggle_cursor(column, line);
		else
			editor.set_cursor(column, line);
		update();
	}

	private void handle_drag_update(double offset_x, double offset_y) {
		double start_x, start_y;
		drag_gesture.get_start_point(out start_x, out start_y);
		size_t line = get_line(start_y + offset_y + _vadjustment.value);
		size_t column = get_column(start_x + offset_x, line);
		editor.extend_selection(column, line);
		update();
	}

	public bool save() {
		if (file == null) {
			return false;
		}
		editor.save(file.get_path());
		return true;
	}
	public void save_as(File file) {
		this.file = file;
		editor.save(file.get_path());
	}
}
