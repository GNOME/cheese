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

internal class Cheese.EffectsManager : GLib.Object
{
    public List<Effect> effects;

    public EffectsManager ()
    {
        effects = new List<Effect> ();
    }

    /**
     * Add the effects into the manager.
     */
    public void load_effects ()
    {
        var loaded_effects = Cheese.Effect.load_effects ();
        var effects_hash = new HashTable<string, Effect> (str_hash, str_equal);

        foreach (var effect in loaded_effects)
        {
            effects_hash.insert (effect.name, effect);
        }

        effects_hash.foreach (add_effect);

        /* Add identity effect as the first in the effect list. */
        if (effects.length () > 0)
        {
            Effect e = new Effect (_("No Effect"), "identity");
            effects.prepend (e);
        }
    }

    /**
     * Add an effect into the manager. Used as a HFunc.
     */
    private void add_effect (string name, Effect effect)
    {
        effects.insert_sorted (effect, sort_value);
    }

    /**
     * Search for and return the requested effect.
     *
     * @param name the name of the effect to search for
     * @return the effect which matches the supplied name, or null
     */
    public Effect ? get_effect (string name)
    {
        foreach (var effect in effects)
        {
            if (effect.name == name)
                return effect;
        }
        return null;
    }

  /**
   * Compare two effects by the pipeline description.
   *
   * @param a an effect to compare against
   * @param b another effect to compare against
   * @return true if the effects are the same, false otherwise
   */
  private static bool cmp_value (Effect a, Effect b)
  {
    return a.pipeline_desc == b.pipeline_desc;
  }

  /**
   * A sort function for effects
   *
   * @param a an effect to sort against
   * @param b another effect to sort against
   * @return -1 if a is less than b, 0 if the effects are the same and 1 if a
   * is greater than b
   */
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
