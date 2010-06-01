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
	private Eog.ThumbNav thumb_nav;
	private Cheese.ThumbView thumb_view;
	private Gtk.Frame thumbnails_right;
	private Gtk.Frame thumbnails_bottom;
	private Gtk.MenuBar menubar;
	private Gtk.HBox leave_fullscreen_button_container;
	
	private bool is_fullscreen;
	private bool is_wide_mode;
	
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

	[CCode (instance_pos = -1)]
	internal void on_layout_wide_mode (ToggleAction action ) {
		set_wide_mode(action.active);
	}

	[CCode (instance_pos = -1)]
	internal void on_layout_fullscreen (ToggleAction action ) {
		set_fullscreen_mode(action.active);
	}

	private void set_fullscreen_mode(bool fullscreen_mode) {
		// After the first time the window has been shown using this.show_all(),
		// the value of leave_fullscreen_button_container.no_show_all should be set to false
		// So that the next time leave_fullscreen_button_container.show_all() is called, the button is actually shown
		// FIXME: If this code can be made cleaner/clearer, pleasae do
		
		is_fullscreen = fullscreen_mode;
		if (fullscreen_mode) {
			if (is_wide_mode) {
				thumbnails_right.hide_all();
			}
			else {				
				thumbnails_bottom.hide_all();
			}
			menubar.hide_all();
			leave_fullscreen_button_container.no_show_all = false;
			leave_fullscreen_button_container.show_all();
			this.fullscreen();			
		}
		else {
			if (is_wide_mode) {
				thumbnails_right.show_all();
			}
			else {				
				thumbnails_bottom.show_all();
			}
			menubar.show_all();
			leave_fullscreen_button_container.hide_all();
			this.unfullscreen();
		}
	}
	
	private void set_wide_mode(bool wide_mode, bool initialize=false) {
		is_wide_mode = wide_mode;
		if (!initialize) {
			// Sets requested size of the viewport_widget to be it's current size
			// So it does not grow smaller with each toggle
			Gtk.Allocation alloc;
			viewport_widget.get_allocation(out alloc);
			viewport_widget.set_size_request(alloc.width, alloc.height);
		}
		
		if (is_wide_mode) {
			thumb_view.set_columns(1);
			thumb_nav.set_vertical(true);
			if (thumbnails_bottom.get_child() != null)
			{
				thumbnails_bottom.remove(thumb_nav);
			}
			thumbnails_right.add(thumb_nav);
			thumbnails_right.resize_children();
			thumbnails_right.show_all();
			thumbnails_bottom.hide_all();
			
		}
		else {
			thumb_view.set_columns(5000);
			thumb_nav.set_vertical(false);
			if (thumbnails_right.get_child() != null)
			{
				thumbnails_right.remove(thumb_nav);
			}
			thumbnails_bottom.add(thumb_nav);
			thumbnails_bottom.resize_children();
			thumbnails_right.hide_all();
			thumbnails_bottom.show_all();

		}
		
		if(!initialize) {
			// Makes sure that the window is the size it ought to be, and no bigger/smaller
			// Used to make sure that the viewport_widget does not keep growing with each toggle
			Gtk.Requisition req;
			this.size_request(out req);
			this.resize(req.width, req.height);
		}
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
		thumbnails_right = (Frame) builder.get_object("thumbnails_right");
		thumbnails_bottom = (Frame) builder.get_object("thumbnails_bottom");
		menubar = (Gtk.MenuBar) builder.get_object("main_menubar");
		leave_fullscreen_button_container = (Gtk.HBox) builder.get_object("leave_fullscreen_button_bin");

		Clutter.Rectangle r = new Clutter.Rectangle();
		r.width = 200;
		r.height = 600;
		r.x = 0;
		r.y = 0;
		r.color = Clutter.Color.from_string("Red");
		viewport.add_actor(r);

		thumb_view = new Cheese.ThumbView();
		thumb_nav = new Eog.ThumbNav(thumb_view, false);
		
		viewport.show_all();
		set_wide_mode(false, true);
		this.add(main_vbox);
			
	}
}
