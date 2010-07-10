using Gtk;
using Gdk;
using GtkClutter;
using Clutter;
using Config;
using Eog;
using Gst;
using Mx;
using Gee;

const int FULLSCREEN_TIMEOUT_INTERVAL = 5 * 1000;

public class Cheese.MainWindow : Gtk.Window
{
  private MediaMode current_mode;

  private Gtk.Builder    gtk_builder;
  private Clutter.Script clutter_builder;

  private Gtk.Widget       thumbnails;
  private GtkClutter.Embed viewport_widget;
  private Gtk.VBox         main_vbox;
  private Eog.ThumbNav     thumb_nav;
  private Cheese.ThumbView thumb_view;
  private Gtk.Frame        thumbnails_right;
  private Gtk.Frame        thumbnails_bottom;
  private Gtk.MenuBar      menubar;
  private Gtk.HBox         leave_fullscreen_button_container;
  private Gtk.ToggleButton photo_toggle_button;
  private Gtk.ToggleButton video_toggle_button;
  private Gtk.ToggleButton burst_toggle_button;
  private Gtk.Button       take_action_button;
  private Gtk.Label        take_action_button_label;
  private Gtk.Image        take_action_button_image;
  private Gtk.ToggleButton effects_toggle_button;
  private Gtk.Button       leave_fullscreen_button;
  private Gtk.HBox         buttons_area;

  private Clutter.Stage     viewport;
  private Clutter.Box       viewport_layout;
  private Clutter.Texture   video_preview;
  private Clutter.BinLayout viewport_layout_manager;
  private Clutter.Text      countdown_layer;
  private Clutter.Rectangle background_layer;

  private Mx.Grid            current_effects_grid;
  private int                current_effects_page;
  private ArrayList<Mx.Grid> effects_grids;

  private Gtk.Action       take_photo_action;
  private Gtk.Action       take_video_action;
  private Gtk.Action       take_burst_action;
  private Gtk.Action       photo_mode_action;
  private Gtk.Action       video_mode_action;
  private Gtk.Action       burst_mode_action;
  private Gtk.ToggleAction effects_toggle_action;
  private Gtk.Action       wide_mode_action;
  private Gtk.Action       countdown_action;

  private bool is_fullscreen;
  private bool is_wide_mode;
  private bool is_recording;       /* Video Recording Flag */
  private bool is_bursting;

  private Gtk.Button[] buttons;

  private Cheese.Camera         camera;
  private Cheese.FileUtil       fileutil;
  private Cheese.Flash          flash;
  private Cheese.GConf          conf;
  private Cheese.EffectsManager effects_manager;

  private Cheese.Effect selected_effect;

  [CCode (instance_pos = -1)]
  internal void on_quit (Gtk.Action action)
  {
    destroy ();
  }

  [CCode (instance_pos = -1)]
  internal void on_help_contents (Gtk.Action action)
  {
    Gdk.Screen screen;
    screen = this.get_screen ();
    try {
      Gtk.show_uri (screen, "ghelp:cheese", Gtk.get_current_event_time ());
    }catch (Error err)
    {
      warning ("Error: %s\n", err.message);
    }
  }

  [CCode (instance_pos = -1)]
  internal void on_help_about (Gtk.Action action)
  {
    /* FIXME: Close doesn't work
     * FIXME: Clicking URL In the License dialog borks. */
    Gtk.AboutDialog about_dialog;
    about_dialog         = (Gtk.AboutDialog)gtk_builder.get_object ("aboutdialog");
    about_dialog.version = Config.VERSION;
    about_dialog.show_all ();
  }

  [CCode (instance_pos = -1)]
  internal void on_layout_wide_mode (ToggleAction action)
  {
    set_wide_mode (action.active);
  }

  [CCode (instance_pos = -1)]
  internal void on_layout_fullscreen (ToggleAction action)
  {
    set_fullscreen_mode (action.active);
  }

  [CCode (instance_pos = -1)]
  internal void on_mode_change (RadioAction action)
  {
    set_mode ((MediaMode) action.value);
  }

  private void enable_mode_change ()
  {
    photo_mode_action.sensitive = true;
    video_mode_action.sensitive = true;
    burst_mode_action.sensitive = true;
  }

  private void disable_mode_change ()
  {
    switch (this.current_mode)
    {
      case MediaMode.PHOTO:
        photo_mode_action.sensitive = true;
        video_mode_action.sensitive = false;
        burst_mode_action.sensitive = false;
        break;

      case MediaMode.VIDEO:
        photo_mode_action.sensitive = false;
        video_mode_action.sensitive = true;
        burst_mode_action.sensitive = false;
        break;

      case MediaMode.BURST:
        photo_mode_action.sensitive = false;
        video_mode_action.sensitive = false;
        burst_mode_action.sensitive = true;
        break;
    }
  }

  private void set_mode (MediaMode mode)
  {
    this.current_mode = mode;
    switch (this.current_mode)
    {
      case MediaMode.PHOTO:
        take_photo_action.sensitive       = true;
        take_video_action.sensitive       = false;
        take_burst_action.sensitive       = false;
        take_action_button.related_action = take_photo_action;
        break;

      case MediaMode.VIDEO:
        take_photo_action.sensitive       = false;
        take_video_action.sensitive       = true;
        take_burst_action.sensitive       = false;
        take_action_button.related_action = take_video_action;
        break;

      case MediaMode.BURST:
        take_photo_action.sensitive       = false;
        take_video_action.sensitive       = false;
        take_burst_action.sensitive       = true;
        take_action_button.related_action = take_burst_action;
        break;
    }
    take_action_button_label.label = "<b>" + take_action_button.related_action.label + "</b>";
  }

  private TimeoutSource fullscreen_timeout;
  private void clear_fullscreen_timeout ()
  {
    if (fullscreen_timeout != null)
    {
      fullscreen_timeout.destroy ();
      fullscreen_timeout = null;
    }
  }

  private void set_fullscreen_timeout ()
  {
    fullscreen_timeout = new TimeoutSource (FULLSCREEN_TIMEOUT_INTERVAL);
    fullscreen_timeout.attach (null);
    fullscreen_timeout.set_callback (() => { buttons_area.hide ();
                                             clear_fullscreen_timeout ();
                                             return true;});
  }

  private bool fullscreen_motion_notify_callback (Gtk.Widget viewport, EventMotion e)
  {
    /* Start a new timeout at the end of every mouse pointer movement.
     * So all timeouts will be cancelled, except one at the last pointer movement.
     * We call show even if the button isn't hidden. */
    clear_fullscreen_timeout ();
    buttons_area.show ();
    set_fullscreen_timeout ();
    return true;
  }

  private void set_fullscreen_mode (bool fullscreen_mode)
  {
    /* After the first time the window has been shown using this.show_all(),
     * the value of leave_fullscreen_button_container.no_show_all should be set to false
     * So that the next time leave_fullscreen_button_container.show_all() is called, the button is actually shown
     * FIXME: If this code can be made cleaner/clearer, please do */

    is_fullscreen = fullscreen_mode;
    if (fullscreen_mode)
    {
      if (is_wide_mode)
      {
        thumbnails_right.hide_all ();
      }
      else
      {
        thumbnails_bottom.hide_all ();
      }
      menubar.hide_all ();
      leave_fullscreen_button_container.no_show_all = false;
      leave_fullscreen_button_container.show_all ();

      /* Make all buttons look 'flat' */
      foreach (Gtk.Button b in buttons)
      {
        b.relief = Gtk.ReliefStyle.NONE;
      }
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
      menubar.show_all ();
      leave_fullscreen_button_container.hide_all ();

      /* Make all buttons look, uhm, Normal */
      foreach (Gtk.Button b in buttons)
      {
        b.relief = Gtk.ReliefStyle.NORMAL;
      }

      /* Show the buttons area anyway - it might've been hidden in fullscreen mode */
      buttons_area.show ();
      viewport_widget.motion_notify_event.disconnect (fullscreen_motion_notify_callback);
      this.unfullscreen ();
    }
  }

  private void set_wide_mode (bool wide_mode, bool initialize = false)
  {
    is_wide_mode              = wide_mode;
    conf.gconf_prop_wide_mode = wide_mode;
    if (!initialize)
    {
      /* Sets requested size of the viewport_widget to be it's current size
       * So it does not grow smaller with each toggle */
      Gtk.Allocation alloc;
      viewport_widget.get_allocation (out alloc);
      viewport_widget.set_size_request (alloc.width, alloc.height);
    }

    if (is_wide_mode)
    {
      thumb_view.set_columns (1);
      thumb_nav.set_vertical (true);
      if (thumbnails_bottom.get_child () != null)
      {
        thumbnails_bottom.remove (thumb_nav);
      }
      thumbnails_right.add (thumb_nav);
      thumbnails_right.resize_children ();
      thumbnails_right.show_all ();
      thumbnails_bottom.hide_all ();
    }
    else
    {
      thumb_view.set_columns (5000);
      thumb_nav.set_vertical (false);
      if (thumbnails_right.get_child () != null)
      {
        thumbnails_right.remove (thumb_nav);
      }
      thumbnails_bottom.add (thumb_nav);
      thumbnails_bottom.resize_children ();
      thumbnails_right.hide_all ();
      thumbnails_bottom.show_all ();
    }

    if (!initialize)
    {
      /* Makes sure that the window is the size it ought to be, and no bigger/smaller
       * Used to make sure that the viewport_widget does not keep growing with each toggle */
      Gtk.Requisition req;
      this.size_request (out req);
      this.resize (req.width, req.height);
    }
  }

  /* To make sure that the layout manager manages the entire stage. */
  internal void on_stage_resize (Clutter.Actor           actor,
                                 Clutter.ActorBox        box,
                                 Clutter.AllocationFlags flags)
  {
    this.viewport_layout.set_size (viewport.width, viewport.height);
    this.background_layer.set_size (viewport.width, viewport.height);
    if (this.current_effects_grid != null)
      this.current_effects_grid.set_size (viewport.width, viewport.height);
  }

  [CCode (instance_pos = -1)]
  internal void on_countdown_toggle (ToggleAction action)
  {
    conf.gconf_prop_countdown = action.active;
  }

  private void finish_countdown_callback ()
  {
    string file_name = fileutil.get_new_media_filename (this.current_mode);

    this.flash.fire ();
    this.camera.take_photo (file_name);
  }

  internal void take_photo ()
  {
    if (conf.gconf_prop_countdown)
    {
      Countdown cd = new Countdown (this.countdown_layer);
      cd.start_countdown (finish_countdown_callback);
    }
    else
    {
      finish_countdown_callback ();
    }
  }

  private int burst_count;

  private bool burst_take_photo ()
  {
    if (is_bursting && burst_count < conf.gconf_prop_burst_repeat)
    {
      this.take_photo ();
      burst_count++;
      return true;
    }
    else
    {
      is_bursting = false;
      this.enable_mode_change ();
      take_action_button.related_action.sensitive = true;
      effects_toggle_action.sensitive             = true;
      burst_count                                 = 0;
      fileutil.reset_burst ();
      return false;
    }
  }

  [CCode (instance_pos = -1)]
  internal void on_take_action (Gtk.Action action)
  {
    if (current_mode == MediaMode.PHOTO)
    {
      this.take_photo ();
    }
    else
    if (current_mode == MediaMode.VIDEO)
    {
      if (!is_recording)
      {
        camera.start_video_recording (fileutil.get_new_media_filename (this.current_mode));
        take_action_button_label.label = "<b>Stop _Recording</b>";
        take_action_button_image.set_from_stock (Gtk.STOCK_MEDIA_STOP, Gtk.IconSize.BUTTON);
        this.is_recording = true;
        this.disable_mode_change ();
        effects_toggle_action.sensitive = false;
      }
      else
      {
        camera.stop_video_recording ();
        take_action_button_label.label = "<b>" + take_action_button.related_action.label + "</b>";
        take_action_button_image.set_from_stock (Gtk.STOCK_MEDIA_RECORD, Gtk.IconSize.BUTTON);
        this.is_recording = false;
        this.enable_mode_change ();
        effects_toggle_action.sensitive = true;
      }
    }
    else
    if (current_mode == MediaMode.BURST)
    {
      is_bursting = true;
      this.disable_mode_change ();
      take_action_button.related_action.sensitive = false;
      effects_toggle_action.sensitive             = false;
      burst_take_photo ();
      GLib.Timeout.add (3500, burst_take_photo);
    }
  }

  [CCode (instance_pos = -1)]
  internal void on_effects_toggle (Gtk.ToggleAction action)
  {
    if (action.active)
    {
      setup_effects_selector ();
    }
    else
    {
      teardown_effects_selector ();
    }
    camera.toggle_effects_pipeline (action.active);
  }

  internal void on_selected_effect_change (Mx.Button button)
  {
    selected_effect = button.get_data ("effect");
    camera.set_effect (selected_effect);
    effects_toggle_action.set_active (false);
  }

  private void activate_effects_page (int number)
  {
    current_effects_page = number;
    viewport_layout.remove ((Clutter.Actor)current_effects_grid);
    current_effects_grid = effects_grids[number];
    viewport_layout.add ((Clutter.Actor)current_effects_grid);
    this.current_effects_grid.set_size (viewport.width, viewport.height);
  }

  private void teardown_effects_selector ()
  {
    video_preview.show ();
    viewport_layout.remove ((Clutter.Actor)current_effects_grid);
  }

  private void setup_effects_selector ()
  {
    video_preview.hide ();
    if (current_effects_grid == null)
    {
      effects_manager = new EffectsManager ();
      effects_manager.load_effects ();

      effects_grids = new ArrayList<Mx.Grid>();

      for (int i = 0; i < (effects_manager.effects.size / 9) + 1; i++)
      {
        Mx.Grid grid = new Mx.Grid ();
        effects_grids.add (grid);

        grid.line_alignment = Mx.Align.MIDDLE;
        grid.child_x_align  = Mx.Align.MIDDLE;
        grid.child_y_align  = Mx.Align.MIDDLE;
        grid.orientation    = Mx.Orientation.VERTICAL;

        grid.column_spacing = 20;
        grid.row_spacing    = 20;
      }

      for (int i = 0; i < effects_manager.effects.size - 1; i++)
      {
        Effect          effect  = effects_manager.effects[i];
        Clutter.Texture texture = new Clutter.Texture ();
        texture.width  = 160;
        texture.height = 120;

        Mx.Button button = new Mx.Button ();
        button.add ((Clutter.Actor)texture);
        button.set_data ("effect", effect);
        button.clicked.connect (on_selected_effect_change);

        effects_grids[i / 9].add ((Clutter.Actor)button);
        camera.connect_effect_texture (effect, texture);
      }
    }
    activate_effects_page (0);
  }

  public void setup_ui ()
  {
    gtk_builder     = new Gtk.Builder ();
    clutter_builder = new Clutter.Script ();
    fileutil        = new FileUtil ();
    flash           = new Flash (this);
    conf            = new GConf ();

    try {
      gtk_builder.add_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-actions.ui"));
      gtk_builder.add_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-about.ui"));
      gtk_builder.add_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-main-window.ui"));
      gtk_builder.connect_signals (this);

      clutter_builder.load_from_file (GLib.Path.build_filename (Config.PACKAGE_DATADIR, "cheese-viewport.json"));
    }catch (Error err)
    {
      warning ("Error: %s\n", err.message);
      return;
    }

    main_vbox                         = (Gtk.VBox)gtk_builder.get_object ("mainbox_normal");
    thumbnails                        = (Gtk.Widget)gtk_builder.get_object ("thumbnails");
    viewport_widget                   = (GtkClutter.Embed)gtk_builder.get_object ("viewport");
    viewport                          = (Clutter.Stage)viewport_widget.get_stage ();
    thumbnails_right                  = (Gtk.Frame)gtk_builder.get_object ("thumbnails_right");
    thumbnails_bottom                 = (Gtk.Frame)gtk_builder.get_object ("thumbnails_bottom");
    menubar                           = (Gtk.MenuBar)gtk_builder.get_object ("main_menubar");
    leave_fullscreen_button_container = (Gtk.HBox)gtk_builder.get_object ("leave_fullscreen_button_bin");
    photo_toggle_button               = (Gtk.ToggleButton)gtk_builder.get_object ("photo_toggle_button");
    video_toggle_button               = (Gtk.ToggleButton)gtk_builder.get_object ("video_toggle_button");
    burst_toggle_button               = (Gtk.ToggleButton)gtk_builder.get_object ("burst_toggle_button");
    take_action_button                = (Gtk.Button)gtk_builder.get_object ("take_action_button");
    take_action_button_label          = (Gtk.Label)gtk_builder.get_object ("take_action_button_internal_label");
    take_action_button_image          = (Gtk.Image)gtk_builder.get_object ("take_action_button_internal_image");
    effects_toggle_button             = (Gtk.ToggleButton)gtk_builder.get_object ("effects_toggle_button");
    leave_fullscreen_button           = (Gtk.Button)gtk_builder.get_object ("leave_fullscreen_button");
    buttons_area                      = (Gtk.HBox)gtk_builder.get_object ("buttons_area");

    take_photo_action     = (Gtk.Action)gtk_builder.get_object ("take_photo");
    take_video_action     = (Gtk.Action)gtk_builder.get_object ("take_video");
    take_burst_action     = (Gtk.Action)gtk_builder.get_object ("take_burst");
    photo_mode_action     = (Gtk.Action)gtk_builder.get_object ("photo_mode");
    video_mode_action     = (Gtk.Action)gtk_builder.get_object ("video_mode");
    burst_mode_action     = (Gtk.Action)gtk_builder.get_object ("burst_mode");
    effects_toggle_action = (Gtk.ToggleAction)gtk_builder.get_object ("effects_toggle");
    countdown_action      = (Gtk.Action)gtk_builder.get_object ("countdown");
    wide_mode_action      = (Gtk.Action)gtk_builder.get_object ("wide_mode");

    /* Array contains all 'buttons', for easier manipulation
     * IMPORTANT: IF ANOTHER BUTTON IS ADDED UNDER THE VIEWPORT, ADD IT TO THIS ARRAY */
    buttons = { photo_toggle_button,
                video_toggle_button,
                burst_toggle_button,
                take_action_button,
                effects_toggle_button,
                leave_fullscreen_button};

    video_preview           = (Clutter.Texture)clutter_builder.get_object ("video_preview");
    viewport_layout         = (Clutter.Box)clutter_builder.get_object ("viewport_layout");
    viewport_layout_manager = (Clutter.BinLayout)clutter_builder.get_object ("viewport_layout_manager");
    countdown_layer         = (Clutter.Text)clutter_builder.get_object ("countdown_layer");
    background_layer        = (Clutter.Rectangle)clutter_builder.get_object ("background");
    viewport.add_actor (background_layer);
    viewport_layout.set_layout_manager (viewport_layout_manager);

    camera = new Camera (video_preview, "/dev/video0", 1024, 768);

    viewport.add_actor (viewport_layout);

    viewport.allocation_changed.connect (on_stage_resize);

    thumb_view = new Cheese.ThumbView ();
    thumb_nav  = new Eog.ThumbNav (thumb_view, false);

    viewport.show_all ();

    try {
      camera.setup (conf.gconf_prop_camera);
    }
    catch (Error err)
    {
      warning ("Error: %s\n", err.message);
      return;
    }
    camera.play ();

    camera.toggle_effects_pipeline (false);

    set_wide_mode (conf.gconf_prop_wide_mode, true);
    set_mode (MediaMode.PHOTO);

    this.add (main_vbox);
  }
}
