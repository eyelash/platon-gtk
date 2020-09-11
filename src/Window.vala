class Window: Gtk.ApplicationWindow {
	private Gtk.FileChooserNative dialog;
	public Window(Gtk.Application application) {
		Object(application: application);
		set_default_size(800, 550);
		var save_action = new SimpleAction("save", null);
		save_action.activate.connect(save);
		add_action(save_action);
		var header_bar = new Gtk.HeaderBar();
		header_bar.show_close_button = true;
		header_bar.title = "Platon";
		var save_button = new Gtk.Button.from_icon_name("document-save-symbolic", Gtk.IconSize.BUTTON);
		save_button.tooltip_text = "Save File";
		save_button.action_name = "win.save";
		header_bar.pack_end(save_button);
		header_bar.show_all();
		set_titlebar(header_bar);
	}
	private unowned EditView get_edit_view() {
		return (get_child() as Gtk.ScrolledWindow).get_child() as EditView;
	}
	private void save() {
		if (!get_edit_view().save()) {
			save_as();
		}
	}
	private void save_as() {
		dialog = new Gtk.FileChooserNative(null, this, Gtk.FileChooserAction.SAVE, null, null);
		dialog.do_overwrite_confirmation = true;
		dialog.response.connect((response) => {
			if (response == Gtk.ResponseType.ACCEPT) {
				get_edit_view().save_as(dialog.get_file());
			}
			dialog.destroy();
		});
		dialog.show();
	}
	public void open_file(File? file) {
		var scrolled_window = new Gtk.ScrolledWindow(null, null);
		scrolled_window.add(new EditView(file));
		scrolled_window.show_all();
		add(scrolled_window);
	}
}
