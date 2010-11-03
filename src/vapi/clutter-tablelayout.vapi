using Clutter;

[CCode (cprefix = "Clutter", lower_case_cprefix = "clutter_", gir_namespace = "Clutter", gir_version = "1.0")]
namespace Clutter {

	
	[CCode (cprefix = "CLUTTER_TABLE_ALIGNMENT_", cheader_filename = "clutter/clutter.h")]
	public enum TableAlignment {
		START,
		CENTER,
		END
	}
	
	[CCode (cheader_filename = "clutter/clutter.h")]
	public class TableLayout : Clutter.LayoutManager {
		[CCode (type = "ClutterLayoutManager*", has_construct_function = false)]
		public TableLayout ();
		public void get_alignment (Clutter.Actor actor, Clutter.TableAlignment x_align, Clutter.TableAlignment y_align);
		public int get_column_count ();
		public uint get_column_spacing ();
		public uint get_easing_duration ();
		public ulong get_easing_mode ();
		public void get_expand (Clutter.Actor actor, bool x_expand, bool y_expand);
		public void get_fill (Clutter.Actor actor, bool x_fill, bool y_fill);
		public int get_row_count ();
		public uint get_row_spacing ();
		public void get_span (Clutter.Actor actor, int column_span, int row_span);
		public bool get_use_animations ();
		public void pack (Clutter.Actor actor, int column, int row);
		public void set_alignment (Clutter.Actor actor, Clutter.TableAlignment x_align, Clutter.TableAlignment y_align);
		public void set_column_spacing (uint spacing);
		public void set_easing_duration (uint msecs);
		public void set_easing_mode (ulong mode);
		public void set_expand (Clutter.Actor actor, bool x_expand, bool y_expand);
		public void set_fill (Clutter.Actor actor, bool x_fill, bool y_fill);
		public void set_row_spacing (uint spacing);
		public void set_span (Clutter.Actor actor, int column_span, int row_span);
		public void set_use_animations (bool animate);
		public uint column_spacing { get; set; }
		public uint easing_duration { get; set; }
		public ulong easing_mode { get; set; }
		public uint row_spacing { get; set; }
		public bool use_animations { get; set; }
	}
}