libcheese_inc = include_directories('.')

enum_headers = files('cheese-widget.h')

gir_headers = files(
  'cheese-camera-device.h',
  'cheese-camera-device-monitor.h',
  'cheese-camera.h',
  'cheese-effect.h',
)

private_gir_headers = files('cheese-fileutil.h')

headers = enum_headers + gir_headers + files(
  'cheese-avatar-chooser.h',
  'cheese-avatar-widget.h',
  'cheese-gtk.h',
  'cheese.h',
)

install_headers(
  headers,
  subdir: cheese_name,
)

sources = files(
  'cheese.c',
  'cheese-camera.c',
  'cheese-camera-device.c',
  'cheese-camera-device-monitor.c',
  'cheese-effect.c',
  'cheese-fileutil.c',
)

deps = [
  clutter_dep,
  gdk_pixbuf_dep,
  gio_dep,
  glib_dep,
  gstreamer_dep,
]

private_deps = [
  clutter_gst_dep,
  gstreamer_pbutils_dep,
  gstreamer_plugins_bad_dep,
  x11_dep,
]

libcheese = shared_library(
  cheese_name,
  version: libcheese_version,
  sources: sources,
  include_directories: top_inc,
  dependencies: deps + private_deps,
  c_args: '-DG_LOG_DOMAIN="@0@"'.format(cheese_name),
  install: true,
)

libcheese_dep = declare_dependency(
  include_directories: libcheese_inc,
  dependencies: deps,
  link_with: libcheese,
)

pkg.generate(
  libcheese,
  version: cheese_version,
  name: cheese_name,
  description: 'Cheese webcam utilities',
  filebase: cheese_name,
  subdirs: cheese_name,
  requires: deps,
  requires_private: private_deps,
  variables: 'exec_prefix=${prefix}',
)

# GObject Introspection
if enable_gir
  incs = [
    'Clutter-1.0',
    'GdkPixbuf-2.0',
    'Gst-1.0',
  ]

  libcheese_gir = gnome.generate_gir(
    libcheese,
    # FIXME: why include private headers if they are not installed?
    # FIXME: WidgetState enums are missing. However, they actually belong to libcheese_gtk.
    sources: sources + gir_headers + private_gir_headers,
    includes: incs,
    namespace: 'Cheese',
    nsversion: cheese_api_version,
    export_packages: cheese_name,
    install: true,
  )
endif

um_crop_area_source = files('um-crop-area.c')

sources = um_crop_area_source + files(
  'cheese-avatar-chooser.c',
  'cheese-avatar-widget.c',
  'cheese-flash.c',
  'cheese-gtk.c',
  'cheese-widget.c',
  'totem-aspect-frame.c',
)

enum_sources = gnome.mkenums_simple(
  'cheese-enums',
  sources: enum_headers,
)

deps = [
  clutter_gtk_dep,
  gtk_dep,
]

private_deps = [
  clutter_gst_dep,
  libcanberra_gtk3_dep,
  m_dep,
  x11_dep,
]

symbol_map = meson.current_source_dir() / 'cheese-gtk.map'
ldflags = cc.get_supported_link_arguments('-Wl,--version-script,@0@'.format(symbol_map))

libcheese_gtk_name = cheese_name + '-gtk'

libcheese_gtk = shared_library(
  libcheese_gtk_name,
  version: libcheese_gtk_version,
  sources: sources + enum_sources,
  include_directories: top_inc,
  dependencies: deps + private_deps + [libcheese_dep],
  c_args: '-DG_LOG_DOMAIN="@0@"'.format(libcheese_gtk_name),
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
)

libcheese_gtk_dep = declare_dependency(
  sources: enum_sources[1],
  dependencies: deps + [libcheese_dep],
  link_with: libcheese_gtk,
)

pkg.generate(
  libcheese_gtk,
  version: cheese_version,
  name: libcheese_gtk_name,
  description: 'Cheese webcam widgets',
  filebase: libcheese_gtk_name,
  subdirs: cheese_name,
  requires: deps,
  #requires_private: private_deps,
  variables: 'exec_prefix=${prefix}',
)
