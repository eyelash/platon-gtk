class Application: Gtk.Application {
	public Application() {
		Object(flags: ApplicationFlags.HANDLES_OPEN);
	}
	public override void activate() {
		var editor = new Platon.Editor();
		var edit_view = new EditView((owned)editor);
		var window = new Window(this);
		window.add_edit_view(edit_view);
		window.present();
	}
	public override void open(File[] files, string hint) {
		foreach (var file in files) {
			var editor = new Platon.Editor.from_file(file.get_path());
			var edit_view = new EditView((owned)editor);
			var window = new Window(this);
			window.add_edit_view(edit_view);
			window.present();
		}
	}
	public static int main(string[] args) {
		return new Application().run(args);
	}
}
