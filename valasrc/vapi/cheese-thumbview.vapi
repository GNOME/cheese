using GLib;
namespace Cheese
{
  [CCode (cheader_filename = "thumbview/cheese-thumb-view.h")]
  public class ThumbView : Gtk.IconView
  {
    public ThumbView ();
    string get_selected_image ();
    int    get_n_selected ();
    void   remove_item (GLib.File file);
  }
}
