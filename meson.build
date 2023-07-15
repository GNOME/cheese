project(
  'cheese', ['c', 'vala'],
  version: '44.1',
  license: 'GPL2',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.58.0',
)

cheese_name = meson.project_name()

cheese_version = meson.project_version()
version_array = cheese_version.split('.')
cheese_major_version = version_array[0].to_int()

#cheese_api_version = cheese_major_version.to_string() + '.0'
cheese_api_version = '3.0'

cheese_prefix = get_option('prefix')
cheese_bindir = get_option('bindir')
cheese_datadir = get_option('datadir')
cheese_includedir = get_option('includedir')
cheese_localedir = get_option('localedir')
cheese_mandir = get_option('mandir')

cheese_namespace = 'org.gnome.Cheese'

#*******************************************************************************
# Libraries
#*******************************************************************************
# Before making a release, the CHEESE_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
current = 8
revision = 19
age = 0
libcheese_version = '@0@.@1@.@2@'.format(current - age, age, revision)

current = 26
revision = 7
age = 1
libcheese_gtk_version = '@0@.@1@.@2@'.format(current - age, age, revision)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

source_root = meson.current_source_dir()

po_dir = source_root / 'po'
vapi_dir = source_root / 'src/vapi'

top_inc = include_directories('.')

cc = meson.get_compiler('c')
valac = meson.get_compiler('vala')

config_h = configuration_data()

# defines
set_defines = [
  # package
  ['PACKAGE_LOCALEDIR', cheese_prefix / cheese_localedir],
  ['PACKAGE_NAME', 'Cheese'],
  ['PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), cheese_version)],
  ['PACKAGE_TARNAME', cheese_name],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Cheese'],
  ['PACKAGE_VERSION', cheese_version],
  # i18n
  ['GETTEXT_PACKAGE', cheese_name],
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

# compiler flags
common_flags = ['-DHAVE_CONFIG_H']

if get_option('buildtype').contains('debug')
  common_flags += cc.get_supported_arguments([
    '-Wnested-externs',
    '-Wstrict-prototypes',
    '-Werror=format=2',
    '-Werror=implicit-function-declaration',
    '-Werror=init-self',
    '-Werror=missing-include-dirs',
    '-Werror=missing-prototypes',
    '-Werror=pointer-arith',
    '-Werror=return-type',
  ])
endif

add_project_arguments(common_flags, language: 'c')

#*******************************************************************************
# Check required libraries
#*******************************************************************************
clutter_dep = dependency('clutter-1.0', version: '>= 1.13.2')
clutter_gst_dep = dependency('clutter-gst-3.0', version: '>= 3.0.0')
clutter_gtk_dep = dependency('clutter-gtk-1.0')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0')
gio_dep = dependency('gio-2.0')
glib_dep = dependency('glib-2.0', version: '>= 2.38.0')
gnome_desktop_dep = dependency('gnome-desktop-3.0')
gstreamer_dep = dependency('gstreamer-1.0')
gstreamer_pbutils_dep = dependency('gstreamer-pbutils-1.0')
gstreamer_plugins_bad_dep = dependency('gstreamer-plugins-bad-1.0', version: '>= 1.4')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.13.4')
libcanberra_dep = dependency('libcanberra')
libcanberra_gtk_lib = meson.get_compiler('c').find_library('canberra-gtk3')
libcanberra_gtk3_dep = declare_dependency(dependencies: [libcanberra_gtk_lib])

m_dep = cc.find_library('m')

cheese_common_dep = valac.find_library('cheese-common', dirs: vapi_dir)
cheese_thumbview_dep = valac.find_library('cheese-thumbview', dirs: vapi_dir)
config_dep = valac.find_library('config', dirs: vapi_dir)
eogthumbnav_dep = valac.find_library('eogthumbnav', dirs: vapi_dir)
posix_dep = valac.find_library('posix')

x11_dep = dependency('x11')
have_xtest = false
if cc.has_header('X11/extensions/XTest.h', dependencies: x11_dep)
  xtst_dep = dependency('xtst', required: false)
  have_xtest = xtst_dep.found() and cc.has_function('XTestFakeKeyEvent', dependencies: xtst_dep)
endif

dbus_session_bus_services_dir = dependency('dbus-1').get_variable(
  'session_bus_services_dir',
  pkgconfig_define: ['datadir', cheese_prefix / cheese_datadir],
)

gio_schemasdir = gio_dep.get_variable(
  'schemasdir',
  pkgconfig_define: ['datadir', cheese_prefix / cheese_datadir],
  default_value: cheese_prefix / cheese_datadir / 'glib-2.0/schemas',
)

gnome_video_effects_dep = dependency(
  'gnome-video-effects',
  required: false,
  not_found_message: 'The gnome-video-effects package is required at runtime for effects',
)

# Recommend some runtime GStreamer plugins.
gst_inspect = find_program('gst-inspect-1.0', required: false)
if gst_inspect.found()
  foreach plugin: ['camerabin', 'vp8enc', 'webmmux']
    if run_command(gst_inspect, plugin, check: false).returncode() != 0
      warning(plugin + ' was not found. It needs to be installed before Cheese is run')
    endif
  endforeach
else
  warning('unable to check for runtime GStreamer plugin dependencies')
endif

# Check for GLib testing utilities.
# FIXME: meson has its own test framework
'''
gtester = find_program('gtester', required: false)
enable_cheese_tests = gtester.found()
if not enable_cheese_tests
  warning('testing disabled as the required utilities were not found')
endif
'''

# introspection support
enable_gir = get_option('introspection')
if enable_gir
  dependency('gobject-introspection-1.0', version: '>= 0.6.7')
endif

subdir('data')
subdir('libcheese')
subdir('src')

if get_option('tests')
  subdir('tests')
endif

subdir('po')
subdir('help')

enable_gtk_doc = get_option('gtk_doc')
enable_man = get_option('man')
if enable_gtk_doc or enable_man
  subdir('docs/reference')
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)
