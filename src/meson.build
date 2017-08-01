cheese_sources = [
  'cheese-application.vala',
  'cheese-countdown.vala',
  'cheese-effects-manager.vala',
  'cheese-preferences.vala',
  'cheese-main.vala',
  'cheese-window.vala',
  cheese_resources
]

cheese_vala = meson.get_compiler('vala').find_library('cheese-common', dirs: join_paths(meson.current_source_dir(), 'vapi'))
cheese_thumbview = meson.get_compiler('vala').find_library('cheese-thumbview', dirs: join_paths(meson.current_source_dir(), 'vapi'))
config_vala = meson.get_compiler('vala').find_library('config', dirs: join_paths(meson.current_source_dir(), 'vapi'))
eogthumbnav = meson.get_compiler('vala').find_library('eogthumbnav', dirs: join_paths(meson.current_source_dir(), 'vapi'))
gstreamer_vala = dependency('gstreamer-1.0')

cheese_dependencies = [
  cheese_vala,
  cheese_thumbview,
  clutter,
  cluttergst,
  cluttergtk,
  config_vala,
  eogthumbnav,
  gdk_pixbuf,
  glib,
  gstreamer_vala,
  gtk,
  libcanberra,
  libcanberra_gtk,
  x11
]

config_inc = include_directories('..')
libcheese_inc = include_directories('../libcheese')

thumbnav = static_library(
  'thumbnav',
  sources : [ 'thumbview/eog-thumb-nav.c', 'thumbview/cheese-thumb-view.c', 'thumbview/cheese-thumbnail.c' ],
  dependencies : [ gdk_pixbuf, gnome_desktop, gtk ],
  include_directories : libcheese_inc
)

executable(
  'cheese',
  sources : [ cheese_sources ],
  dependencies : cheese_dependencies,
  link_with : [ libcheese, libcheese_gtk, thumbnav ],
  include_directories : [ config_inc, libcheese_inc ],
  install : true)
