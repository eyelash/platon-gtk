class Theme: Object {
	public struct Style {
		Gdk.RGBA color;
		Pango.Weight weight;
		bool italic;
	}
	public Gdk.RGBA background;
	public Gdk.RGBA background_active;
	public Gdk.RGBA selection;
	public Gdk.RGBA cursor;
	public Gdk.RGBA number_background;
	public Gdk.RGBA number_background_active;
	public Style number;
	public Style number_active;
	public Style[] styles;
	private static Gdk.RGBA decode_color(Json.Array color) {
		int64 r = color.get_int_element(0);
		int64 g = color.get_int_element(1);
		int64 b = color.get_int_element(2);
		int64 a = color.get_int_element(3);
		return Gdk.RGBA() {
			red = r / 255.0,
			green = g / 255.0,
			blue = b / 255.0,
			alpha = a / 255.0
		};
	}
	private static Gdk.RGBA get_color(Json.Object theme, string color_name) {
		if (!theme.has_member(color_name)) {
			return Gdk.RGBA();
		}
		var color = theme.get_array_member(color_name);
		return decode_color(color);
	}
	private static Style get_style(Json.Object style) {
		return Style() {
			color = decode_color(style.get_array_member("color")),
			weight = (Pango.Weight)style.get_int_member("weight"),
			italic = style.get_boolean_member("italic")
		};
	}

	public Theme(Json.Object theme) {
		background = get_color(theme, "background");
		background_active = get_color(theme, "background_active");
		selection = get_color(theme, "selection");
		cursor = get_color(theme, "cursor");
		number_background = get_color(theme, "number_background");
		number_background_active = get_color(theme, "number_background_active");
		number = get_style(theme.get_object_member("number"));
		number_active = get_style(theme.get_object_member("number_active"));
		var json_styles = theme.get_array_member("styles");
		styles.resize((int)json_styles.get_length());
		for (uint i = 0; i < json_styles.get_length(); i++) {
			styles[i] = get_style(json_styles.get_object_element(i));
		}
	}
}
