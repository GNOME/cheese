project('cheese', 'c', 'vala', version : '3.25.1')

add_global_arguments('-DGETTEXT_PACKAGE="cheese"', language : 'c')
add_global_arguments('-DGNOME_DESKTOP_USE_UNSTABLE_API=1', language : 'c')

conf_data = configuration_data()
# FIXME: hack
conf_data.set_quoted('PACKAGE_LOCALEDIR', '/usr/share/locale')
conf_data.set_quoted('PACKAGE_NAME', 'Cheese')
conf_data.set_quoted('PACKAGE_TARNAME', meson.project_name())
conf_data.set_quoted('PACKAGE_URL', 'https://wiki.gnome.org/Apps/Cheese/')
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
configure_file(output : 'config.h',
               configuration : conf_data)

libcheese_version = '8.0.7'
libcheese_gtk_version = '25.0.7'

clutter = dependency('clutter-1.0', version : '>= 1.13.2')
cluttergst = dependency('clutter-gst-3.0', version : '>= 3.0.0')
cluttergtk = dependency('clutter-gtk-1.0')
gdk_pixbuf = dependency('gdk-pixbuf-2.0')
glib = dependency('glib-2.0', version : '>= 2.39.90')
gnome_desktop = dependency('gnome-desktop-3.0')
gstreamer = dependency('gstreamer-plugins-bad-1.0', version : '>= 1.4')
gstreamer_utils = dependency('gstreamer-pbutils-1.0')
gtk = dependency('gtk+-3.0', version : '>= 3.13.4')
libcanberra = dependency('libcanberra')
# FIXME: Hack, depends on VAPI file being named the same as pkg-config file.
libcanberra_gtk = dependency('libcanberra-gtk3', version : '>= 0.26')
video_effects = dependency('gnome-video-effects', required : false)
x11 = dependency('x11')

# TODO: warn about gstreamer plugins being missing?
# TODO: tests
# TODO: man page utilities
# TODO: generate ChangeLog

xsltproc = find_program('xsltproc')

gnome = import('gnome')
i18n = import('i18n')

subdir('data')
subdir('docs')
subdir('help')
subdir('libcheese')
subdir('po')
subdir('src')
