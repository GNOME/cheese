using Gtk;
using Gdk;

public class Cheese.MainWindow : Gtk.Window {

	[CCode (instance_pos = -1)]
	internal void on_quit (Action action ) {
		destroy();
	}


	[CCode (instance_pos = -1)]
	internal void on_help_contents (Action action ) {
		Gdk.Screen screen;
		screen = this.get_screen();
		Gtk.show_uri(screen, "ghelp:cheese", Gtk.get_current_event_time());
	}

	
	public	void setup_ui () {
		Builder builder = new Builder();
		VBox main_vbox;
		builder.add_from_file (Path.build_filename ("../data/", "cheese.ui"));
		builder.connect_signals(this);
		
		main_vbox = (VBox) builder.get_object ("mainbox_normal");
		
		this.add(main_vbox);
			
	}
}
