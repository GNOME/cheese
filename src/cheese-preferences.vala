using Gtk;

internal class Cheese.PreferencesDialog : GLib.Object
{
  private Cheese.Camera camera;
  private Cheese.GConf  conf;

  private Gtk.Dialog dialog;

  private Gtk.ComboBox resolution_combo;
  private Gtk.ComboBox source_combo;

  private Gtk.Adjustment brightness_adjustment;
  private Gtk.Adjustment contrast_adjustment;
  private Gtk.Adjustment hue_adjustment;
  private Gtk.Adjustment saturation_adjustment;

  private Gtk.SpinButton burst_count_spin;
  private Gtk.SpinButton burst_delay_spin;

  private Gtk.Button close_button;

  public PreferencesDialog (Cheese.Camera camera, Cheese.GConf conf)
  {
    this.camera = camera;
    this.conf   = conf;

    Gtk.Builder builder = new Gtk.Builder ();
    builder.add_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-prefs.ui"));
    builder.connect_signals (this);

    this.dialog = (Gtk.Dialog)builder.get_object ("cheese_prefs_dialog");

    this.brightness_adjustment = (Gtk.Adjustment)builder.get_object ("brightness_adjustment");
    this.contrast_adjustment   = (Gtk.Adjustment)builder.get_object ("contrast_adjustment");
    this.hue_adjustment        = (Gtk.Adjustment)builder.get_object ("hue_adjustment");
    this.saturation_adjustment = (Gtk.Adjustment)builder.get_object ("saturation_adjustment");

    this.resolution_combo = (Gtk.ComboBox)builder.get_object ("resolution_combo_box");
    this.source_combo     = (Gtk.ComboBox)builder.get_object ("camera_combo_box");

    this.burst_count_spin = (Gtk.SpinButton)builder.get_object ("burst_count");
    this.burst_delay_spin = (Gtk.SpinButton)builder.get_object ("burst_delay");

    this.close_button = (Gtk.Button)builder.get_object ("close");
  }

  [CCode (instance_pos = -1)]
  internal void on_dialog_close (Gtk.Button button)
  {
    this.dialog.hide_all ();
  }

  public void show ()
  {
    this.dialog.show_all ();
  }
}
