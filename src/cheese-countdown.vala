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

using GLib;
using Clutter;

const int COUNTDOWN_START = 3;

internal class Cheese.Countdown : GLib.Object
{
  public delegate void CountdownCallback ();

  private Clutter.Text countdown_actor;

  private static CountdownCallback completed_callback;

  private static int current_value = 0;

  private static ulong signal_id;

  private static Clutter.Animation anim;

  public bool running;

  public Countdown (Clutter.Text countdown_actor)
  {
    this.countdown_actor = countdown_actor;
  }

  private void fade_out ()
  {
    anim      = this.countdown_actor.animate (Clutter.AnimationMode.LINEAR, 500, "opacity", 0);
    signal_id = Signal.connect_after (anim, "completed", (GLib.Callback)fade_in, this);
  }

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

    anim      = this.countdown_actor.animate (Clutter.AnimationMode.LINEAR, 500, "opacity", 255);
    signal_id = Signal.connect_after (anim, "completed", (GLib.Callback)fade_out, this);
  }

  public void start (CountdownCallback completed_callback)
  {
    this.completed_callback = completed_callback;
    this.current_value      = COUNTDOWN_START;
    running = true;
    countdown_actor.show ();
    fade_in ();
  }

  public void stop ()
  {
    countdown_actor.hide ();
    SignalHandler.disconnect (anim, signal_id);
    running = false;
  }
}
