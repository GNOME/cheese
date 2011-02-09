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

public class Cheese.PreferencesDialog : GLib.Object
{
  private Cheese.Camera camera;

  private GLib.Settings settings;

  private Gtk.Dialog dialog;

  private Gtk.ComboBox photo_resolution_combo;
  private Gtk.ComboBox video_resolution_combo;
  private Gtk.ComboBox source_combo;

  private Gtk.Adjustment brightness_adjustment;
  private Gtk.Adjustment contrast_adjustment;
  private Gtk.Adjustment hue_adjustment;
  private Gtk.Adjustment saturation_adjustment;

  private Gtk.Scale brightness_scale;
  private Gtk.Scale contrast_scale;
  private Gtk.Scale hue_scale;
  private Gtk.Scale saturation_scale;

  private Gtk.SpinButton burst_repeat_spin;
  private Gtk.SpinButton burst_delay_spin;

  private Gtk.CheckButton countdown_check;
  private Gtk.CheckButton flash_check;
  
  private MediaMode current_mode;

  public PreferencesDialog (Cheese.Camera camera, GLib.Settings settings)
  {
    this.camera = camera;
    this.settings   = settings;

    Gtk.Builder builder = new Gtk.Builder ();
    try
    {
      builder.add_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-prefs.ui"));
    }
    catch (Error err)
    {
      error ("Error: %s", err.message);
    }

    this.dialog = (Gtk.Dialog)builder.get_object ("cheese_prefs_dialog");

    this.brightness_adjustment = (Gtk.Adjustment) builder.get_object ("brightness_adjustment");
    this.contrast_adjustment   = (Gtk.Adjustment) builder.get_object ("contrast_adjustment");
    this.hue_adjustment        = (Gtk.Adjustment) builder.get_object ("hue_adjustment");
    this.saturation_adjustment = (Gtk.Adjustment) builder.get_object ("saturation_adjustment");

    /* Here instead of in cheese-prefs.ui because of https://bugzilla.gnome.org/show_bug.cgi?id=624443 */

    this.brightness_scale = (Gtk.Scale) builder.get_object ("brightness_scale");
    this.contrast_scale   = (Gtk.Scale) builder.get_object ("contrast_scale");
    this.hue_scale        = (Gtk.Scale) builder.get_object ("hue_scale");
    this.saturation_scale = (Gtk.Scale) builder.get_object ("saturation_scale");

    this.brightness_scale.add_mark (0, Gtk.PositionType.BOTTOM, null);
    this.contrast_scale.add_mark (1, Gtk.PositionType.BOTTOM, null);
    this.hue_scale.add_mark (0, Gtk.PositionType.BOTTOM, null);
    this.saturation_scale.add_mark (1, Gtk.PositionType.BOTTOM, null);

    this.photo_resolution_combo = (Gtk.ComboBox) builder.get_object ("photo_resolution_combo_box");
    this.video_resolution_combo = (Gtk.ComboBox) builder.get_object ("video_resolution_combo_box");
    this.source_combo     = (Gtk.ComboBox) builder.get_object ("camera_combo_box");

    this.burst_repeat_spin = (Gtk.SpinButton) builder.get_object ("burst_repeat");
    this.burst_delay_spin  = (Gtk.SpinButton) builder.get_object ("burst_delay");

    this.countdown_check  = (Gtk.CheckButton) builder.get_object ("countdown");
    this.flash_check  = (Gtk.CheckButton) builder.get_object ("flash");

    setup_combo_box_models ();
    initialize_camera_devices ();
    initialize_values_from_settings ();

    /*
     * Connect signals only after all the widgets have been setup
     * Stops a bunch of unnecessary signals from being fired
     */
    builder.connect_signals (this);
  }

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

  private void initialize_camera_devices ()
  {
    Cheese.CameraDevice   dev;
    unowned GLib.PtrArray devices      = camera.get_camera_devices ();
    ListStore             camera_model = new ListStore (2, typeof (string), typeof (Cheese.CameraDevice));

    source_combo.model = camera_model;
    if (devices.len <= 1)
      source_combo.sensitive = false;

    for (int i = 0; i < devices.len; i++)
    {
      TreeIter iter;
      dev = (Cheese.CameraDevice) devices.index (i);
      camera_model.append (out iter);
      camera_model.set (iter,
                        0, dev.get_name () + " (" + dev.get_device_file () + " )",
                        1, dev);
      if (camera.get_selected_device ().get_device_file () == dev.get_device_file ())
      {
        source_combo.set_active_iter (iter);
      }
    }

    settings.set_string ("camera", camera.get_selected_device ().get_device_file ());
    setup_resolutions_for_device (camera.get_selected_device ());
  }

  private void setup_resolutions_for_device (Cheese.CameraDevice device)
  {
    unowned List<VideoFormat>  formats = device.get_format_list ();
    unowned Cheese.VideoFormat format;
    ListStore                  resolution_model = new ListStore (2, typeof (string), typeof (Cheese.VideoFormat));

    photo_resolution_combo.model = resolution_model;
    video_resolution_combo.model = resolution_model;

    for (int i = 0; i < formats.length (); i++)
    {
      TreeIter iter;
      format = formats<VideoFormat>.nth (i).data;
      resolution_model.append (out iter);
      resolution_model.set (iter,
                            0, format.width.to_string () + " x " + format.height.to_string (),
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
    }

    /* Video resolution combo shows photo resolution by
    *  default if previous user choice is not found in settings or not supported
    *  by current device. These values are saved to settings.
    */
    if (video_resolution_combo.get_active () == -1)
    {
      video_resolution_combo.set_active (photo_resolution_combo.get_active ());
      settings.set_int ("video-x-resolution", settings.get_int ("photo-x-resolution"));
      settings.set_int ("video-y-resolution", settings.get_int ("photo-y-resolution"));
    }
  }

  private void initialize_values_from_settings ()
  {
    brightness_adjustment.value = settings.get_double ("brightness");
    contrast_adjustment.value   = settings.get_double ("contrast");
    hue_adjustment.value        = settings.get_double ("hue");
    saturation_adjustment.value = settings.get_double ("saturation");

    burst_repeat_spin.value = settings.get_int ("burst-repeat");
    burst_delay_spin.value  = settings.get_int ("burst-delay") / 1000;

    countdown_check.active = settings.get_boolean ("countdown");
    flash_check.active = settings.get_boolean ("flash");
  }

  [CCode (instance_pos = -1)]
  public void on_source_change (Gtk.ComboBox combo)
  {
    TreeIter iter;

    Cheese.CameraDevice dev;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out dev);
    camera.set_device_by_dev_file (dev.get_device_file ());
    camera.switch_camera_device ();
    setup_resolutions_for_device (camera.get_selected_device ());
    settings.set_string ("camera", dev.get_device_file ());
  }

  [CCode (instance_pos = -1)]
  public void on_photo_resolution_change (Gtk.ComboBox combo)
  {
    TreeIter iter;

    unowned Cheese.VideoFormat format;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out format);
    
    if (current_mode == MediaMode.PHOTO || current_mode == MediaMode.BURST)
      camera.set_video_format (format);

    settings.set_int ("photo-x-resolution", format.width);
    settings.set_int ("photo-y-resolution", format.height);
  }

  [CCode (instance_pos = -1)]
  public void on_video_resolution_change (Gtk.ComboBox combo)
  {
    TreeIter iter;

    unowned Cheese.VideoFormat format;

    combo.get_active_iter (out iter);
    combo.model.get (iter, 1, out format);
    
    if (current_mode == MediaMode.VIDEO)
      camera.set_video_format (format);

    settings.set_int ("video-x-resolution", format.width);
    settings.set_int ("video-y-resolution", format.height);
  }

  [CCode (instance_pos = -1)]
  public void on_dialog_close (Gtk.Button button)
  {
    this.dialog.hide ();
  }

  [CCode (instance_pos = -1)]
  public void on_countdown_toggle (Gtk.CheckButton checkbutton)
  {
    settings.set_boolean ("countdown", checkbutton.active);
  }

  [CCode (instance_pos = -1)]
  public void on_flash_toggle (Gtk.CheckButton checkbutton)
  {
    settings.set_boolean ("flash", checkbutton.active);
  }

  [CCode (instance_pos = -1)]
  public void on_burst_repeat_change (Gtk.SpinButton spinbutton)
  {
    settings.set_int ("burst-repeat", (int) spinbutton.value);
  }

  [CCode (instance_pos = -1)]
  public void on_burst_delay_change (Gtk.SpinButton spinbutton)
  {
    settings.set_int ("burst-delay", (int) spinbutton.value * 1000);
  }

  [CCode (instance_pos = -1)]
  public void on_brightness_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("brightness", adjustment.value);
    settings.set_double ("brightness", adjustment.value);
  }

  [CCode (instance_pos = -1)]
  public void on_contrast_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("contrast", adjustment.value);
    settings.set_double ("contrast", adjustment.value);
  }

  [CCode (instance_pos = -1)]
  public void on_hue_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("hue", adjustment.value);
    settings.set_double ("hue", adjustment.value);
  }

  [CCode (instance_pos = -1)]
  public void on_saturation_change (Gtk.Adjustment adjustment)
  {
    this.camera.set_balance_property ("saturation", adjustment.value);
    settings.set_double ("saturation", adjustment.value);
  }

  public void show ()
  {
    this.dialog.show_all ();
  }
  
  public void set_current_mode (MediaMode mode)
  {
    this.current_mode = mode;
  }
  
}
