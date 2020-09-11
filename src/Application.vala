class Application: Gtk.Application {
	public Application() {
		Object(flags: ApplicationFlags.HANDLES_OPEN);
	}
	public override void activate() {
		var window = new Window(this);
		window.open_file(null);
		window.present();
	}
	public override void open(File[] files, string hint) {
		foreach (var file in files) {
			var window = new Window(this);
			window.open_file(file);
			window.present();
		}
	}
	public static int main(string[] args) {
		return new Application().run(args);
	}
}
