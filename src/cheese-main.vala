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

public class Cheese.Main : Gtk.Application
{
  static bool   wide;
  static string device;
  static bool   version;
  static bool   fullscreen;

  static Cheese.MainWindow main_window;

  const OptionEntry[] options = {
    {"wide",       'w', 0, OptionArg.NONE,     ref wide,       N_("Start in wide mode"),                  null        },
    {"device",     'd', 0, OptionArg.FILENAME, ref device,     N_("Device to use as a camera"),           N_("DEVICE")},
    {"version",    'v', 0, OptionArg.NONE,     ref version,    N_("Output version information and exit"), null        },
    {"fullscreen", 'f', 0, OptionArg.NONE,     ref fullscreen, N_("Start in fullscreen mode"),            null        },
    {null}
  };

  public Main (string app_id, ApplicationFlags flags)
  {
    GLib.Object (application_id: app_id, flags: flags);
  }

  public void on_app_activate ()
  {
    if (get_windows () != null)
      main_window.present ();
    else
    {
      main_window = new Cheese.MainWindow ();

      Environment.set_application_name (_("Cheese"));
      Window.set_default_icon_name ("cheese");

      Gtk.IconTheme.get_default ().append_search_path (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "icons"));

      main_window.setup_ui ();

      if (wide)
        main_window.set_startup_wide_mode ();
      if (fullscreen)
        main_window.set_startup_fullscreen_mode ();

      main_window.set_application (this);
      main_window.destroy.connect (Gtk.main_quit);
      main_window.show ();
      main_window.setup_camera (device);
     }
  }

  public override bool local_command_line ([CCode (array_null_terminated = true, array_length = false)]
                                           ref unowned string[] arguments,
                                           out int exit_status)
  {
    // Try to register.
    try
    {
      register();
    }
    catch (Error e)
    {
      stdout.printf ("Error: %s\n", e.message);
      exit_status = 1;
      return true;
    }

    // Workaround until bug 642885 is solved.
    unowned string[] local_args = arguments;

    // Check command line parameters.
    int n_args = local_args.length;
    if (n_args <= 1)
    {
      Gst.init (ref local_args);
      activate ();
      exit_status = 0;
    }
    else
    {
      // Set parser.
      try
      {
        var context = new OptionContext (_("- Take photos and videos from your webcam"));
        context.set_help_enabled (true);
        context.add_main_entries (options, null);
        context.add_group (Gtk.get_option_group (true));
        context.add_group (Clutter.get_option_group ());
        context.add_group (Gst.init_get_option_group ());
        context.parse (ref local_args);
      }
      catch (OptionError e)
      {
        stdout.printf ("%s\n", e.message);
        stdout.printf (_("Run '%s --help' to see a full list of available command line options.\n"), arguments[0]);
        exit_status = 1;
        return true;
      }

      if (version)
      {
        stdout.printf ("%s %s\n", Config.PACKAGE_NAME, Config.PACKAGE_VERSION);
        exit_status = 1;
        return true;
      }

      //Remote instance process commands locally.
      if (get_is_remote ())
      {
          stdout.printf (_("Another instance of Cheese is currently running\n"));
          exit_status = 1;
          return true;
      }
      //Primary instance.
      else
      {
        Gst.init (ref local_args);
        activate ();
        exit_status=0;
      }
    }
    return true;
  }
}

  public int main (string[] args)
  {
    Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.PACKAGE_LOCALEDIR);
    Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain (Config.GETTEXT_PACKAGE);

    GtkClutter.init (ref args);

    Cheese.Main app;
    app = new Cheese.Main ("org.gnome.Cheese", ApplicationFlags.FLAGS_NONE);

    app.activate.connect (app.on_app_activate);
    int status = app.run (args);

    return status;
  }
