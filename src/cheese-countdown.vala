using GLib;
using Clutter;

const int COUNTDOWN_START = 3;

internal class Cheese.Countdown : GLib.Object
{
  public delegate void CountdownCallback ();

  private Clutter.Text countdown_actor;

  private static CountdownCallback completed_callback;

  private static int current_value = 0;

  public Countdown (Clutter.Text countdown_actor)
  {
    this.countdown_actor = countdown_actor;
  }

  private void fade_out ()
  {
    Clutter.Animation anim = this.countdown_actor.animate (Clutter.AnimationMode.LINEAR, 500, "opacity", 0);
    Signal.connect_after (anim, "completed", (GLib.Callback)fade_in, this);
  }

  private void fade_in ()
  {
    if (this.current_value <= 0)
    {
      this.completed_callback ();
      return;
    }
    this.countdown_actor.text = this.current_value.to_string ();
    this.current_value--;

    Clutter.Animation anim = this.countdown_actor.animate (Clutter.AnimationMode.LINEAR, 500, "opacity", 255);
    Signal.connect_after (anim, "completed", (GLib.Callback)fade_out, this);
  }

  public void start_countdown (CountdownCallback completed_callback)
  {
    this.completed_callback = completed_callback;
    this.current_value      = COUNTDOWN_START;
    fade_in ();
  }
}
