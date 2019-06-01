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
