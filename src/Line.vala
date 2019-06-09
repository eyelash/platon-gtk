class Line: Object {
	private Pango.Layout layout;
	private Pango.Layout number;
	private double[] cursors;

	private static void set_color(Pango.AttrList attributes, uint start_index, uint end_index, Gdk.RGBA color) {
		var attribute = Pango.attr_foreground_new((uint16)(color.red*uint16.MAX), (uint16)(color.green*uint16.MAX), (uint16)(color.blue*uint16.MAX));
		attribute.start_index = start_index;
		attribute.end_index = end_index;
		attributes.insert((owned)attribute);
		attribute = Pango.attr_foreground_alpha_new((uint16)(color.alpha*uint16.MAX));
		attribute.start_index = start_index;
		attribute.end_index = end_index;
		attributes.insert((owned)attribute);
	}

	private static void set_weight(Pango.AttrList attributes, uint start_index, uint end_index, Pango.Weight weight) {
		var attribute = Pango.attr_weight_new(weight);
		attribute.start_index = start_index;
		attribute.end_index = end_index;
		attributes.insert((owned)attribute);
	}

	private static void set_italic(Pango.AttrList attributes, uint start_index, uint end_index) {
		var attribute = Pango.attr_style_new(Pango.Style.ITALIC);
		attribute.start_index = start_index;
		attribute.end_index = end_index;
		attributes.insert((owned)attribute);
	}

	public Line(Pango.Context pango_context, Json.Object json_line, double char_width, Theme theme) {
		layout = new Pango.Layout(pango_context);
		layout.set_text(json_line.get_string_member("text"), -1);
		number = new Pango.Layout(pango_context);
		number.set_text(json_line.get_int_member("number").to_string(), -1);
		var json_cursors = json_line.get_array_member("cursors");
		cursors.resize((int)json_cursors.get_length());
		for (uint i = 0; i < json_cursors.get_length(); ++i) {
			cursors[i] = json_cursors.get_int_element(i) * char_width;
		}
		var spans = json_line.get_array_member("spans");
		var attributes = new Pango.AttrList();
		for (uint j = 0; j < spans.get_length(); ++j) {
			var span = spans.get_array_element(j);
			uint span_start = (uint)span.get_int_element(0);
			uint span_end = (uint)span.get_int_element(1);
			uint index = (uint)span.get_int_element(2);
			set_color(attributes, span_start, span_end, theme.styles[index].color);
			set_weight(attributes, span_start, span_end, theme.styles[index].weight);
			if (theme.styles[index].italic) {
				set_italic(attributes, span_start, span_end);
			}
		}
		layout.set_attributes(attributes);
	}

	public void draw(Cairo.Context cr, double x, double y, double ascent, double line_height, Theme theme) {
		Gdk.cairo_set_source_rgba(cr, theme.styles[0].color);
		cr.move_to(x, y + ascent);
		Pango.cairo_show_layout_line(cr, layout.get_line_readonly(0));
		Gdk.cairo_set_source_rgba(cr, theme.cursor);
		foreach (double cursor in cursors) {
			cr.rectangle(x + cursor - 1, y, 2, line_height);
			cr.fill();
		}
	}

	public void draw_number(Cairo.Context cr, double x, double y, double ascent, Theme theme) {
		unowned Pango.LayoutLine line = number.get_line_readonly(0);
		Pango.Rectangle extents;
		line.get_pixel_extents(null, out extents);
		if (cursors.length > 0) {
			Gdk.cairo_set_source_rgba(cr, theme.number_active.color);
		} else {
			Gdk.cairo_set_source_rgba(cr, theme.number.color);
		}
		cr.move_to(x - extents.width, y + ascent);
		Pango.cairo_show_layout_line(cr, line);
	}
}
