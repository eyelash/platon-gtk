class Window: Gtk.ApplicationWindow {
	public Window(Gtk.Application application) {
		Object(application: application);
		set_default_size(800, 550);
	}
	public void add_edit_view(EditView edit_view) {
		var scrolled_window = new Gtk.ScrolledWindow(null, null);
		scrolled_window.add(edit_view);
		scrolled_window.show_all();
		add(scrolled_window);
	}
}
