#! /usr/bin/env python
# encoding: utf-8
#
# Copyright (C) 2007,2008 daniel g. siegel <dgsiegel@gmail.com>
# Copyright (C) 2007,2008 Jaap A. Haitsma <jaap@haitsma.org>
#
# Licensed under the GNU General Public License Version 2
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

import os, sys
# waf imports
import Common, Params, gnome, intltool, misc

# the following two variables are used by the target "waf dist"
VERSION='2.26.3'
APPNAME='cheese'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = '_build_'

def set_options(opt):
	opt.add_option('--maintainer', action="store_true", default=False, help="enable maintainer mode", dest="maintainer")
	opt.add_option('--cheese-advices', action="store_true", default=False, help="enable mobile application usage advice handler", dest="advices")
	pass

def configure(conf):
	if Params.g_options.advices:
		check_cheese_build_consistency()

	conf.check_tool('gcc gnome intltool misc')

	conf.check_pkg('glib-2.0', destvar='GLIB', vnum='2.16.0', mandatory=True)
	conf.check_pkg('gobject-2.0', destvar='GOBJECT', vnum='2.12.0', mandatory=True)
	conf.check_pkg('gio-2.0', destvar='GIO', vnum='2.16.0', mandatory=True)
	conf.check_pkg('gtk+-2.0', destvar='GTK', vnum='2.14.0', mandatory=True)
	conf.check_pkg('gdk-2.0', destvar='GDK', vnum='2.14.0', mandatory=True)
	conf.check_pkg('gnome-desktop-2.0', destvar='LIBGNOMEDESKTOP', vnum='2.25.1', mandatory=True)
	conf.check_pkg('gconf-2.0', destvar='GCONF', vnum='2.16.0', mandatory=True)
	conf.check_pkg('gstreamer-0.10', destvar='GSTREAMER', vnum='0.10.20', mandatory=True)
	conf.check_pkg('gstreamer-plugins-base-0.10', destvar='GSTREAMER_PLUGINS_BASE', vnum='0.10.20', mandatory=True)
	conf.check_pkg('libebook-1.2', destvar='LIBEBOOK', vnum='1.12.0', mandatory=True)
	conf.check_pkg('cairo', destvar='CAIRO', vnum='1.4.0', mandatory=True)
	conf.check_pkg('dbus-1', destvar='DBUS', vnum='1.0', mandatory=True)
	conf.check_pkg('dbus-glib-1', destvar='DBUSGLIB', vnum='0.7', mandatory=True)
	conf.check_pkg('hal', destvar='HAL', vnum='0.5.9', mandatory=True)
	conf.check_pkg('pangocairo', destvar='PANGOCAIRO', vnum='1.18.0', mandatory=True)
	conf.check_pkg('librsvg-2.0', destvar='LIBRSVG', vnum='2.18.0', mandatory=True)
	conf.env['LIB_GSTREAMER'] += ['gstinterfaces-0.10']

	conf.define('VERSION', VERSION)
	conf.define('GETTEXT_PACKAGE', 'cheese')
	conf.define('PACKAGE', 'cheese')

	if Params.g_options.maintainer:
		import os
		conf.define('PACKAGE_DATADIR', os.getcwd() + '/data')
		conf.env['CCFLAGS']='-Wall -Werror -g'
		print ""
		print "*******************************************************************"
		print "**                 MAINTAINER MODE ENABLED.                      **"
		print "**       CHEESE CAN BE RUN BY EXECUTING CHEESE IN _build_        **"
		print "**               BUT DO NOT TRY TO INSTALL IT                    **"
		print "*******************************************************************"
		print ""
	else:
		conf.define('PACKAGE_DATADIR', conf.env['DATADIR'] + '/cheese')

	conf.define('APPNAME_DATA_DIR', conf.env['DATADIR'] + '/cheese')
	conf.define('PACKAGE_DOCDIR', conf.env['DATADIR'] + '/share/doc/cheese')
	conf.define('PACKAGE_LOCALEDIR', conf.env['DATADIR'] + '/locale')
	conf.define('PACKAGE_LIBEXECDIR', conf.env['PREFIX'] + '/libexec/cheese')
	conf.define('BINDIR', conf.env['PREFIX'] + '/bin')
	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')
	conf.env.append_value('CCFLAGS', '-DGNOME_DESKTOP_USE_UNSTABLE_API=1')

	conf.write_config_header('cheese-config.h')

def build(bld):
	# process subfolders from here
	bld.add_subdirs('data help po src')

def shutdown():
	gnome.postinstall(APPNAME, schemas=True, icons=True, scrollkeeper=False)

def dist():
	# set the compression type to gzip (default is bz2)
	import Scripting
	Scripting.g_gz = 'gz'

	# after making the package, print the md5sum
	import md5
	from Scripting import DistTarball
	(f, filename) = DistTarball(APPNAME, VERSION)
	f = file(filename,'rb')
	m = md5.md5()
	readBytes = 1024 # read 1024 bytes per time
	while (readBytes):
		readString = f.read(readBytes)
		m.update(readString)
		readBytes = len(readString)
	f.close()
	print filename, m.hexdigest()
	sys.exit(0)

def check_cheese_build_consistency():
  # check the hash of the build and src-files with the available files
	Params.pprint('YELLOW', """
	ClRoZSBDaGVlc2UgdGVhbSBhZHZpY2VzIHlvdSB0byBoYXZlIGF0IGxlYXN0IHRoZSBmb2xsb3dp
	bmcKdmFyaWV0eSBvZiBjaGVlc2VzIGluIHlvdXIgZnJpZGdlLCBvbmNlIHlvdXIgZnJpZW5kcyB3
	aWxsCnN1cnByaXNpbmdseSB2aXNpdCB5b3U6CgpIYXJ6ZXIKYSBHZXJtYW4gc291ciBtaWxrIGNo
	ZWVzZSBtYWRlIGZyb20gbG93IGZhdCBjdXJkIGNoZWVzZSwKd2hpY2ggY29udGFpbnMgb25seSBh
	Ym91dCBvbmUgcGVyY2VudCBmYXQgYW5kIG9yaWdpbmF0ZXMgaW4KdGhlIEhhcnogbW91bnRhaW4g
	cmVnaW9uIHNvdXRoIG9mIEJyYXVuc2Nod2VpZy4gQW5kcmUgYWxzbwpyZWNvbW1lbmRzIHRoaXMs
	IHNvIGl0IGNhbid0IGJlIHRoYXQgYmFkIQoKQXNpYWdvCmFuIEl0YWxpYW4gY2hlZXNlIHRoYXQg
	YWNjb3JkaW5nIHRvIHRoZSBkaWZmZXJlbnQgYWdpbmcgY2FuCmFzc3VtZSBkaWZmZXJlbnQgdGV4
	dHVyZXMsIGZyb20gc21vb3RoIGZvciB0aGUgZnJlc2ggQXNpYWdvCmNoZWVzZSB0byBhIGNydW1i
	bHkgdGV4dHVyZSBmb3IgdGhlIGFnZWQgY2hlZXNlIHdob3NlCmZsYXZvciBpcyByZW1pbmlzY2Vu
	dCBvZiBzaGFycCBDaGVkZGFyIGFuZCBQYXJtZXNhbi4gVGhpcwppcyBEYW5pZWwncyBmYXZvdXJp
	dGUuCgpCZWwgUGFlc2UKYSBzZW1pLXNvZnQgSXRhbGlhbiBjaGVlc2UKCkxpcHRhdWVyCmEgU2xv
	dmFrIGRpc2ggb2Ygc3BpY2VkLCB3aGl0ZSBjaGVlc2UgbWFkZSBmcm9tIGEgbWl4dHVyZQpvZiBz
	aGVlcCdzIGFuZCBjb3cncyBtaWxrCgpNb3p6YXJlbGxhCmEgZ2VuZXJpYyB0ZXJtIGZvciB0aGUg
	c2V2ZXJhbCBraW5kcyBvZiwgb3JpZ2luYWxseSwKSXRhbGlhbiBmcmVzaCBjaGVlc2VzIHRoYXQg
	YXJlIG1hZGUgdXNpbmcgc3Bpbm5pbmcgYW5kIHRoZW4KY3V0dGluZy4gVXN1YWxseSB0aGlzIHR5
	cGUgb2YgY2hlZXNlIGlzIHVzZWQgZm9yIHBpenphcwoKUGlhdmUKYSBjb3cncyBtaWxrIGNoZWVz
	ZSBtYWRlIG9ubHkgaW4gdGhlIFBpYXZlIFJpdmVyIFZhbGxleQpyZWdpb24gb2YgQmVsbHVubywg
	SXRhbHkKCkVtbWVudGhhbGVyCmEgeWVsbG93LCBtZWRpdW0taGFyZCBjaGVlc2UsIHdpdGggY2hh
	cmFjdGVyaXN0aWMgbGFyZ2UKaG9sZXMgZnJvbSBTd2l0emVybGFuZAoKS2Flc2VmdXNzCmFuIGF3
	ZnVsIHNtZWxsaW5nIGNoZWVzZSwgd2hpY2ggeW91IGNhbiBmaW5kIGluIG1hbnkgR2VybWFuCnJl
	Z2lvbnMsIGVzcGVjaWFsbHkgaW4gbG9ja2VyIHJvb21zCgpBcHBlbnplbGxlcgphIGhhcmQgY293
	J3MtbWlsayBjaGVlc2UgcHJvZHVjZWQgaW4gdGhlIEFwcGVuemVsbCByZWdpb24Kb2Ygbm9ydGhl
	YXN0IFN3aXR6ZXJsYW5kCgpHb3Jnb256b2xhCmEgYmx1ZSB2ZWluZWQgSXRhbGlhbiBibHVlIGNo
	ZWVzZSwgbWFkZSBmcm9tIHVuc2tpbW1lZApjb3cncyBtaWxrLiBJdCBjYW4gYmUgYnV0dGVyeSBv
	ciBmaXJtLCBjcnVtYmx5IGFuZCBxdWl0ZQpzYWx0eSwgd2l0aCBhICdiaXRlJyBmcm9tIGl0cyBi
	bHVlIHZlaW5pbmcKCkZldGEKYSBjdXJkIGNoZWVzZSBpbiBicmluZS4gSXQgaXMgdHJhZGl0aW9u
	YWxseSBtYWRlIGZyb20KZ29hdCdzIGFuZC9vciBzaGVlcCdzIG1pbGsgYWx0aG91Z2ggY293J3Mg
	bWlsayBtYXkgYmUKc3Vic3RpdHV0ZWQKCQ==
	""".decode('base64')) or sys.exit(0)

