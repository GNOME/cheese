using Gtk;

public class Cheese.CheeseWindow : Window {

	enum Mode {
		PHOTO,
		VIDEO,
		BURST
	} 

	bool is_recording = false;

	Mode mode;

	/* if you come up with a better name ping me */
	Widget thewidget;
	Widget video_area;
	
	Widget fullscreen_popup;
	
	Widget widget_alignment;
	Widget notebook_bar;
	Widget fullscreen_bar;
	
	VBox main_vbox;
	VBox video_vbox;
	
	Widget netbook_alignment;
	Widget toolbar_alignment;
	Widget effect_button_alignment;
	Widget info_bar_alignment;
	Widget togglegroup_alignment;
	
	Widget effect_frame;
	Widget effect_vbox;
	Widget effect_alignment;
	Widget effect_chooser;
	Widget throbber_align;
	Widget throbber_box;
	Widget throbber;
	Widget info_bar;
	Widget countdown_frame;
	Widget countdown_frame_fullscreen;
	Widget countdown;
	Widget countdown_fullscreen;
	
	Button button_effects;
	Button button_photo;
	Button button_video;
	Button button_burst;
	Button button_effects_fullscreen;
	Button button_photo_fullscreen;
	Button button_video_fullscreen;
	Button button_burst_fullscreen;
	Button button_exit_fullscreen;
	
	Widget image_take_photo;
	Widget image_take_photo_fullscreen;
	Label label_effects;
	Label label_photo;
	Label label_photo_fullscreen;
	Label label_take_photo;
	Label label_take_photo_fullscreen;
	Label label_video;
	Label label_video_fullscreen;
	
	Widget thumb_scrollwindow;
	Widget thumb_nav;
	Widget thumb_view;
	Widget thumb_view_popup_menu;
	
	Button take_picture;
	Button take_picture_fullscreen;
	
	ActionGroup actions_main;
	ActionGroup actions_prefs;
	ActionGroup actions_countdown;
	ActionGroup actions_flash;
	ActionGroup actions_effects;
	ActionGroup actions_file;
	ActionGroup actions_photo;
	ActionGroup actions_toggle;
	ActionGroup actions_video;
	ActionGroup actions_burst;
	ActionGroup actions_fullscreen;
	ActionGroup actions_wide_mode;
	ActionGroup actions_trash;
	
  	UIManager ui_manager;
	private const ActionEntry[] action_entries_main = {
		{"Cheese",       null,            N_("_Cheese")                       },
		{"Edit",         null,            N_("_Edit")                         },
		{"Help",         null,            N_("_Help")                         },
		
		{"Quit",         STOCK_QUIT,  null, null, null,  on_quit},
		{"HelpContents", STOCK_HELP,  N_("_Contents"), "F1", N_("Help on this Application"),
		 on_help_contents},
		{"About",        STOCK_ABOUT, null, null, null,  on_about}
	};
	
	const ActionEntry[] action_entries_prefs = {
		{"Preferences",  STOCK_PREFERENCES, N_("Preferences"), null, null,  (on_preferences_window)}
	};

	const RadioActionEntry[] action_entries_toggle = {
		{"Photo", null, N_("_Photo"), null, null, 0},
		{"Video", null, N_("_Video"), null, null, 1},
		{"Burst", null, N_("_Burst"), null, null, 2}
	};
	
	const ToggleActionEntry[] action_entries_countdown = {
		{"Countdown", null, N_("Countdown"), null, null,  (on_toggle_countdown), false}
	};
	const ToggleActionEntry[] action_entries_flash = {
		{"Flash", null, N_("Flash"), null, null,  (on_toggle_flash), false}
	};
	
	const ToggleActionEntry[] action_entries_effects = {
		{"Effects", null, N_("_Effects"), null, null,  (on_effect_button_press), false}
	};
	
	const ToggleActionEntry[] action_entries_fullscreen = {
		{"Fullscreen", STOCK_FULLSCREEN, null, "F11", null,  (on_toggle_fullscreen), false}
	};
	const ToggleActionEntry[] action_entries_wide_mode = {
		{"WideMode", null, N_("_Wide mode"), null, null,  (on_toggle_wide_mode), false}
	};
	
	const ActionEntry[] action_entries_photo = {
		{"TakePhoto", null, N_("_Take a Photo"), "space", null,  (on_action_button_clicked)}
	};
	const ToggleActionEntry[] action_entries_video = {
		{"TakeVideo", null, N_("_Recording"), "space", null,  (on_action_button_clicked), false}
	};
	const ActionEntry[] action_entries_burst = {
		{"TakeBurst", null, N_("_Take multiple Photos"), "space", null,  (on_action_button_clicked)}
	};

	const ActionEntry[] action_entries_file = {
		{"Open",        STOCK_OPEN,    N_("_Open"),          "<control>O",    null,
		 (on_file_open)},
		{"SaveAs",      STOCK_SAVE_AS, N_("Save _As…"),      "<control>S",    null,
		 (on_file_save_as)},
		{"MoveToTrash", "user-trash",      N_("Move to _Trash"), "Delete",        null,
		 (on_file_move_to_trash)},
		{"Delete",      null,              N_("Delete"),         "<shift>Delete", null,
		 (on_file_delete)}
	};
	
	const ActionEntry[] action_entries_trash = {
		{"RemoveAll",    null,             N_("Move All to Trash"), null, null,
		 (on_file_move_all_to_trash)}
};
	
	
	
	ActionGroup add_radio_action_group (string name, RadioActionEntry[] entries) {
		ActionGroup actions = new ActionGroup(name);
		actions.set_translation_domain (Config.GETTEXT_PACKAGE);
		actions.add_radio_actions (entries, 0, on_radio_change);
		return actions;
	}
	
	ActionGroup add_toggle_action_group (string name, ToggleActionEntry[] entries) {
	ActionGroup actions = new ActionGroup(name);
	actions.set_translation_domain (Config.GETTEXT_PACKAGE);
	actions.add_toggle_actions (entries, this);
	return actions;
	}
	
	ActionGroup add_action_group(string name, ActionEntry[] entries) {
		ActionGroup actions = new ActionGroup(name);
		actions.set_translation_domain (Config.GETTEXT_PACKAGE);
		actions.add_actions (entries, this);
		return actions;
	}
	
	void on_radio_change (Action action) {
	}
	
	void on_quit (Action action) {
	}
	
	void on_help_contents (Action action) {
	}
	
	void on_about (Action action) {
	}
	
	void on_preferences_window (Action action) {
	}
	
	void on_toggle_countdown (Action action) {
	}
	
	void on_toggle_flash (Action action) {
	}
	
	void on_effect_button_press (Action action) {
	}
	
	void on_toggle_fullscreen (Action action) {
	}
	
	void on_toggle_wide_mode (Action action) {
	}
	
	void on_action_button_clicked (Action action) {
	}
	
	void on_file_open (Action action) {
	}
	
	void on_file_save_as (Action action) {
	}
	
	void on_file_move_to_trash (Action action) {
	}
	
	void on_file_delete (Action action) {
	}
	
	void on_file_move_all_to_trash (Action action) {
	}
	
	
	
	
	public	void setup_ui () {
		Builder builder = new Builder();
		
		builder.add_from_file (Path.build_filename ("../data/", "cheese.ui"));
		
		button_effects              = (Button) builder.get_object ("button_effects");
		button_photo                = (Button) builder.get_object ("button_photo");
		button_video                = (Button) builder.get_object ("button_video");
		button_burst                = (Button) builder.get_object ("button_burst");
		image_take_photo            = (Image) builder.get_object ("image_take_photo");
		label_effects               = (Label) builder.get_object ("label_effects");
		label_photo                 = (Label) builder.get_object ("label_photo");
		label_take_photo            = (Label) builder.get_object ("label_take_photo");
		label_video                 = (Label) builder.get_object ("label_video");
		main_vbox                   = (VBox) builder.get_object ("main_vbox");
		netbook_alignment           = (Alignment) builder.get_object ("netbook_alignment");
		togglegroup_alignment       = (Alignment) builder.get_object ("togglegroup_alignment");
		effect_button_alignment     = (Alignment) builder.get_object ("effect_button_alignment");
		info_bar_alignment          = (Alignment) builder.get_object ("info_bar_alignment");
		toolbar_alignment           = (Alignment) builder.get_object ("toolbar_alignment");
		video_vbox                  = (VBox) builder.get_object ("video_vbox");
		widget_alignment            = (Alignment) builder.get_object ("widget_alignment");
//			notebook_bar                = builder.get_object ("notebook_bar");
		take_picture                = (Button) builder.get_object ("take_picture");
//			thumb_scrollwindow          = builder.get_object ("thumb_scrollwindow");
//			countdown_frame             = builder.get_object ("countdown_frame");
//			effect_frame                = builder.get_object ("effect_frame");
//			effect_vbox                = builder.get_object ("effect_vbox");
//			effect_alignment            = builder.get_object ("effect_alignment");
//			fullscreen_popup            = builder.get_object ("fullscreen_popup");
//			fullscreen_bar              = builder.get_object ("fullscreen_notebook_bar");
		button_effects_fullscreen   = (Button) builder.get_object ("button_effects_fullscreen");
		button_photo_fullscreen     = (Button) builder.get_object ("button_photo_fullscreen");
		button_video_fullscreen     = (Button) builder.get_object ("button_video_fullscreen");
		button_burst_fullscreen     = (Button) builder.get_object ("button_burst_fullscreen");
		take_picture_fullscreen     = (Button) builder.get_object ("take_picture_fullscreen");
		label_take_photo_fullscreen = (Label) builder.get_object ("label_take_photo_fullscreen");
		image_take_photo_fullscreen = (Image) builder.get_object ("image_take_photo_fullscreen");
		label_photo_fullscreen      = (Label) builder.get_object ("label_photo_fullscreen");
		label_video_fullscreen      = (Label) builder.get_object ("label_video_fullscreen");
//			countdown_frame_fullscreen  = builder.get_object ("countdown_frame_fullscreen");
		button_exit_fullscreen      = (Button) builder.get_object ("button_exit_fullscreen");

		add(main_vbox);
		
		actions_main = add_action_group ("ActionsMain", action_entries_main);
		actions_prefs = add_action_group ("ActionsPrefs", action_entries_prefs);
		actions_countdown = add_toggle_action_group ("ActionsCountdown", action_entries_countdown);
		actions_flash = add_toggle_action_group ("ActionsFlash", action_entries_flash);
		actions_effects = add_toggle_action_group ("ActionsEffects", action_entries_effects);
		actions_file = add_action_group ("ActionsFile", action_entries_file);
		actions_photo = add_action_group ("ActionsPhoto", action_entries_photo);
		actions_toggle = add_radio_action_group ("ActionsToggle", action_entries_toggle);
		actions_video = add_toggle_action_group ("ActionsVideo", action_entries_video);
		actions_burst = add_action_group ("ActionsBurst", action_entries_burst);
		actions_fullscreen = add_toggle_action_group ("ActionsFullscreen", action_entries_fullscreen);
		actions_wide_mode = add_toggle_action_group ("ActionsWideMode", action_entries_wide_mode);
		actions_trash = add_action_group ("ActionsTrash", action_entries_trash);

		ui_manager = new UIManager();
		ui_manager.add_ui_from_file (Path.build_filename("../data", "cheese-ui.xml"));

	
	}
}
