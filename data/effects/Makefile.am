%.effect: %.effect.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

effectsdir = $(datadir)/cheese/effects
effects_in_files = \
	agingtv.effect.in \
	dicetv.effect.in \
	edgetv.effect.in \
	hulk.effect.in \
	identity.effect.in \
	mauve.effect.in \
	noir.effect.in \
	optv.effect.in \
	quarktv.effect.in \
	radioactv.effect.in \
	saturation.effect.in \
	shagadelictv.effect.in \
	streaktv.effect.in \
	vertigotv.effect.in \
	warptv.effect.in

effects_DATA = $(effects_in_files:.effect.in=.effect)

EXTRA_DIST = \
	$(effects_in_files)

DISTCLEANFILES = \
	$(effects_DATA)
