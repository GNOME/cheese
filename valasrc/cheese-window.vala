using Gtk;
using Gdk;
using GtkClutter;
using Clutter;
using Config;
using Eog;

const int DEFAULT_WINDOW_WIDTH = 600;
const int DEFAULT_WINDOW_HEIGHT = 450;

public class Cheese.MainWindow : Gtk.Window {

	private Gtk.Builder builder;
	
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

	[CCode (instance_pos = -1)]
	internal void on_help_about (Action action) {
		// FIXME: Close doesn't work
		// FIXME: Clicking URL In the License dialog borks.
		Gtk.AboutDialog about_dialog;
		about_dialog = (Gtk.AboutDialog) builder.get_object("aboutdialog");
		about_dialog.version = Config.VERSION;
		about_dialog.show_all();
	}		
	
	public	void setup_ui () {
		builder = new Builder();
		VBox main_vbox;
		builder.add_from_file (GLib.Path.build_filename ("../data/", "cheese-actions.ui"));
		builder.add_from_file (GLib.Path.build_filename ("../data/", "cheese-about.ui"));
		builder.add_from_file (GLib.Path.build_filename ("../data/", "cheese-main-window.ui"));
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
		Cheese.ThumbView tv = new Cheese.ThumbView();
		Eog.ThumbNav tnav = new Eog.ThumbNav(tv, false);
		tv.set_columns(1);
		tnav.set_vertical(true);
		Gtk.Frame tf = (Gtk.Frame) builder.get_object("thumbnails");		
		tf.add(tnav);
		this.add(main_vbox);
			
	}
}
