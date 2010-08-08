/*
 * Copyright © 2010 Yuvaraj Pandian T <yuvipanda@yuvi.in>
 * Copyright © 2010 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2008 Filippo Argiolas <filippo.argiolas@gmail.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using GLib;
using Gtk;
using Clutter;
using Gst;
using Unique;

public class Cheese.Main
{
  static bool       verbose;
  static bool       wide;
  static bool       version_only;
  static FileStream log_file;

  static Cheese.MainWindow main_window;

  const OptionEntry[] options = {
    { "verbose", 'v', 0, OptionArg.NONE, ref verbose,      N_("Be verbose"),                          null},
    { "wide",    'w', 0, OptionArg.NONE, ref wide,         N_("Enable wide mode"),                    null},
    { "version", 0,   0, OptionArg.NONE, ref version_only, N_("Output version information and exit"), null},
    { null}
  };

  static void print_handler (string text)
  {
    log_file.puts (text);
    if (verbose)
    {
      stdout.puts (text);
    }
  }

  public static Unique.Response unique_message_received (int                command,
                                                         Unique.MessageData msg,
                                                         uint               time)
  {
    if (command == Unique.Command.ACTIVATE)
    {
      main_window.set_screen (msg.get_screen ());
      main_window.activate ();
    }
    return Unique.Response.OK;
  }

  public static int main (string[] args)
  {
    Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.PACKAGE_LOCALEDIR);
    Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain (Config.GETTEXT_PACKAGE);

    Gtk.rc_parse (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "gtkrc"));

    Gtk.init (ref args);
    Clutter.init (ref args);

    try {
      var context = new OptionContext (_("- Take photos and videos from your webcam"));
      context.set_help_enabled (true);
      context.add_main_entries (options, null);
      context.add_group (Gtk.get_option_group (true));
      context.add_group (Clutter.get_option_group ());
      context.add_group (Gst.init_get_option_group ());
      context.parse (ref args);
    }
    catch (OptionError e)
    {
      stdout.printf ("%s\n", e.message);
      stdout.printf (_("Run '%s --help' to see a full list of available command line options.\n"), args[0]);
      return 1;
    }

    main_window = new Cheese.MainWindow ();

    Unique.App app = new Unique.App ("org.gnome.Cheese", null);
    if (app.is_running)
    {
      Unique.Response response;
      response = app.send_message (Unique.Command.ACTIVATE, null);
      return 0;
    }
    else
    {
      app.watch_window (main_window);
      app.message_received.connect (unique_message_received);
    }

    Environment.set_application_name (_("Cheese"));
    Window.set_default_icon_name ("cheese");

    string log_file_dir = GLib.Path.build_filename (Environment.get_home_dir (), ".config", "cheese");
    DirUtils.create_with_parents (log_file_dir, 0775);
    log_file = FileStream.open (GLib.Path.build_filename (log_file_dir, "cheese.log"), "w");
    set_print_handler (print_handler);

    Gtk.IconTheme.get_default ().append_search_path (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "icons"));

    main_window.setup_ui ();
    main_window.destroy.connect (Gtk.main_quit);
    main_window.show_all ();
    main_window.setup_camera ();
    Gtk.main ();

    return 0;
  }
}
