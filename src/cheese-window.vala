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

using Gtk;
using Gdk;
using GtkClutter;
using Clutter;
using Config;
using Eog;
using Gst;
using CanberraGtk;

const int FULLSCREEN_TIMEOUT_INTERVAL = 5 * 1000;
const uint EFFECTS_PER_PAGE = 9;

[GtkTemplate (ui = "/org/gnome/Cheese/cheese-main-window.ui")]
public class Cheese.MainWindow : Gtk.ApplicationWindow
{
    private const GLib.ActionEntry actions[] = {
        { "file-open", on_file_open },
        { "file-saveas", on_file_saveas },
        { "file-trash", on_file_trash },
        { "file-delete", on_file_delete },
        { "effects-next", on_effects_next },
        { "effects-previous", on_effects_previous }
    };

    private MediaMode current_mode;

    private Clutter.Script clutter_builder;

    private Gtk.Builder header_bar_ui = new Gtk.Builder.from_resource ("/org/gnome/Cheese/headerbar.ui");

    private Gtk.HeaderBar header_bar;

    private GLib.Settings settings;

    [GtkChild]
    private GtkClutter.Embed viewport_widget;
    [GtkChild]
    private Gtk.Widget main_vbox;
    private Eog.ThumbNav thumb_nav;
    private Cheese.ThumbView thumb_view;
    [GtkChild]
    private Gtk.Alignment thumbnails_right;
    [GtkChild]
    private Gtk.Alignment thumbnails_bottom;
    [GtkChild]
    private Gtk.Widget leave_fullscreen_button_box;
    [GtkChild]
    private Gtk.Button take_action_button;
    [GtkChild]
    private Gtk.Image take_action_button_image;
    [GtkChild]
    private Gtk.ToggleButton effects_toggle_button;
    [GtkChild]
    private Gtk.Image effects_prev_page_button_image;
    [GtkChild]
    private Gtk.Image effects_next_page_button_image;
    [GtkChild]
    private Gtk.Widget buttons_area;
    private Gtk.Menu thumbnail_popup;

    private Clutter.Stage viewport;
    private Clutter.Actor viewport_layout;
    private Clutter.Texture video_preview;
    private Clutter.BinLayout viewport_layout_manager;
    private Clutter.Text countdown_layer;
    private Clutter.Actor background_layer;
    private Clutter.Text error_layer;
    private Clutter.Text timeout_layer;

  private Clutter.Actor current_effects_grid;
  private uint current_effects_page = 0;
  private List<Clutter.Actor> effects_grids;

  private bool is_fullscreen;
  private bool is_wide_mode;
  private bool is_recording;       /* Video Recording Flag */
  private bool is_bursting;
  private bool is_effects_selector_active;
  private bool action_cancelled;
    private bool was_maximized;

  private Cheese.Camera   camera;
  private Cheese.FileUtil fileutil;
  private Cheese.Flash    flash;

  private Cheese.EffectsManager    effects_manager;

  private Cheese.Effect selected_effect;

  /**
   * Responses from the delete files confirmation dialog.
   *
   * @param SKIP skip a single file
   * @param SKIP_ALL skill all following files
   */
  enum DeleteResponse
  {
    SKIP = 1,
    SKIP_ALL = 2
  }

    public MainWindow (Gtk.Application application)
    {
        GLib.Object (application: application);

        header_bar = header_bar_ui.get_object ("header_bar") as Gtk.HeaderBar;

        Gtk.Settings settings = Gtk.Settings.get_default ();

        if (settings.gtk_dialogs_use_header)
        {
            header_bar.visible = true;
            this.set_titlebar (header_bar);
        }

        if (get_direction () == Gtk.TextDirection.RTL)
        {
            effects_prev_page_button_image.icon_name = "go-previous-rtl-symbolic";
            effects_next_page_button_image.icon_name = "go-next-rtl-symbolic";
        }
        else
        {
            effects_prev_page_button_image.icon_name = "go-previous-symbolic";
            effects_next_page_button_image.icon_name = "go-next-symbolic";
        }
    }

    private void set_window_title (string title)
    {
        header_bar.set_title (title);
        this.set_title (title);
    }

    private bool on_window_state_change_event (Gtk.Widget widget,
                                               Gdk.EventWindowState event)
    {
        was_maximized = (((event.new_window_state - event.changed_mask)
                          & Gdk.WindowState.MAXIMIZED) != 0);

        window_state_event.disconnect (on_window_state_change_event);
        return false;
    }

    /**
    * Popup a context menu when right-clicking on a thumbnail.
    *
    * @param iconview the thumbnail view that emitted the signal
    * @param event the event
    * @return false to allow further processing of the event, true to indicate
    * that the event was handled completely
    */
    public bool on_thumbnail_button_press_event (Gtk.Widget iconview,
                                                 Gdk.EventButton event)
    {
        Gtk.TreePath path;
        path = thumb_view.get_path_at_pos ((int) event.x, (int) event.y);

        if (path == null)
        {
            return false;
        }

        if (!thumb_view.path_is_selected (path))
        {
            thumb_view.unselect_all ();
            thumb_view.select_path (path);
            thumb_view.set_cursor (path, null, false);
        }

        if (event.type == Gdk.EventType.BUTTON_PRESS)
        {
            Gdk.Event* button_press = (Gdk.Event*)(&event);

            if (button_press->triggers_context_menu ())
            {
                thumbnail_popup.popup (null, thumb_view, null, event.button,
                                       event.time);
            }
            else if (event.type == Gdk.EventType.2BUTTON_PRESS)
            {
                on_file_open ();
            }

            return true;
        }

        return false;
    }

  /**
   * Open an image associated with a thumbnail in the default application.
   */
  private void on_file_open ()
  {
    string filename, uri;

    Gdk.Screen screen;
    filename = thumb_view.get_selected_image ();

    if (filename == null)
      return;                     /* Nothing selected. */

    try
    {
      uri    = GLib.Filename.to_uri (filename);
      screen = this.get_screen ();
      Gtk.show_uri (screen, uri, Gtk.get_current_event_time ());
    }
    catch (Error err)
    {
      MessageDialog error_dialog = new MessageDialog (this,
                                                      Gtk.DialogFlags.MODAL |
                                                      Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                      Gtk.MessageType.ERROR,
                                                      Gtk.ButtonsType.OK,
                                                      _("Could not open %s"),
                                                      filename);

      error_dialog.run ();
      error_dialog.destroy ();
    }
  }

  /**
   * Delete the requested image or images in the thumbview from storage.
   *
   * A confirmation dialog is shown to the user before deleting any files.
   */
  private void on_file_delete ()
  {
    int response;
    int error_response;
    bool skip_all_errors = false;

    var files = thumb_view.get_selected_images_list ();
    var files_length = files.length ();

    var confirmation_dialog = new MessageDialog.with_markup (this,
      Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
      Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE,
      GLib.ngettext("Are you sure you want to permanently delete the file?",
        "Are you sure you want to permanently delete %d files?",
        files_length), files_length);
    confirmation_dialog.add_button (_("_Cancel"), Gtk.ResponseType.CANCEL);
    confirmation_dialog.add_button (_("_Delete"), Gtk.ResponseType.ACCEPT);
    confirmation_dialog.format_secondary_text ("%s",
      GLib.ngettext("If you delete an item, it will be permanently lost",
        "If you delete the items, they will be permanently lost",
        files_length));

    response = confirmation_dialog.run ();
    if (response == Gtk.ResponseType.ACCEPT)
    {
      foreach (var file in files)
      {
        if (file == null)
          return;

        try
        {
          file.delete (null);
        }
        catch (Error err)
        {
          warning ("Unable to delete file: %s", err.message);

          if (!skip_all_errors) {
            var error_dialog = new MessageDialog (this,
              Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
              Gtk.MessageType.ERROR, Gtk.ButtonsType.NONE,
              "Could not delete %s", file.get_path ());

            error_dialog.add_button (_("_Cancel"), Gtk.ResponseType.CANCEL);
            error_dialog.add_button ("Skip", DeleteResponse.SKIP);
            error_dialog.add_button ("Skip all", DeleteResponse.SKIP_ALL);

            error_response = error_dialog.run ();
            if (error_response == DeleteResponse.SKIP_ALL) {
              skip_all_errors = true;
            } else if (error_response == Gtk.ResponseType.CANCEL) {
              break;
            }

            error_dialog.destroy ();
          }
        }
      }
    }
    confirmation_dialog.destroy ();
  }

  /**
   * Move the requested image in the thumbview to the trash.
   *
   * A confirmation dialog is shown to the user before moving the file.
   */
  private void on_file_trash ()
  {
    File file;

    GLib.List<GLib.File> files = thumb_view.get_selected_images_list ();

    for (int i = 0; i < files.length (); i++)
    {
      file = files<GLib.File>.nth (i).data;
      if (file == null)
        return;

      try
      {
        file.trash (null);
      }
      catch (Error err)
      {
        MessageDialog error_dialog = new MessageDialog (this,
                                                        Gtk.DialogFlags.MODAL |
                                                        Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                        Gtk.MessageType.ERROR,
                                                        Gtk.ButtonsType.OK,
                                                        _("Could not move %s to trash"),
                                                        file.get_path ());

        error_dialog.run ();
        error_dialog.destroy ();
      }
    }
  }

  /**
   * Save the selected file in the thumbview to an alternate storage location.
   *
   * A file chooser dialog is shown to the user, asking where the file should
   * be saved and the filename.
   */
  private void on_file_saveas ()
  {
    string            filename, basename;
    FileChooserDialog save_as_dialog;
    int               response;

    filename = thumb_view.get_selected_image ();
    if (filename == null)
      return;                    /* Nothing selected. */

    save_as_dialog = new FileChooserDialog (_("Save File"),
                                            this,
                                            Gtk.FileChooserAction.SAVE,
                                            _("_Cancel"), Gtk.ResponseType.CANCEL,
                                            _("Save"), Gtk.ResponseType.ACCEPT,
                                            null);

    save_as_dialog.do_overwrite_confirmation = true;
    basename                                 = GLib.Filename.display_basename (filename);
    save_as_dialog.set_current_name (basename);
    save_as_dialog.set_current_folder (GLib.Environment.get_home_dir ());

    response = save_as_dialog.run ();

    save_as_dialog.hide ();
    if (response == Gtk.ResponseType.ACCEPT)
    {
      string target_filename;
      target_filename = save_as_dialog.get_filename ();

      File src  = File.new_for_path (filename);
      File dest = File.new_for_path (target_filename);

      try
      {
        src.copy (dest, FileCopyFlags.OVERWRITE, null, null);
      }
      catch (Error err)
      {
        MessageDialog error_dialog = new MessageDialog (this,
                                                        Gtk.DialogFlags.MODAL |
                                                        Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                        Gtk.MessageType.ERROR,
                                                        Gtk.ButtonsType.OK,
                                                        _("Could not save %s"),
                                                        target_filename);

        error_dialog.run ();
        error_dialog.destroy ();
      }
    }
    save_as_dialog.destroy ();
  }

    /**
     * Toggle fullscreen mode.
     *
     * @param fullscreen whether the window should be fullscreen
     */
    public void set_fullscreen (bool fullscreen)
    {
        set_fullscreen_mode (fullscreen);
    }

    /**
     * Make the media capture mode actions sensitive.
     */
    private void enable_mode_change ()
    {
        var mode = this.application.lookup_action ("mode") as SimpleAction;
        mode.set_enabled (true);
        var effects = this.application.lookup_action ("effects") as SimpleAction;
        effects.set_enabled (true);
        var preferences = this.application.lookup_action ("preferences") as SimpleAction;
        preferences.set_enabled (true);
    }

    /**
     * Make the media capture mode actions insensitive.
     */
    private void disable_mode_change ()
    {
        var mode = this.application.lookup_action ("mode") as SimpleAction;
        mode.set_enabled (false);
        var effects = this.application.lookup_action ("effects") as SimpleAction;
        effects.set_enabled (false);
        var preferences = this.application.lookup_action ("preferences") as SimpleAction;
        preferences.set_enabled (false);
    }

  /**
   * Set the capture resolution, based on the current capture mode.
   *
   * @param mode the current capture mode (photo, video or burst)
   */
  private void set_resolution(MediaMode mode)
  {
    if (camera == null)
      return;

    var formats = camera.get_video_formats ();

    if (formats == null)
      return;
    
    unowned Cheese.VideoFormat format;
    int width = 0;
    int height = 0;

    switch (mode)
    {
      case MediaMode.PHOTO:
      case MediaMode.BURST:
        width  = settings.get_int ("photo-x-resolution");
        height = settings.get_int ("photo-y-resolution");
        break;
      case MediaMode.VIDEO:
        width  = settings.get_int ("video-x-resolution");
        height = settings.get_int ("video-y-resolution");
        break;
    }

    for (int i = 0; i < formats.length (); i++)
    {
      format = formats<VideoFormat>.nth (i).data;
      if (width == format.width && height == format.height)
      {
        camera.set_video_format (format);
        break;
      }
    }
  }

  private TimeoutSource fullscreen_timeout;
  /**
   * Clear the fullscreen activity timeout.
   */
  private void clear_fullscreen_timeout ()
  {
    if (fullscreen_timeout != null)
    {
      fullscreen_timeout.destroy ();
      fullscreen_timeout = null;
    }
  }

  /**
   * Set the fullscreen timeout, for hiding the UI if there is no mouse
   * movement.
   */
  private void set_fullscreen_timeout ()
  {
    fullscreen_timeout = new TimeoutSource (FULLSCREEN_TIMEOUT_INTERVAL);
    fullscreen_timeout.attach (null);
    fullscreen_timeout.set_callback (() => {buttons_area.hide ();
                                            clear_fullscreen_timeout ();
                                            this.fullscreen ();
                                            return true; });
  }

    /**
     * Show the UI in fullscreen if there is any mouse activity.
     *
     * Start a new timeout at the end of every mouse pointer movement. All
     * timeouts will be cancelled, except one created during the last movement
     * event. Show() is called even if the button is not hidden.
     *
     * @param viewport the widget to check for mouse activity on
     * @param e the (unused) event
     */
    private bool fullscreen_motion_notify_callback (Gtk.Widget viewport,
                                                    EventMotion e)
    {
        clear_fullscreen_timeout ();
        this.unfullscreen ();
        this.maximize ();
        buttons_area.show ();
        set_fullscreen_timeout ();
        return true;
    }

  /**
   * Enable or disable fullscreen mode to the requested state.
   *
   * @param fullscreen_mode whether to enable or disable fullscreen mode
   */
  private void set_fullscreen_mode (bool fullscreen)
  {
    /* After the first time the window has been shown using this.show_all (),
     * the value of leave_fullscreen_button_box.no_show_all should be set to false
     * So that the next time leave_fullscreen_button_box.show_all () is called, the button is actually shown
     * FIXME: If this code can be made cleaner/clearer, please do */

    is_fullscreen = fullscreen;
    if (fullscreen)
    {
            window_state_event.connect (on_window_state_change_event);

      if (is_wide_mode)
      {
        thumbnails_right.hide ();
      }
      else
      {
        thumbnails_bottom.hide ();
      }
      leave_fullscreen_button_box.no_show_all = false;
      leave_fullscreen_button_box.show_all ();

      this.fullscreen ();
      viewport_widget.motion_notify_event.connect (fullscreen_motion_notify_callback);
      set_fullscreen_timeout ();
    }
    else
    {
      if (is_wide_mode)
      {
        thumbnails_right.show_all ();
      }
      else
      {
        thumbnails_bottom.show_all ();
      }
      leave_fullscreen_button_box.hide ();

      /* Stop timer so buttons_area does not get hidden after returning from
       * fullscreen mode */
      clear_fullscreen_timeout ();
      /* Show the buttons area anyway - it might've been hidden in fullscreen mode */
      buttons_area.show ();
      viewport_widget.motion_notify_event.disconnect (fullscreen_motion_notify_callback);
      this.unfullscreen ();

            if (was_maximized)
            {
                this.maximize ();
            }
            else
            {
                this.unmaximize ();
            }
    }
  }

  /**
   * Enable or disable wide mode to the requested state.
   *
   * @param wide_mode whether to enable or disable wide mode
   */
  public void set_wide_mode (bool wide_mode)
  {
    is_wide_mode = wide_mode;

    /* keep the viewport to its current size while rearranging the ui,
     * so that thumbview moves from right to bottom and viceversa
     * while the rest of the window stays unchanged */
    Gtk.Allocation alloc;
    viewport_widget.get_allocation (out alloc);
    viewport_widget.set_size_request (alloc.width, alloc.height);

    if (is_wide_mode)
    {
      thumb_view.set_vertical (true);
      thumb_nav.set_vertical (true);
      if (thumbnails_bottom.get_child () != null)
      {
        thumbnails_bottom.remove (thumb_nav);
      }
      thumbnails_right.add (thumb_nav);

            if (!is_fullscreen)
            {
                thumbnails_right.show_all ();
                thumbnails_bottom.hide ();
            }
    }
    else
    {
      thumb_view.set_vertical (false);
      thumb_nav.set_vertical (false);
      if (thumbnails_right.get_child () != null)
      {
        thumbnails_right.remove (thumb_nav);
      }
      thumbnails_bottom.add (thumb_nav);

            if (!is_fullscreen)
            {
                thumbnails_bottom.show_all ();
                thumbnails_right.hide ();
            }
    }

    /* handy trick to keep the window to the desired size while not
     * requesting a fixed one. This way the window is resized to its
     * natural size (particularly with the constraints imposed by the
     * viewport, see above) but can still be shrinked down */

    Gtk.Requisition req;
    this.get_preferred_size(out req, out req);
    this.resize (req.width, req.height);
    viewport_widget.set_size_request (-1, -1);
  }

  /**
   * Make sure that the layout manager manages the entire stage.
   *
   * @param actor unused
   * @param box unused
   * @param flags unused
   */
  public void on_stage_resize (Clutter.Actor           actor,
                               Clutter.ActorBox        box,
                               Clutter.AllocationFlags flags)
  {
    this.viewport_layout.set_size (viewport.width, viewport.height);
    this.background_layer.set_size (viewport.width, viewport.height);
    this.timeout_layer.set_position (video_preview.width/3 + viewport.width/2,
                                viewport.height-20);
  }

  /**
   * The method to call when the countdown is finished.
   */
  private void finish_countdown_callback ()
  {
    if (action_cancelled == false)
    {
      string file_name = fileutil.get_new_media_filename (this.current_mode);

      if (settings.get_boolean ("flash"))
      {
        this.flash.fire ();
      }
      CanberraGtk.play_for_widget (this.main_vbox, 0,
                                   Canberra.PROP_EVENT_ID, "camera-shutter",
                                   Canberra.PROP_MEDIA_ROLE, "event",
                                   Canberra.PROP_EVENT_DESCRIPTION, _("Shutter sound"),
                                   null);
      this.camera.take_photo (file_name);
    }

    if (current_mode == MediaMode.PHOTO)
    {
      enable_mode_change ();
    }
  }

  Countdown current_countdown;
  /**
   * Start to take a photo, starting a countdown if it is enabled.
   */
  public void take_photo ()
  {
    if (settings.get_boolean ("countdown"))
    {
      if (current_mode == MediaMode.PHOTO)
      {
        disable_mode_change ();
      }

      current_countdown = new Countdown (this.countdown_layer);
      current_countdown.start (finish_countdown_callback);
    }
    else
    {
      finish_countdown_callback ();
    }
  }

  private int  burst_count;
  private uint burst_callback_id;

  /**
   * Take a photo during burst mode, and increment the burst count.
   *
   * @return true if there are more photos to be taken in the current burst,
   * false otherwise
   */
  private bool burst_take_photo ()
  {
    if (is_bursting && burst_count < settings.get_int ("burst-repeat"))
    {
      this.take_photo ();
      burst_count++;
      return true;
    }
    else
    {
      toggle_photo_bursting (false);
      return false;
    }
  }

    /**
     * Cancel the current action (if any)
     */
    private bool cancel_running_action ()
    {
        if ((current_countdown != null && current_countdown.running)
            || is_bursting || is_recording)
        {
            action_cancelled = true;

            switch (current_mode)
            {
                case MediaMode.PHOTO:
                    current_countdown.stop ();
                    finish_countdown_callback ();
                    break;
                case MediaMode.BURST:
                    toggle_photo_bursting (false);
                    break;
                case MediaMode.VIDEO:
                    toggle_video_recording (false);
                    break;
            }

            action_cancelled = false;

            return true;
        }

        return false;
    }

  /**
   * Cancel the current activity if the escape key is pressed.
   *
   * @param event the key event, to check which key was pressed
   * @return false, to allow further processing of the event
   */
  private bool on_key_release (Gdk.EventKey event)
  {
    string key;

    key = Gdk.keyval_name (event.keyval);
    if (strcmp (key, "Escape") == 0)
    {
      if (cancel_running_action ())
      {
        return false;
      }
      else if (is_effects_selector_active)
      {
        effects_toggle_button.set_active (false);
      }
    }
    return false;
  }

  /**
   * Toggle whether video recording is active.
   *
   * @param is_start whether to start video recording
   */
  public void toggle_video_recording (bool is_start)
  {
    if (is_start)
    {
      camera.start_video_recording (fileutil.get_new_media_filename (this.current_mode));
      /* Will be called every 1 second while
       * update_timeout_layer returns true.
       */
      Timeout.add_seconds (1, update_timeout_layer);
      take_action_button.tooltip_text = _("Stop recording");
      take_action_button_image.set_from_icon_name ("media-playback-stop-symbolic", Gtk.IconSize.BUTTON);
      this.is_recording = true;
      this.disable_mode_change ();
    }
    else
    {
      camera.stop_video_recording ();
      /* The timeout_layer always shows the "00:00:00"
       * string when not recording, in order to notify
       * the user about two things:
       *   + The user is making use of the recording mode.
       *   + The user is currently not recording.
       */
      timeout_layer.text = "00:00:00";
      take_action_button.tooltip_text = _("Record a video");
      take_action_button_image.set_from_icon_name ("camera-web-symbolic", Gtk.IconSize.BUTTON);
      this.is_recording = false;
      this.enable_mode_change ();
    }
  }

  /**
   * Update the timeout layer displayed timer.
   *
   * @return false, if the source, Timeout.add_seconds (used
   * in the toogle_video_recording method), should be removed.
   */
  private bool update_timeout_layer ()
  {
    if (is_recording) {
      timeout_layer.text = camera.get_recorded_time ();
      return true;
    }
    else
      return false;
  }

  /**
   * Toggle whether photo bursting is active.
   *
   * @param is_start whether to start capturing a photo burst
   */
  public void toggle_photo_bursting (bool is_start)
  {
    if (is_start)
    {
      is_bursting = true;
      this.disable_mode_change ();
      // FIXME: Set the effects action to be inactive.
      take_action_button.tooltip_text = _("Stop taking pictures");
      burst_take_photo ();

      /* Use the countdown duration if it is greater than the burst delay, plus
       * about 500 ms for taking the photo. */
      var burst_delay = settings.get_int ("burst-delay");
      var countdown_duration = 500 + settings.get_int ("countdown-duration") * 1000;
      if ((burst_delay - countdown_duration) < 1000 && settings.get_boolean ("countdown"))
      {
        burst_callback_id = GLib.Timeout.add (countdown_duration, burst_take_photo);
      }
      else
      {
        burst_callback_id = GLib.Timeout.add (burst_delay, burst_take_photo);
      }
    }
    else
    {
      if (current_countdown != null && current_countdown.running)
        current_countdown.stop ();

      is_bursting = false;
      this.enable_mode_change ();
      take_action_button.tooltip_text = _("Take multiple photos");
      burst_count = 0;
      fileutil.reset_burst ();
      GLib.Source.remove (burst_callback_id);
    }
  }

    /**
     * Take a photo or burst of photos, or record a video, based on the current
     * capture mode.
     */
    public void shoot ()
    {
        switch (current_mode)
        {
            case MediaMode.PHOTO:
                take_photo ();
                break;
            case MediaMode.VIDEO:
                toggle_video_recording (!is_recording);
                break;
            case MediaMode.BURST:
                toggle_photo_bursting (!is_bursting);
                break;
            default:
                assert_not_reached ();
        }
    }

    /**
     * Show an error.
     *
     * @param error the error to display, or null to hide the error layer
     */
    public void show_error (string? error)
    {
        if (error != null)
        {
            current_effects_grid.hide ();
            video_preview.hide ();
            error_layer.text = error;
            error_layer.show ();
        }
        else
        {
            error_layer.hide ();

            if (is_effects_selector_active)
            {
                current_effects_grid.show ();
            }
            else
            {
                video_preview.show ();
            }
        }
    }

    /**
     * Toggle the display of the effect selector.
     *
     * @param effects whether effects should be enabled
     */
    public void set_effects (bool effects)
    {
        toggle_effects_selector (effects);
    }

  /**
   * Change the selected effect, as a new one was selected.
   *
   * @param tap unused
   * @param source the actor (with associated effect) that was selected
   */
  public void on_selected_effect_change (Clutter.TapAction tap,
                                         Clutter.Actor source)
  {
    /* Disable the effects selector after selecting an effect. */
    effects_toggle_button.set_active (false);

    selected_effect = source.get_data ("effect");
    camera.set_effect (selected_effect);
    settings.set_string ("selected-effect", selected_effect.name);
  }

    /**
     * Navigate back one page of effects.
     */
    private void on_effects_previous ()
    {
        if (is_previous_effects_page ())
        {
            activate_effects_page ((int)current_effects_page - 1);
        }
    }

    /**
     * Navigate forward one page of effects.
     */
    private void on_effects_next ()
    {
        if (is_next_effects_page ())
        {
            activate_effects_page ((int)current_effects_page + 1);
        }
    }

  /**
   * Switch to the supplied page of effects.
   *
   * @param number the effects page to switch to
   */
  private void activate_effects_page (int number)
  {
    if (!is_effects_selector_active)
      return;
    current_effects_page = number;
    if (viewport_layout.get_children ().index (current_effects_grid) != -1)
    {
      viewport_layout.remove_child (current_effects_grid);
    }
    current_effects_grid = effects_grids.nth_data (number);
    current_effects_grid.opacity = 0;
    viewport_layout.add_child (current_effects_grid);
    current_effects_grid.save_easing_state ();
    current_effects_grid.set_easing_mode (Clutter.AnimationMode.LINEAR);
    current_effects_grid.set_easing_duration (500);
    current_effects_grid.opacity = 255;
    current_effects_grid.restore_easing_state ();


    uint i = 0;
    foreach (var effect in effects_manager.effects)
    {
        uint page_nr = i / EFFECTS_PER_PAGE;
        if (page_nr == number)
        {
            if (!effect.is_preview_connected ())
            {
                Clutter.Texture texture = effect.get_data<Clutter.Texture> ("texture");
                camera.connect_effect_texture (effect, texture);
            }
            effect.enable_preview ();
        }
        else
        {
            if (effect.is_preview_connected ())
            {
                effect.disable_preview ();
            }
        }

	    i++;
    }

    setup_effects_page_switch_sensitivity ();
  }

    /**
     * Control the sensitivity of the effects page navigation buttons.
     */
    private void setup_effects_page_switch_sensitivity ()
    {
        var effects_next = this.lookup_action ("effects-next") as SimpleAction;
        var effects_previous = this.lookup_action ("effects-previous") as SimpleAction;

        effects_next.set_enabled (is_effects_selector_active
                                  && is_next_effects_page ());
        effects_previous.set_enabled (is_effects_selector_active
                                      && is_previous_effects_page ());
    }

    private bool is_next_effects_page ()
    {
        // Calculate the number of effects visible up to the current page.
        return (current_effects_page + 1) * EFFECTS_PER_PAGE < effects_manager.effects.length ();
    }

    private bool is_previous_effects_page ()
    {
        return current_effects_page != 0;
    }

    /**
     * Toggle the visibility of the effects selector.
     *
     * @param active whether the selector should be active
     */
    private void toggle_effects_selector (bool active)
    {
        is_effects_selector_active = active;

        if (effects_grids.length () == 0)
        {
            show_error (active ? _("No effects found") : null);
        }
        else if (active)
        {
            video_preview.hide ();
            current_effects_grid.show ();
            activate_effects_page ((int)current_effects_page);
        }
        else
        {
            current_effects_grid.hide ();
            video_preview.show ();
        }

        camera.toggle_effects_pipeline (active);
        setup_effects_page_switch_sensitivity ();
        update_header_bar_title ();
    }

  /**
   * Create the effects selector.
   */
  private void setup_effects_selector ()
  {
    if (current_effects_grid == null)
    {
      effects_manager = new EffectsManager ();
      effects_manager.load_effects ();

      /* Must initialize effects_grids before returning, as it is dereferenced later, bug 654671. */
      effects_grids = new List<Clutter.Actor> ();

      if (effects_manager.effects.length () == 0)
      {
        warning ("gnome-video-effects is not installed.");
        return;
      }

      foreach (var effect in effects_manager.effects)
      {
          Clutter.GridLayout grid_layout = new GridLayout ();
          var grid = new Clutter.Actor ();
          grid.set_layout_manager (grid_layout);
          effects_grids.append (grid);
          grid_layout.set_column_spacing (10);
          grid_layout.set_row_spacing (10);
      }

      uint i = 0;
      foreach (var effect in effects_manager.effects)
      {
        Clutter.Texture   texture = new Clutter.Texture ();
        Clutter.BinLayout layout  = new Clutter.BinLayout (Clutter.BinAlignment.CENTER,
                                                           Clutter.BinAlignment.CENTER);
        var box = new Clutter.Actor ();
        box.set_layout_manager (layout);
        Clutter.Text      text = new Clutter.Text ();
        var rect = new Clutter.Actor ();

        rect.opacity = 128;
        rect.background_color = Clutter.Color.from_string ("black");

        texture.keep_aspect_ratio = true;
        box.add_child (texture);
        box.reactive = true;
        var tap = new Clutter.TapAction ();
        box.add_action (tap);
        tap.tap.connect (on_selected_effect_change);
        box.set_data ("effect", effect);
        effect.set_data ("texture", texture);

        text.text  = effect.name;
        text.color = Clutter.Color.from_string ("white");

        rect.height = text.height + 5;
        rect.x_align = Clutter.ActorAlign.FILL;
        rect.y_align = Clutter.ActorAlign.END;
        rect.x_expand = true;
        rect.y_expand = true;
        box.add_child (rect);

        text.x_align = Clutter.ActorAlign.CENTER;
        text.y_align = Clutter.ActorAlign.END;
        text.x_expand = true;
        text.y_expand = true;
        box.add_child (text);

        var grid_layout = effects_grids.nth_data (i / EFFECTS_PER_PAGE).layout_manager as GridLayout;
        grid_layout.attach (box, ((int)(i % EFFECTS_PER_PAGE)) % 3,
                            ((int)(i % EFFECTS_PER_PAGE)) / 3, 1, 1);

        i++;
      }

      setup_effects_page_switch_sensitivity ();
      current_effects_grid = effects_grids.nth_data (0);
    }
  }

    /**
     * Update the UI when the camera starts playing.
     */
    public void camera_state_change_playing ()
    {
        show_error (null);

        Effect effect = effects_manager.get_effect (settings.get_string ("selected-effect"));
        if (effect != null)
        {
            camera.set_effect (effect);
        }
    }

    /**
     * Report an error as the camerabin switched to the NULL state.
     */
    public void camera_state_change_null ()
    {
        cancel_running_action ();

        if (!error_layer.visible)
        {
            show_error (_("There was an error playing video from the webcam"));
        }
    }

  /**
   * Load the UI from the GtkBuilder description.
   */
  public void setup_ui ()
  {
        clutter_builder = new Clutter.Script ();
    fileutil        = new FileUtil ();
    flash           = new Flash (this);
    settings        = new GLib.Settings ("org.gnome.Cheese");

        var menu = new GLib.Menu ();
        var section = new GLib.Menu ();
        menu.append_section (null, section);
        var item = new GLib.MenuItem (_("Open"), "win.file-open");
        item.set_attribute ("accel", "s", "<Primary>o");
        section.append_item (item);
        item = new GLib.MenuItem (_("Save _As…"), "win.file-saveas");
        item.set_attribute ("accel", "s", "<Primary>S");
        section.append_item (item);
        item = new GLib.MenuItem (_("Move to _Trash"), "win.file-trash");
        item.set_attribute ("accel", "s", "Delete");
        section.append_item (item);
        item = new GLib.MenuItem (_("Delete"), "win.file-delete");
        item.set_attribute ("accel", "s", "<Shift>Delete");
        section.append_item (item);
        thumbnail_popup = new Gtk.Menu.from_model (menu);

        this.add_action_entries (actions, this);

        try
        {
            clutter_builder.load_from_resource ("/org/gnome/Cheese/cheese-viewport.json");
        }
        catch (Error err)
        {
            error ("Error: %s", err.message);
        }

        viewport = viewport_widget.get_stage () as Clutter.Stage;

        video_preview = clutter_builder.get_object ("video_preview") as Clutter.Texture;
        viewport_layout = clutter_builder.get_object ("viewport_layout") as Clutter.Actor;
        viewport_layout_manager = clutter_builder.get_object ("viewport_layout_manager") as Clutter.BinLayout;
        countdown_layer = clutter_builder.get_object ("countdown_layer") as Clutter.Text;
        background_layer = clutter_builder.get_object ("background") as Clutter.Actor;
        error_layer = clutter_builder.get_object ("error_layer") as Clutter.Text;
        timeout_layer = clutter_builder.get_object ("timeout_layer") as Clutter.Text;

    video_preview.keep_aspect_ratio = true;
    video_preview.request_mode      = Clutter.RequestMode.HEIGHT_FOR_WIDTH;
    viewport.add_child (background_layer);
    viewport_layout.set_layout_manager (viewport_layout_manager);

    viewport.add_child (viewport_layout);
    viewport.add_child (timeout_layer);

    viewport.allocation_changed.connect (on_stage_resize);

    thumb_view = new Cheese.ThumbView ();
    thumb_nav  = new Eog.ThumbNav (thumb_view, false);
        thumbnail_popup.attach_to_widget (thumb_view, null);

        Gtk.CssProvider css;
        try
        {
            var file = File.new_for_uri ("resource:///org/gnome/Cheese/cheese.css");
            css = new Gtk.CssProvider ();
            css.load_from_file (file);
        }
        catch (Error e)
        {
            // TODO: Use parsing-error signal.
            error ("Error parsing CSS: %s\n", e.message);
        }

    Gtk.StyleContext.add_provider_for_screen (screen, css, STYLE_PROVIDER_PRIORITY_USER);

    thumb_view.button_press_event.connect (on_thumbnail_button_press_event);

    /* needed for the sizing tricks in set_wide_mode (allocation is 0
     * if the widget is not realized */
    viewport_widget.realize ();

    set_wide_mode (false);

    setup_effects_selector ();

    this.key_release_event.connect (on_key_release);
  }

    public Clutter.Texture get_video_preview ()
    {
        return video_preview;
    }

  /**
   * Setup the thumbview thumbnail monitors.
   */
  public void start_thumbview_monitors ()
  {
    thumb_view.start_monitoring_video_path (fileutil.get_video_path ());
    thumb_view.start_monitoring_photo_path (fileutil.get_photo_path ());
  }

    /**
     * Set the current media mode (photo, video or burst).
     *
     * @param mode the media mode to set
     */
    public void set_current_mode (MediaMode mode)
    {
        current_mode = mode;

        set_resolution (current_mode);
        update_header_bar_title ();
        timeout_layer.hide ();

        switch (current_mode)
        {
            case MediaMode.PHOTO:
                take_action_button.tooltip_text = _("Take a photo using a webcam");
                break;

            case MediaMode.VIDEO:
                take_action_button.tooltip_text = _("Record a video using a webcam");
                timeout_layer.text = "00:00:00";
                timeout_layer.show ();
                break;

            case MediaMode.BURST:
                take_action_button.tooltip_text = _("Take multiple photos using a webcam");
                break;
        }
    }

     /**
     * Set the header bar title.
     */
    private void update_header_bar_title ()
    {
        if (is_effects_selector_active)
        {
            set_window_title (_("Choose an Effect"));
        }
        else
        {
            switch (current_mode)
            {
                case MediaMode.PHOTO:
                    set_window_title (_("Take a Photo"));
                    break;

                case MediaMode.VIDEO:
                    set_window_title (_("Record a Video"));
                    break;

                case MediaMode.BURST:
                    set_window_title (_("Take Multiple Photos"));
                    break;
            }
        }
    }
    /**
     * Set the camera.
     *
     * @param camera the camera to set
     */
    public void set_camera (Camera camera)
    {
        this.camera = camera;
    }
}
