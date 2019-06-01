class Theme: Object {
	public Gdk.RGBA background;
	public Gdk.RGBA cursor;
	public Gdk.RGBA[] text;
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
	public Theme(Json.Object theme) {
		background = get_color(theme, "background");
		cursor = get_color(theme, "cursor");
		var json_text = theme.get_array_member("text");
		text.resize((int)json_text.get_length());
		for (uint i = 0; i < json_text.get_length(); i++) {
			text[i] = decode_color(json_text.get_array_element(i));
		}
	}
}
