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

[GtkTemplate (ui = "/org/gnome/Cheese/cheese-prefs.ui")]
public class Cheese.PreferencesDialog : Gtk.Dialog
{
    private Cheese.Camera camera;

    private GLib.Settings settings;

    [GtkChild]
    private Gtk.ComboBox photo_resolution_combo;
    [GtkChild]
    private Gtk.ComboBox video_resolution_combo;
    [GtkChild]
    private Gtk.ComboBox source_combo;

    private Gtk.ListStore camera_model;

    [GtkChild]
    private Gtk.Adjustment brightness_adjustment;
    [GtkChild]
    private Gtk.Adjustment contrast_adjustment;
    [GtkChild]
    private Gtk.Adjustment hue_adjustment;
    [GtkChild]
    private Gtk.Adjustment saturation_adjustment;

    [GtkChild]
    private Gtk.SpinButton burst_repeat_spin;
    [GtkChild]
    private Gtk.SpinButton burst_delay_spin;

    [GtkChild]
    private Gtk.CheckButton countdown_check;
    [GtkChild]
    private Gtk.CheckButton flash_check;
  
    private MediaMode current_mode;

public PreferencesDialog (Cheese.Camera camera)
{
    this.camera = camera;

    settings = new GLib.Settings ("org.gnome.Cheese");

    setup_combo_box_models ();
    initialize_camera_devices ();
    initialize_values_from_settings ();

    /*
     * Connect signals only after all the widgets have been setup
     * Stops a bunch of unnecessary signals from being fired
     */
    camera.notify["num-camera-devices"].connect(this.on_camera_update_num_camera_devices);
}

  /**
   * Set up combo box cell renderers.
   */
  private void setup_combo_box_models ()
  {
    CellRendererText cell = new CellRendererText ();

    photo_resolution_combo.pack_start (cell, false);
    photo_resolution_combo.set_attributes (cell, "text", 0);

    video_resolution_combo.pack_start (cell, false);
    video_resolution_combo.set_attributes (cell, "text", 0);

    source_combo.pack_start (cell, false);
    source_combo.set_attributes (cell, "text", 0);
  }

  /**
   * Initialize and populate the camera device combo box model.
   */
  private void initialize_camera_devices ()
  {
    unowned GLib.PtrArray devices = camera.get_camera_devices ();
    camera_model = new ListStore (2, typeof (string), typeof (Cheese.CameraDevice));

    source_combo.model = camera_model;
    source_combo.sensitive = false;

    devices.foreach(add_camera_device);

    settings.set_string ("camera", camera.get_selected_device ().get_device_node ());
    setup_resolutions_for_device (camera.get_selected_device ());
  }

  /**
   * Initialize and populate the resolution combo box models for the device.
   *
   * @param device the Cheese.CameraDevice for which to enumerate resolutions
   */
  private void setup_resolutions_for_device (Cheese.CameraDevice device)
  {
    var formats = device.get_format_list ();
    ListStore resolution_model = new ListStore (2, typeof (string),
        typeof (Cheese.VideoFormat));

    photo_resolution_combo.model = resolution_model;
    video_resolution_combo.model = resolution_model;

    formats.foreach ((format) =>
    {
      TreeIter iter;
      resolution_model.append (out iter);
      resolution_model.set (iter,
                            0, format.width.to_string () + " × " + format.height.to_string (),
                            1, format);
      if (camera.get_current_video_format ().width == format.width &&
          camera.get_current_video_format ().height == format.height)
      {
        photo_resolution_combo.set_active_iter (iter);
        settings.set_int ("photo-x-resolution", format.width);
        settings.set_int ("photo-y-resolution", format.height);
      }

      if (settings.get_int ("video-x-resolution") == format.width &&
          settings.get_int ("video-y-resolution") == format.height)
      {
        video_resolution_combo.set_active_iter (iter);
      }
    });

    /* Video resolution combo shows photo resolution by default if previous
     * user choice is not found in settings or not supported by current device.
     * These values are saved to settings.
     */
    if (video_resolution_combo.get_active () == -1)
    {
      video_resolution_combo.set_active (photo_resolution_combo.get_active ());
      settings.set_int ("video-x-resolution", settings.get_int ("photo-x-resolution"));
      settings.set_int ("video-y-resolution", settings.get_int ("photo-y-resolution"));
    }
  }

    /**
     * Take the user preferences from GSettings.
     */
    private void initialize_values_from_settings ()
    {
      brightness_adjustment.value = settings.get_double ("brightness");
      contrast_adjustment.value   = settings.get_double ("contrast");
      hue_adjustment.value        = settings.get_double ("hue");
      saturation_adjustment.value = settings.get_double ("saturation");

        settings.bind ("burst-repeat", burst_repeat_spin, "value",
                       SettingsBindFlags.DEFAULT);
      burst_delay_spin.value  = settings.get_int ("burst-delay") / 1000;

        settings.bind ("countdown", countdown_check, "active",
                       SettingsBindFlags.DEFAULT);
        settings.bind ("flash", flash_check, "active",
                       SettingsBindFlags.DEFAULT);
    }

  /**
   * Update the active device to the active iter of the device combo box.
   *
   * @param combo the video device combo box
   */
  [GtkCallback]
  private void on_source_change (Gtk.ComboBox combo)
  {
    // TODO: Handle going from 1 to 0 devices, cleanly!
    return_if_fail (camera.num_camera_devices > 0);

    TreeIter iter;
    Cheese.CameraDevice dev;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out dev);
    camera.set_device_by_device_node (dev.get_device_node ());
    camera.switch_camera_device ();
    setup_resolutions_for_device (camera.get_selected_device ());
    settings.set_string ("camera", dev.get_device_node ());
  }

  /**
   * Update the current photo capture resolution to the active iter of the
   * photo resolution combo box.
   *
   * @param combo the photo resolution combo box
   */
  [GtkCallback]
  private void on_photo_resolution_change (Gtk.ComboBox combo)
  {
    TreeIter iter;

    Cheese.VideoFormat format;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out format);
    
    if (current_mode == MediaMode.PHOTO || current_mode == MediaMode.BURST)
      camera.set_video_format (format);

    settings.set_int ("photo-x-resolution", format.width);
    settings.set_int ("photo-y-resolution", format.height);
  }

  /**
   * Update the current video capture resolution to the active iter of the
   * video resolution combo box.
   *
   * @param combo the video resolution combo box
   */
  [GtkCallback]
  private void on_video_resolution_change (Gtk.ComboBox combo)
  {
    TreeIter iter;

    Cheese.VideoFormat format;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out format);
    
    if (current_mode == MediaMode.VIDEO)
      camera.set_video_format (format);

    settings.set_int ("video-x-resolution", format.width);
    settings.set_int ("video-y-resolution", format.height);
  }

    /**
    * Hide the dialog when it is closed, rather than deleting it.
    */
    [GtkCallback]
    private bool on_delete ()
    {
        return this.hide_on_delete ();
    }

    /**
    * Hide the dialog when it is closed, rather than deleting it.
    */
    [GtkCallback]
    private void on_dialog_close (Gtk.Button button)
    {
        this.hide ();
    }

    /**
    * Show the help for the preferences dialog.
    *
    * @param button the help button
    */
    [GtkCallback]
    private void on_dialog_help (Gtk.Button button)
    {
        try
        {
            Gtk.show_uri (this.get_screen (), "help:cheese/index#preferences",
                          Gdk.CURRENT_TIME);
        }
        catch
        {
            warning ("%s", "Error showing help");
        }
    }

  /**
   * Change the burst-delay GSetting when changing the spin button.
   *
   * The burst delay is the time, in milliseconds, between individual photos in
   * a burst.
   *
   * @param spinbutton the burst-delay spin button
   */
  [GtkCallback]
  private void on_burst_delay_change (Gtk.SpinButton spinbutton)
  {
    settings.set_int ("burst-delay", (int) spinbutton.value * 1000);
  }

  /**
   * Change the brightness of the image, and update the GSetting, when changing
   * the scale.
   *
   * @param adjustment the adjustment of the brightness Gtk.Scale
   */
  [GtkCallback]
  private void on_brightness_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("brightness", adjustment.value);
    settings.set_double ("brightness", adjustment.value);
  }

  /**
   * Change the contrast of the image, and update the GSetting, when changing
   * the scale.
   *
   * @param adjustment the adjustment of the contrast Gtk.Scale
   */
  [GtkCallback]
  private void on_contrast_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("contrast", adjustment.value);
    settings.set_double ("contrast", adjustment.value);
  }

  /**
   * Change the hue of the image, and update the GSetting, when changing the
   * scale.
   *
   * @param adjustment the adjustment of the hue Gtk.Scale
   */
  [GtkCallback]
  private void on_hue_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("hue", adjustment.value);
    settings.set_double ("hue", adjustment.value);
  }

  /**
   * Change the saturation of the image, and update the GSetting, when changing
   * the scale.
   *
   * @param adjustment the adjustment of the saturation Gtk.Scale
   */
  [GtkCallback]
  private void on_saturation_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("saturation", adjustment.value);
    settings.set_double ("saturation", adjustment.value);
  }

  /**
   * Update the video device combo box when a camera device was added or
   * removed.
   */
  private void on_camera_update_num_camera_devices ()
  {
    unowned GLib.PtrArray devices = camera.get_camera_devices ();
    Cheese.CameraDevice   dev;

    // Add (if) / Remove (else) a camera device.
    if (devices.len > camera_model.iter_n_children (null))
    {
      dev = (Cheese.CameraDevice) devices.index (devices.len - 1);
      add_camera_device(dev);
    }
    else
    {
      // First camera device in the combobox.
      TreeIter iter;
      camera_model.get_iter_first (out iter);

      // Combobox active element.
      TreeIter active_iter;
      Cheese.CameraDevice active_device;
      source_combo.get_active_iter (out active_iter);
      camera_model.get (active_iter, 1, out active_device, -1);

      // Find which device was removed.
      bool device_removed = false;
      devices.foreach ((device) =>
      {
        var old_device = (Cheese.CameraDevice) device;
        Cheese.CameraDevice new_device;
        camera_model.get (iter, 1, out new_device, -1);

        // Found the device that was removed.
        if (strcmp (old_device.device_node, new_device.device_node) != 0)
        {
            remove_camera_device (iter, new_device, active_device);
            device_removed = true;
            // Remember, this is from the anonymous function!
            return;
        }
        camera_model.iter_next (ref iter);
      });

      // Special case: the last device on the list was removed.
      if (!device_removed)
      {
        Cheese.CameraDevice old_device;
        camera_model.get (iter, 1, out old_device, -1);
        remove_camera_device (iter, old_device, active_device);
      }
    }

    settings.set_string ("camera", camera.get_selected_device ().get_device_node ());
    setup_resolutions_for_device (camera.get_selected_device ());
  }

  /**
   * Add the supplied camera device to the device combo box model.
   *
   * This method is intended to be used with the foreach method of GLib
   * containers.
   *
   * @param device a Cheese.CameraDevice to add to the device combo box model
   */
  private void add_camera_device (void *device)
  {
    TreeIter iter;
    Cheese.CameraDevice dev = (Cheese.CameraDevice) device;

    camera_model.append (out iter);
    camera_model.set (iter,
                      0, dev.get_name () + " (" + dev.get_device_node () + ")",
                      1, dev);

    if (camera.get_selected_device ().get_device_node () == dev.get_device_node ())
        source_combo.set_active_iter (iter);

    if (camera_model.iter_n_children (null) > 1)
      source_combo.sensitive = true;
  }

  /**
   * Remove the supplied camera device from the device combo box model.
   *
   * @param iter the iterator of the device to remove
   * @param device_node the device to remove from the combo box model
   * @param active_device_node the currently-active camera device
   */
  private void remove_camera_device (TreeIter iter, Cheese.CameraDevice device_node,
                             Cheese.CameraDevice active_device_node)
  {
      unowned GLib.PtrArray devices = camera.get_camera_devices ();

      // Check if the camera that we want to remove, is the active one
      if (strcmp (device_node.device_node, active_device_node.device_node) == 0)
      {
        if (devices.len > 0)
          set_new_available_camera_device (iter);
        else
          this.hide ();
      }
      camera_model.remove (iter);

      if (camera_model.iter_n_children (null) <= 1)
        source_combo.sensitive = false;
  }

  /**
   * Search for an available camera device and activate it in the device combo
   * box model.
   *
   * @param iter a device in the combo box model to search either side of
   */
  private void set_new_available_camera_device (TreeIter iter)
  {
    TreeIter new_iter = iter;

    if (!camera_model.iter_next (ref new_iter))
    {
      new_iter = iter;
      camera_model.iter_previous (ref new_iter);
    }
    source_combo.set_active_iter (new_iter);
  }


    /**
     * Set the current media mode (photo, video or burst).
     *
     * The current mode is used to update the video format on the Cheese.Camera
     * when the resolution for the current mode is changed.
     *
     * @param mode the media mode to set
     */
    public void set_current_mode (MediaMode mode)
    {
        current_mode = mode;
    }
}
