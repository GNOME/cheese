using GLib;
namespace Cheese
{
  [CCode (cheader_filename = "thumbview/cheese-thumb-view.h")]
  public class ThumbView : Gtk.IconView
  {
    public ThumbView ();
    public string get_selected_image ();
    public int    get_n_selected ();
    public void   remove_item (GLib.File file);
  }
}
