install_headers([
  'cheese-avatar-chooser.h',
  'cheese-avatar-widget.h',
  'cheese-camera.h',
  'cheese-camera-device.h',
  'cheese-camera-device-monitor.h',
  'cheese-effect.h',
  'cheese-gtk.h',
  'cheese.h',
  'cheese-widget.h',
  ],
  subdir : 'cheese')

libcheese_sources = [
  'cheese.c',
  'cheese-camera.c',
  'cheese-camera-device.c',
  'cheese-camera-device-monitor.c',
  'cheese-effect.c',
  'cheese-fileutil.c',
]

libcheese_gtk_sources = [
  'cheese-flash.c',
  'cheese-gtk.c'
]

libcheese = shared_library('cheese', libcheese_sources,
  version : libcheese_version,
  dependencies : [ clutter, cluttergst, gdk_pixbuf, glib, gnome_desktop, gstreamer, gstreamer_utils, x11],
  include_directories : include_directories('..'),
  install : true)

libcheese_gtk = shared_library('cheese-gtk', libcheese_gtk_sources,
  version : libcheese_gtk_version,
  dependencies : [ cluttergtk, libcanberra, gstreamer, gtk ],
  link_with : libcheese,
  install : true)

libcheese_enums_c = gnome.mkenums('cheese-enums.c',
  sources : 'cheese-widget.h',
  fhead : '''#include "cheese-enums.h"

''',
  fprod : '''/* enumerations from \"@filename@\" */
#include "@filename@"\n
''',
  vhead : '''GType
@enum_name@_get_type (void)
{
  static GType type = 0;

  if (!type)
  {
    static const G@Type@Value _@enum_name@_values[] = {''',
  vprod : '      { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  vtail : '''      { 0, NULL, NULL }
    }

    type = g_@type@_register_static ("@EnumName@", _@enum_name@_values);
  }

  return type;
}

''',
)

libcheese_enums_h = gnome.mkenums('cheese-enums.h',
  sources : 'cheese-widget.h',
  fhead : '''#ifndef CHEESE_ENUMS_H_
#define CHEESE_ENUMS_H_

#include <glib-object.h>

G_BEGIN_DECLS

''',
  fprod : '''/* enumerations from @filename@ */
''',
  vhead : '''GType @enum_name@_get_type (void);
#define CHEESE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())
''',
  ftail : '''G_END_DECLS

#endif /* CHEESE_ENUMS_H_ */'''
)

libcheese_headers = [
  'cheese-camera.h',
  'cheese-camera-device.h',
  'cheese-camera-device-monitor.h',
  'cheese-effect.h',
  'cheese-fileutil.h'
]

introspection_sources = [
  libcheese_sources,
  libcheese_headers
]

gnome.generate_gir(libcheese,
  sources : introspection_sources,
  nsversion : '3.0',
  namespace : 'Cheese',
  symbol_prefix : 'cheese_',
  identifier_prefix : 'Cheese',
  export_packages : 'cheese',
  includes : ['Clutter-1.0', 'GdkPixbuf-2.0', 'Gst-1.0'],
  install : true
)

pkg = import('pkgconfig')

pkg.generate(libraries : 'cheese',
  name : 'cheese',
  filebase : 'cheese',
  description : 'Cheese webcam utilities',
  requires : 'clutter-1.0 gsk-pixbuf-2.0 gstreamer-1.0',
  requires_private : 'clutter-gst-3.0 gio-2.0',
  version : meson.project_version())

pkg.generate(libraries : 'cheese-gtk',
  name : 'cheese-gtk',
  filebase : 'cheese',
  description : 'Cheese webcam utilities',
  requires : 'gtk+-3.0',
  requires_private : 'cheese clutter-gtk-1.0 libcanberra-gtk3',
  version : meson.project_version())
