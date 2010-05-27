
using Gtk;
using Gdk;
using GtkClutter;
using Clutter;

const int DEFAULT_WINDOW_WIDTH = 600;
const int DEFAULT_WINDOW_HEIGHT = 450;

public class Cheese.MainWindow : Gtk.Window {

	private Widget thumbnails;
	private GtkClutter.Embed viewport_widget;
	private Clutter.Stage viewport;
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
		builder.add_from_file (GLib.Path.build_filename ("../data/", "cheese.ui"));
		builder.connect_signals(this);
		
		main_vbox = (VBox) builder.get_object ("mainbox_normal");
		thumbnails = (Widget) builder.get_object("thumbnails");
		viewport_widget = (GtkClutter.Embed) builder.get_object("viewport");
		viewport = (Clutter.Stage) viewport_widget.get_stage();

		Clutter.Rectangle r = new Clutter.Rectangle();
		r.width = 200;
		r.height = 600;
		r.x = 0;
		r.y = 0;
		r.color = Clutter.Color.from_string("Red");
		viewport.add_actor(r);
					
		viewport.show_all();
		this.add(main_vbox);
			
	}
}
