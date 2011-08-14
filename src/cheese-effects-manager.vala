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
using Gee;

const string GROUP_NAME = "Effect";

internal class Cheese.EffectsManager : GLib.Object
{
  public ArrayList<Effect> effects;

  public EffectsManager ()
  {
    effects = new ArrayList<Effect>((EqualFunc) cmp_value);
  }

  public void load_effects ()
  {
    GLib.List<Cheese.Effect> effect_list = Cheese.Effect.load_effects ();
    for (int i = 0; i < effect_list.length (); i++)
      effects.add (effect_list<Cheese.Effect>.nth (i).data);

    effects.sort ((CompareFunc) sort_value);

    /* add identity effect as the first in the effect list */
    if (effects.size > 0)
    {
      Effect e = new Effect ();
      e.name          = _("No Effect");
      e.pipeline_desc = "identity";
      effects.insert (0, e);
    }
  }

  public Effect ? get_effect (string name)
  {
    foreach (Effect eff in effects)
    {
      if (eff.name == name)
        return eff;
    }
    return null;
  }

  private static bool cmp_value (Effect a, Effect b)
  {
    return a.pipeline_desc == b.pipeline_desc;
  }

  private static int sort_value (Effect a, Effect b)
  {
    if (a.name.down () < b.name.down ())
      return -1;
    else if (a.name.down () > b.name.down ())
      return 1;
    else
      return 0;
  }
}
