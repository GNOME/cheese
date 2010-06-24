using GLib;

const string GROUP_NAME = "effect";

internal class Cheese.EffectsManager : GLib.Object
{
	public static Cheese.Effect parse_effect_file(string filename)
	{
		KeyFile kf = new KeyFile();
		kf.load_from_file (filename, KeyFileFlags.NONE);
		Effect eff = new Effect();
		eff.name = kf.get_string (GROUP_NAME, "name");
		eff.pipeline_desc = kf.get_string (GROUP_NAME, "pipeline_desc");
		critical("%s", eff.pipeline_desc);
		return eff;
	}
}