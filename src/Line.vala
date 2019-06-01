class Line: Object {
	private Pango.Layout layout;
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

	public Line(Pango.Context pango_context, Json.Object json_line, double char_width, Theme theme) {
		layout = new Pango.Layout(pango_context);
		layout.set_text(json_line.get_string_member("text"), -1);
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
			set_color(attributes, span_start, span_end, theme.text[index]);
		}
		layout.set_attributes(attributes);
	}

	public void draw(Cairo.Context cr, double y, double line_height, Theme theme) {
		Gdk.cairo_set_source_rgba(cr, theme.text[0]);
		cr.move_to(0, y);
		Pango.cairo_show_layout(cr, layout);
		foreach (double cursor in cursors) {
			Gdk.cairo_set_source_rgba(cr, theme.cursor);
			cr.rectangle(cursor - 1, y, 2, line_height);
			cr.fill();
		}
	}
}
