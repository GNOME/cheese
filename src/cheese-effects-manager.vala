using GLib;
using Gee;

const string GROUP_NAME = "effect";

internal class Cheese.EffectsManager : GLib.Object
{
  public static Cheese.Effect ? parse_effect_file (string filename)
  {
    KeyFile kf  = new KeyFile ();
    Effect  eff = new Effect ();

    try
    {
      kf.load_from_file (filename, KeyFileFlags.NONE);
      eff.name          = kf.get_string (GROUP_NAME, "name");
      eff.pipeline_desc = kf.get_string (GROUP_NAME, "pipeline_desc");
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
        Effect effect = EffectsManager.parse_effect_file (GLib.Path.build_filename (directory, cur_file));
        effects.add (effect);
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
    string system_effects = GLib.Path.build_filename (Config.PACKAGE_DATADIR, "effects");

    effects.add_all (load_effects_from_directory (system_effects));

    string user_effects = GLib.Path.build_filename (Environment.get_user_data_dir (), ".cheese", "effects");
    effects.add_all (load_effects_from_directory (user_effects));
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
