class Window: Gtk.ApplicationWindow {
	public Window(Gtk.Application application) {
		Object(application: application);
		set_default_size(800, 550);
		var header_bar = new Gtk.HeaderBar();
		header_bar.show_close_button = true;
		header_bar.title = "Platon";
		header_bar.show_all();
		set_titlebar(header_bar);
	}
	public void add_edit_view(EditView edit_view) {
		var scrolled_window = new Gtk.ScrolledWindow(null, null);
		scrolled_window.add(edit_view);
		scrolled_window.show_all();
		add(scrolled_window);
	}
}
