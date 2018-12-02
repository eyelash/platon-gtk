[CCode(cheader_filename = "c_api.h")]
namespace Platon {
	[Compact]
	public class Editor {
		public Editor(string path);
		public size_t get_total_lines();
		public unowned string render(size_t first_line, size_t last_line);
		public void insert(string text);
		public void backspace();
		public void set_cursor(size_t column, size_t row);
	}
}
