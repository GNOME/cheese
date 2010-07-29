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
  public static Cheese. Effect ? parse_effect_file (string filename)
  {
    KeyFile kf  = new KeyFile ();
    Effect  eff = new Effect ();
    var locale = Intl.setlocale(LocaleCategory.ALL, "");

    try
    {
      kf.load_from_file (filename, KeyFileFlags.NONE);
      eff.name          = kf.get_locale_string (GROUP_NAME, "Name", locale);
      eff.pipeline_desc = kf.get_string (GROUP_NAME, "PipelineDescription");
    }
    catch (KeyFileError err)
    {
      warning ("Error: %s\n", err.message);
      return null;
    }
    catch (FileError err)
    {
      warning ("Error: %s\n", err.message);
      return null;
    }

    message ("Found %s (%s)", eff.name, eff.pipeline_desc);
    return eff;
  }

  public ArrayList<Effect> effects;

  private ArrayList<Effect> ? load_effects_from_directory (string directory)
  {
    ArrayList<Effect> effects = new ArrayList<Effect>();
    if (FileUtils.test (directory, FileTest.EXISTS | FileTest.IS_DIR))
    {
      Dir    dir;
      string cur_file;
      try
      {
        dir = Dir.open (directory);
      }
      catch (FileError err)
      {
        warning ("Error: %s\n", err.message);
        return null;
      }


      cur_file = dir.read_name ();
      while (cur_file != null)
      {
        if (cur_file.has_suffix (".effect"))
        {
          Effect effect = EffectsManager.parse_effect_file (GLib.Path.build_filename (directory, cur_file));
          effects.add (effect);
        }
        cur_file = dir.read_name ();
      }
    }
    return effects;
  }

  public EffectsManager ()
  {
    effects = new ArrayList<Effect>();
  }

  public void load_effects ()
  {
    string system_effects;
    foreach (string dir in Environment.get_system_data_dirs ())
    {
      system_effects = GLib.Path.build_filename (dir, "gnome-video-effects");
      effects.add_all (load_effects_from_directory (system_effects));
    }

    // FIXME: it would be probably better to use ~/.local/share/
    string user_effects = GLib.Path.build_filename (Environment.get_user_data_dir (), ".gnome-video-effects");
    effects.add_all (load_effects_from_directory (user_effects));

    /* GROSS HACK: to make identity element appear first */
    foreach (Effect e in effects)
    {
      if (e.pipeline_desc == "identity")
      {
        effects.remove (e);
        effects.insert (0, e);
        break;
      }
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
}
