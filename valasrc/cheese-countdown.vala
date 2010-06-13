using GLib;
using Clutter;

const int COUNTDOWN_TILL = 3;

internal class Cheese.Countdown : GLib.Object{

	public delegate void CountdownCallback();
	
	private Clutter.Text countdown_actor;
	private CountdownCallback completed_callback;

	private int current_value = 0;
	
	public Countdown(Clutter.Text countdown_actor) {
		this.countdown_actor = countdown_actor;
	}

	// HACK: completed signal of animation never seems to be fired.
	// Faking it with a one shot timer. Ugh.
	private void fade_out() {
		Clutter.Animation anim = this.countdown_actor.animate(Clutter.AnimationMode.LINEAR, 500,
															  "opacity", 0);
//		anim.completed.connect_after( () => {fade_in();});
		GLib.Timeout.add(500,
						 () => { fade_in();
								 return false;
						 });						 
	}
	
	private void fade_in() {
		this.current_value++;
		if (this.current_value > 3) {
			this.completed_callback();
			return;
		}
		this.countdown_actor.text = this.current_value.to_string();

			
		Clutter.Animation anim = this.countdown_actor.animate(Clutter.AnimationMode.LINEAR, 500,
															  "opacity", 255);
//		anim.completed.connect_after(() => {fade_out();});
		GLib.Timeout.add(500,
						 () => { fade_out();
								 return false;
						 });
	}
	
	public void start_countdown (CountdownCallback completed_callback) {
		this.completed_callback = completed_callback;
		fade_in();
	}
}