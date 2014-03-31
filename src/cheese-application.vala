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

public class Cheese.Application : Gtk.Application
{
    private GLib.Settings settings;
    private uint inhibited = 0;

    static string device;

    static MainWindow main_window;

    private Camera camera;
    private PreferencesDialog preferences_dialog;

    private const GLib.ActionEntry action_entries[] = {
        { "shoot", on_shoot },
        { "mode", on_action_radio, "s", "'photo'", on_mode_change },
        { "fullscreen", on_action_toggle, null, "false",
          on_fullscreen_change },
        { "wide-mode", on_action_toggle, null, "false", on_wide_mode_change },
        { "effects", on_action_toggle, null, "false", on_effects_change },
        { "preferences", on_preferences },
        { "help", on_help },
        { "about", on_about },
        { "quit", on_quit }
    };

    const OptionEntry[] options = {
        { "wide", 'w', 0, OptionArg.NONE, null, N_("Start in wide mode"),
          null  },
        { "device", 'd', 0, OptionArg.FILENAME, null,
          N_("Device to use as a camera"), N_("DEVICE") },
        { "version", 'v', 0, OptionArg.NONE, null,
          N_("Output version information and exit"), null },
        { "fullscreen", 'f', 0, OptionArg.NONE, null,
          N_("Start in fullscreen mode"), null },
        { null }
    };

    public Application ()
    {
        GLib.Object (application_id: "org.gnome.Cheese",
                     flags: ApplicationFlags.HANDLES_COMMAND_LINE);

        this.add_main_option_entries (options);
    }

    /**
     * Perform one-time initialization tasks.
     */
    protected override void startup ()
    {
        settings = new GLib.Settings ("org.gnome.Cheese");

        add_action_entries (action_entries, this);

        string[] args = { null };
        unowned string[] arguments = args;

        if (!Cheese.gtk_init (ref arguments))
        {
            error ("Unable to initialize libcheese-gtk");
        }

        // Calls gtk_init() with no arguments.
        base.startup ();
    }

    /**
     * Ensure that the main window has been shown, camera set up and so on.
     */
    private void common_init ()
    {
        if (this.get_windows () == null)
        {
            // Prefer a dark GTK+ theme, bug 660628.
            var gtk_settings = Gtk.Settings.get_default ();

            if (gtk_settings != null)
            {
                gtk_settings.gtk_application_prefer_dark_theme = true;
            }

            main_window = new Cheese.MainWindow (this);

            Environment.set_variable ("PULSE_PROP_media.role", "production",
                                      true);

            Environment.set_application_name (_("Cheese"));
            Window.set_default_icon_name ("cheese");

            // Create the menus.
            var builder = new Gtk.Builder.from_resource ("/org/gnome/Cheese/cheese-appmenu.ui");
            var appmenu = builder.get_object ("appmenu") as GLib.MenuModel;
            this.set_app_menu (appmenu);

            this.add_accelerator ("space", "app.shoot", null);

            // FIXME: Push these into the main window initialization.
            main_window.setup_ui ();
            main_window.start_thumbview_monitors ();

            /* Shoot when the webcam capture button is pressed. */
            main_window.add_events (Gdk.EventMask.KEY_PRESS_MASK
                                    | Gdk.EventMask.KEY_RELEASE_MASK);
            main_window.key_press_event.connect (on_webcam_key_pressed);

            main_window.show ();
            setup_camera ();
            preferences_dialog = new PreferencesDialog (camera);
            var preferences = this.lookup_action ("preferences");
            preferences.notify["enabled"].connect (on_preferences_enabled);
            this.add_window (main_window);
        }
    }

    /**
     * Present the existing main window, or create a new one.
     */
    protected override void activate ()
    {
        if (this.get_windows () != null)
        {
            main_window.present ();
        }
        else
        {
            common_init ();
        }
    }

    protected override int command_line (ApplicationCommandLine cl)
    {
        var opts = cl.get_options_dict ();

        if (opts.lookup ("device", "^ay", out device, null))
        {
            settings.set_string ("camera", device);
        }

        if (opts.contains ("fullscreen"))
        {
            activate_action ("fullscreen", null);
        }

        if (opts.contains ("wide"))
        {
            activate_action ("wide-mode", null);
        }

        this.activate ();

        return 0;
    }

    protected override int handle_local_options (VariantDict opts)
    {
        if (opts.contains ("version"))
        {
            stdout.printf ("%s %s\n", Config.PACKAGE_NAME,
                           Config.PACKAGE_VERSION);
            return 0;
        }

        return -1;
    }

    /**
     * Setup the camera listed in GSettings.
     */
    public void setup_camera ()
    {
        var effects = this.lookup_action ("effects") as SimpleAction;
        var mode = this.lookup_action ("mode") as SimpleAction;
        var shoot = this.lookup_action ("shoot") as SimpleAction;
        effects.set_enabled (false);
        mode.set_enabled (false);
        shoot.set_enabled (false);

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
            message ("Error during camera setup: %s\n", err.message);
            main_window.show_error (err.message);

            return;
        }

        double value;

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
                activate_action ("shoot", null);
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
        var effects = this.lookup_action ("effects") as SimpleAction;
        var mode = this.lookup_action ("mode") as SimpleAction;
        var shoot = this.lookup_action ("shoot") as SimpleAction;

        switch (new_state)
        {
            case Gst.State.PLAYING:
                if (effects.state.get_boolean ())
                {
                    mode.set_enabled (false);
                    shoot.set_enabled (false);
                }
                else
                {
                    mode.set_enabled (true);
                    shoot.set_enabled (true);
                }

                effects.set_enabled (true);

                main_window.camera_state_change_playing ();

                inhibited = this.inhibit (main_window,
                                          Gtk.ApplicationInhibitFlags.SWITCH
                                          | Gtk.ApplicationInhibitFlags.IDLE,
                                          _("Webcam in use"));
                break;
            case Gst.State.NULL:
                effects.set_enabled (false);
                mode.set_enabled (false);
                shoot.set_enabled (false);

                main_window.camera_state_change_null ();

                if (inhibited != 0)
                {
                    this.uninhibit (inhibited);
                }
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

        // Action can be activated before activate ().
        common_init ();

        main_window.set_fullscreen (state);

        action.set_state (value);
    }

    /**
     * Handle the wide-mode state being changed.
     *
     * @param action the action that emitted the signal
     * @param value the state to switch to
     */
    private void on_wide_mode_change (SimpleAction action, Variant? value)
    {
        return_if_fail (value != null);

        var state = value.get_boolean ();

        // Action can be activated before activate ().
        common_init ();

        main_window.set_wide_mode (state);

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

        var shoot = this.lookup_action ("shoot") as SimpleAction;
        var mode = this.lookup_action ("mode") as SimpleAction;

        // Effects selection and shooting/mode changes are mutually exclusive.
        shoot.set_enabled (!state);
        mode.set_enabled (!state);

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
            "copyright", "Copyright © 2011 - 2014 David King <amigadave@amigadave.com>\nCopyright © 2007 - 2011 daniel g. siegel <dgsiegel@gnome.org>",
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

    /**
     * Close the preferences dialog when the preferences action is disabled.
     */
    private void on_preferences_enabled ()
    {
        var preferences = this.lookup_action ("preferences");

        if (!preferences.enabled)
        {
            preferences_dialog.hide ();
        }
    }
}
