[CCode(cheader_filename = "c_api.h")]
namespace Platon {
	[Compact]
	public class Editor {
		public Editor();
		public Editor.from_file(string path);
		public size_t get_total_lines();
		public unowned string render(size_t first_line, size_t last_line);
		public void insert_text(string text);
		public void insert_newline();
		public void delete_backward();
		public void delete_forward();
		public void set_cursor(size_t column, size_t line);
		public void toggle_cursor(size_t column, size_t line);
		public void extend_selection(size_t column, size_t line);
		public void move_left(bool extend_selection);
		public void move_right(bool extend_selection);
		public void move_up(bool extend_selection);
		public void move_down(bool extend_selection);
		public void move_to_beginning_of_word(bool extend_selection);
		public void move_to_end_of_word(bool extend_selection);
		public void move_to_beginning_of_line(bool extend_selection);
		public void move_to_end_of_line(bool extend_selection);
		public void select_all();
		public unowned string get_theme();
		public unowned string copy();
		public unowned string cut();
		public void paste(string text);
		public void save(string path);
	}
}
