using Gtk;

public class Cheese.MainWindow : Window {

	[CCode (instance_pos = -1)]
	internal void on_quit (Action action ) {
		destroy();
	}
	
	public	void setup_ui () {
		Builder builder = new Builder();
		VBox main_vbox;
		builder.add_from_file (Path.build_filename ("../data/", "cheese.ui"));
		builder.connect_signals(this);
		
		main_vbox = (VBox) builder.get_object ("mainbox_normal");
		
		add(main_vbox);
			
	}
}
