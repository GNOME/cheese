using GLib;
using Gtk;
using Clutter;

public class Cheese.Main {
	static bool verbose;
	static bool wide;
	static bool version_only;
	static FileStream log_file;

	const OptionEntry[] options = {
		{ "verbose", 'v', 0, OptionArg.NONE, ref verbose, N_("Be verbose"), null},
		{ "wide", 'w', 0, OptionArg.NONE, ref wide, N_("Enable wide mode"), null},
		{ "version", 0, 0, OptionArg.NONE, ref version_only, N_("Output version information and exit"), null},
		{ null }
	};

	static void print_handler (string text) {
		log_file.puts (text);
		if (verbose) {
			stdout.puts (text);
		}
	}

	public static int main (string[] args) {

		Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.PACKAGE_LOCALEDIR);
  		Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
		Intl.textdomain (Config.GETTEXT_PACKAGE);

		Gtk.init(ref args);
		Clutter.init(ref args);
		
		try {
			var context = new OptionContext (_("- Take photos and videos from your webcam"));
			context.set_help_enabled (true);
			context.add_main_entries (options, null);
			context.add_group (Gtk.get_option_group (true));
			context.add_group (Clutter.get_option_group ());
			context.parse (ref args);
		} catch (OptionError e) {
			stdout.printf ("%s\n", e.message);
			stdout.printf (_("Run '%s --help' to see a full list of available command line options.\n"), args[0]);
			return 1;
		}

 		Environment.set_application_name (_("Cheese"));
		Window.set_default_icon_name ("cheese");

		string log_file_dir = GLib.Path.build_filename (Environment.get_home_dir(), ".config", "cheese");
		DirUtils.create_with_parents (log_file_dir, 0775);
		log_file = FileStream.open (GLib.Path.build_filename (log_file_dir, "cheese.log"), "w");
		set_print_handler (print_handler);

//		Gtk.IconTheme.get_default().append_search_path(GLib.Path.build_filename (Config.PACKAGE_DATADIR, "icons"));
		Cheese.MainWindow main_window = new Cheese.MainWindow();
		main_window.setup_ui();
		main_window.destroy.connect(Gtk.main_quit);
		main_window.show_all();
		
		Gtk.main ();
		
		return 0;
	}
}
