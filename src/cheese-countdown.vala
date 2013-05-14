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

internal class Cheese.Countdown : GLib.Object
{
  public delegate void CountdownCallback ();
  private Clutter.Text countdown_actor;
  private unowned CountdownCallback completed_callback;
  private int current_value = 0;
  private GLib.Settings settings;
  public bool running;

  public Countdown (Clutter.Text countdown_actor)
  {
    this.countdown_actor = countdown_actor;
    settings             = new GLib.Settings("org.gnome.Cheese");
  }

  ~Countdown ()
  {
    stop ();
  }

  /**
   * Fade the countdown text out, over 500 milliseconds.
   *
   * Once the fade-out is complete, this method calls fade_in().
   */
  private void fade_out ()
  {
    var pulse_out = new Clutter.PropertyTransition ("opacity");
    pulse_out.set_duration (500);
    pulse_out.set_from_value (255);
    pulse_out.set_to_value (0);
    pulse_out.remove_on_complete = true;
    pulse_out.completed.connect (fade_in);

    this.countdown_actor.add_transition ("pulse-out", pulse_out);
  }

  /**
   * Decrement the countdown text and fade it in, over 500 milliseconds.
   *
   * Once the fade-in is complete, this method calls fade_out().
   */
  private void fade_in ()
  {
    if (this.current_value <= 0)
    {
      this.completed_callback ();
      running = false;
      return;
    }
    this.countdown_actor.text = this.current_value.to_string ();
    this.current_value--;

    var pulse_in = new Clutter.PropertyTransition ("opacity");
    pulse_in.set_duration (500);
    pulse_in.set_from_value (0);
    pulse_in.set_to_value (255);
    pulse_in.remove_on_complete = true;
    pulse_in.completed.connect (fade_out);

    this.countdown_actor.add_transition ("pulse-in", pulse_in);
  }

  /**
   * Start the countdown, using the countdown-duration GSetting for the time.
   *
   * @param completed_callback the callback to call upon countdown completion
   */
  public void start (CountdownCallback completed_callback)
  {
    this.completed_callback = completed_callback;
    this.current_value = settings.get_int("countdown-duration");
    running = true;
    countdown_actor.show ();
    fade_in ();
  }

  /**
   * Stop the countdown, for example if it was interrupted by the user.
   */
  public void stop ()
  {
    countdown_actor.hide ();
    countdown_actor.remove_all_transitions ();
    running = false;
  }
}
