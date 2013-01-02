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
    static bool wide;
    static string device;
    static bool version;
    static bool fullscreen;

    static MainWindow main_window;

    private Camera camera;
    private PreferencesDialog preferences_dialog;

    private const GLib.ActionEntry action_entries[] = {
        { "shoot", on_shoot },
        { "mode", on_action_radio, "s", "'photo'", on_mode_change },
        { "fullscreen", on_action_toggle, null, "false", on_fullscreen_change },
        { "effects", on_action_toggle, null, "false", on_effects_change },
        { "preferences", on_preferences },
        { "about", on_about },
        { "help", on_help },
        { "quit", on_quit }
    };

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

  /**
   * Present the existing main window, or create a new one.
   */
  public void on_app_activate ()
  {
    if (get_windows () != null)
      main_window.present ();
    else
    {
      // Prefer a dark GTK+ theme, bug 660628.
      var gtk_settings = Gtk.Settings.get_default ();
      if (gtk_settings != null)
        gtk_settings.gtk_application_prefer_dark_theme = true;

      main_window = new Cheese.MainWindow (this);

      Environment.set_variable ("PULSE_PROP_media.role", "production", true);

      Environment.set_application_name (_("Cheese"));
      Window.set_default_icon_name ("cheese");

      Gtk.IconTheme.get_default ().append_search_path (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "icons"));

            add_action_entries (action_entries, this);

            // Create the menus.
            var menu = new GLib.Menu ();
            var section = new GLib.Menu ();
            menu.append_section (null, section);
            var item = new GLib.MenuItem (_("_Shoot"), "app.shoot");
            item.set_attribute ("accel", "s", "space");
            section.append_item (item);
            section = new GLib.Menu ();
            menu.append_section (_("Mode:"), section);
            section.append (_("_Photo"), "app.mode::photo");
            section.append (_("_Video"), "app.mode::video");
            section.append (_("_Burst"), "app.mode::burst");
            section = new GLib.Menu ();
            menu.append_section (null, section);
            item = new GLib.MenuItem (_("_Fullscreen"), "app.fullscreen");
            item.set_attribute ("accel", "s", "F11");
            section.append_item (item);
            section = new GLib.Menu ();
            menu.append_section (null, section);
            section.append (_("_Effects"), "app.effects");
            section = new GLib.Menu ();
            menu.append_section (null, section);
            section.append (_("P_references"), "app.preferences");
            section = new GLib.Menu ();
            menu.append_section (null, section);
            section.append (_("_About"), "app.about");
            item = new GLib.MenuItem (_("_Help"), "app.help");
            item.set_attribute ("accel", "s", "F1");
            section.append_item (item);
            item = new GLib.MenuItem (_("_Quit"), "app.quit");
            item.set_attribute ("accel", "s", "<Primary>q");
            section.append_item (item);
            set_app_menu (menu);

            // FIXME: Read fullscreen state from GSettings.

      main_window.setup_ui ();
      main_window.start_thumbview_monitors ();

      if (wide)
        main_window.set_startup_wide_mode ();
      if (fullscreen)
        main_window.set_startup_fullscreen_mode ();

      /* Shoot when the webcam capture button is pressed. */
      main_window.add_events (Gdk.EventMask.KEY_PRESS_MASK
                              | Gdk.EventMask.KEY_RELEASE_MASK);
      main_window.key_press_event.connect (on_webcam_key_pressed);

      main_window.show ();
      setup_camera (device);
      preferences_dialog = new PreferencesDialog (camera);
    }
  }

  /**
   * Overridden method of GApplication, to handle the arguments locally.
   *
   * @param arguments the command-line arguments
   * @param exit_status the exit status to return to the OS
   * @return true if the arguments were successfully processed, false otherwise
   */
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
        context.set_translation_domain (Config.GETTEXT_PACKAGE);
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

    /**
     * Setup the camera listed in GSettings.
     *
     * @param uri the uri of the device node to setup, or null
     */
    public void setup_camera (string? uri)
    {
        var settings = new GLib.Settings ("org.gnome.Cheese");
        string device;
        double value;

        if (uri != null && uri.length > 0)
        {
            device = uri;
        }
        else
        {
            device = settings.get_string ("camera");
        }

        var video_preview = main_window.get_video_preview ();
        camera = new Camera (video_preview, device,
            settings.get_int ("photo-x-resolution"),
            settings.get_int ("photo-y-resolution"));

        try
        {
            camera.setup (device);
        }
        catch (Error err)
        {
            video_preview.hide ();
            warning ("Error: %s\n", err.message);
            //error_layer.text = err.message;
            //error_layer.show ();

            //toggle_camera_actions_sensitivities (false);
            return;
        }

        value = settings.get_double ("brightness");
        if (value != 0.0)
        {
            camera.set_balance_property ("brightness", value);
        }

        value = settings.get_double ("contrast");
        if (value != 1.0)
        {
            camera.set_balance_property ("contrast", value);
        }

        value = settings.get_double ("hue");
        if (value != 0.0)
        {
            camera.set_balance_property ("hue", value);
        }

        value = settings.get_double ("saturation");
        if (value != 1.0)
        {
            camera.set_balance_property ("saturation", value);
        }

        camera.state_flags_changed.connect (on_camera_state_flags_changed);
        main_window.set_camera (camera);
        camera.play ();
    }

    /**
     * Handle the webcam take photo button being pressed.
     *
     * @param event the Gdk.KeyEvent
     * @return true to stop other handlers being invoked, false to propagate
     * the event further
     */
    private bool on_webcam_key_pressed (Gdk.EventKey event)
    {
        /* Ignore the event if any modifier keys are pressed. */
        if (event.state != 0
            && ((event.state & Gdk.ModifierType.CONTROL_MASK) != 0
                 || (event.state & Gdk.ModifierType.MOD1_MASK) != 0
                 || (event.state & Gdk.ModifierType.MOD3_MASK) != 0
                 || (event.state & Gdk.ModifierType.MOD4_MASK) != 0
                 || (event.state & Gdk.ModifierType.MOD5_MASK) != 0))
        {
            return false;
        }

        switch (event.keyval)
        {
            case Gdk.Key.WebCam:
                on_shoot ();
                return true;
        }

        return false;
    }

    /**
     * Handle the camera state changing.
     *
     * @param new_state the new Cheese.Camera state
     */
    private void on_camera_state_flags_changed (Gst.State new_state)
    {
        switch (new_state)
        {
            case Gst.State.PLAYING:
                main_window.camera_state_change_playing ();
                break;
            default:
                break;
        }
    }

    /**
     * Update the current capture mode in the main window and preferences
     * dialog.
     *
     * @param mode the mode to set
     */
    private void update_mode (MediaMode mode)
    {
        main_window.set_current_mode (mode);
        preferences_dialog.set_current_mode (mode);
    }

    /**
     * Handle radio actions by setting the new state.
     *
     * @param action the action which was triggered
     * @param parameter the new value to set on the action
     */
    private void on_action_radio (SimpleAction action, Variant? parameter)
    {
        action.change_state (parameter);
    }

    /**
     * Handle toggle actions by toggling the current state.
     *
     * @param action the action which was triggered
     * @param parameter unused
     */
    private void on_action_toggle (SimpleAction action, Variant? parameter)
    {
        var state = action.get_state ();

        // Toggle current state.
        action.change_state (new Variant.boolean (!state.get_boolean ()));
    }

    /**
     * Handle the shoot action being activated.
     */
    private void on_shoot ()
    {
        // Shoot.
        main_window.shoot ();
    }

    /**
     * Handle the fullscreen state being changed.
     *
     * @param action the action that emitted the signal
     * @param value the state to switch to
     */
    private void on_fullscreen_change (SimpleAction action, Variant? value)
    {
        return_if_fail (value != null);

        var state = value.get_boolean ();

        main_window.set_fullscreen (state);

        action.set_state (value);
    }

    /**
     * Handle the effects state being changed.
     *
     * @param action the action that emitted the signal
     * @param value the state to switch to
     */
    private void on_effects_change (SimpleAction action, Variant? value)
    {
        return_if_fail (value != null);

        var state = value.get_boolean ();

        main_window.set_effects (state);

        action.set_state (value);
    }

    /**
     * Change the media capture mode (photo, video or burst).
     *
     * @param action the action that emitted the signal
     * @param parameter the mode to switch to, or null
     */
    private void on_mode_change (SimpleAction action, Variant? value)
    {
        return_if_fail (value != null);

        var state = value.get_string ();

        // FIXME: Should be able to get these from the enum.
        if (state == "photo")
            update_mode (MediaMode.PHOTO);
        else if (state == "video")
            update_mode (MediaMode.VIDEO);
        else if (state == "burst")
            update_mode (MediaMode.BURST);
        else
            assert_not_reached ();

        action.set_state (value);
    }

    /**
     * Show the preferences dialog.
     */
    private void on_preferences ()
    {
        preferences_dialog.show ();
    }

    /**
     * Show the Cheese help contents.
     */
    private void on_help ()
    {
        var screen = main_window.get_screen ();
        try
        {
            Gtk.show_uri (screen, "help:cheese", Gtk.get_current_event_time ());
        }
        catch (Error err)
        {
            message ("Error opening help: %s", err.message);
        }
    }

    /**
     * Show the about dialog.
     */
    private void on_about ()
    {
        string[] artists = { "Andreas Nilsson <andreas@andreasn.se>",
            "Josef Vybíral <josef.vybiral@gmail.com>",
            "Kalle Persson <kalle@kallepersson.se>",
            "Lapo Calamandrei <calamandrei@gmail.com>",
            "Or Dvory <gnudles@nana.co.il>",
            "Ulisse Perusin <ulisail@yahoo.it>",
            null };

        string[] authors = { "daniel g. siegel <dgsiegel@gnome.org>",
            "Jaap A. Haitsma <jaap@haitsma.org>",
            "Filippo Argiolas <fargiolas@gnome.org>",
            "Yuvaraj Pandian T <yuvipanda@yuvi.in>",
            "Luciana Fujii Pontello <luciana@fujii.eti.br>",
            "David King <amigadave@amigadave.com>",
            "",
            "Aidan Delaney <a.j.delaney@brighton.ac.uk>",
            "Alex \"weej\" Jones <alex@weej.com>",
            "Andrea Cimitan <andrea.cimitan@gmail.com>",
            "Baptiste Mille-Mathias <bmm80@free.fr>",
            "Cosimo Cecchi <anarki@lilik.it>",
            "Diego Escalante Urrelo <dieguito@gmail.com>",
            "Felix Kaser <f.kaser@gmx.net>",
            "Gintautas Miliauskas <gintas@akl.lt>",
            "Hans de Goede <jwrdegoede@fedoraproject.org>",
            "James Liggett <jrliggett@cox.net>",
            "Luca Ferretti <elle.uca@libero.it>",
            "Mirco \"MacSlow\" Müller <macslow@bangang.de>",
            "Patryk Zawadzki <patrys@pld-linux.org>",
            "Ryan Zeigler <zeiglerr@gmail.com>",
            "Sebastian Keller <sebastian-keller@gmx.de>",
            "Steve Magoun <steve.magoun@canonical.com>",
            "Thomas Perl <thp@thpinfo.com>",
            "Tim Philipp Müller <tim@centricular.net>",
            "Todd Eisenberger <teisenberger@gmail.com>",
            "Tommi Vainikainen <thv@iki.fi>",
            null };

        string[] documenters = { "Joshua Henderson <joshhendo@gmail.com>",
            "Jaap A. Haitsma <jaap@haitsma.org>",
            "Felix Kaser <f.kaser@gmx.net>",
            null };

        Gtk.show_about_dialog (main_window,
            "artists", artists,
            "authors", authors,
            "comments", _("Take photos and videos with your webcam, with fun graphical effects"),
            "copyright", "Copyright © 2007 - 2010 daniel g. siegel <dgsiegel@gnome.org>",
            "documenters", documenters,
            "license-type", Gtk.License.GPL_2_0,
            "logo-icon-name", Config.PACKAGE_TARNAME,
            "program-name", _("Cheese"),
            "translator-credits", _("translator-credits"),
            "website", Config.PACKAGE_URL,
            "website-label", _("Cheese Website"),
            "version", Config.PACKAGE_VERSION);
    }

    /**
     * Destroy the main window, and shutdown the application, when quitting.
     */
    private void on_quit ()
    {
        main_window.destroy ();
    }
}

  public int main (string[] args)
  {
    Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.PACKAGE_LOCALEDIR);
    Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain (Config.GETTEXT_PACKAGE);

    if (!Cheese.gtk_init (ref args))
      return Posix.EXIT_FAILURE;

    Cheese.Main app;
    app = new Cheese.Main ("org.gnome.Cheese", ApplicationFlags.FLAGS_NONE);

    app.activate.connect (app.on_app_activate);
    int status = app.run (args);

    return status;
  }
