sources = [
  files(
    'cheese-application.vala',
    'cheese-countdown.vala',
    'cheese-effects-manager.vala',
    'cheese-main.vala',
    'cheese-preferences.vala',
    'cheese-window.vala',
    'thumbview/cheese-thumbnail.c',
    'thumbview/cheese-thumb-view.c',
    'thumbview/eog-thumb-nav.c',
    'vapi/libcanberra-gtk3.vapi'
  ),
  compile_schemas
]

deps = [
  cheese_common_dep,
  cheese_thumbview_dep,
  config_dep,
  gnome_desktop_dep,
  eogthumbnav_dep,
  libcanberra_dep,
  libcanberra_gtk3_dep,
  libcheese_gtk_dep,
  posix_dep,
]

cflags = [
  '-DPACKAGE_LOCALEDIR="@0@"'.format(cheese_prefix / cheese_localedir),
  '-DGNOME_DESKTOP_USE_UNSTABLE_API=1',
  '-DGETTEXT_PACKAGE="@0@"'.format(cheese_name),
]

executable(
  cheese_name,
  sources: sources + resource_sources,
  include_directories: top_inc,
  dependencies: deps,
  c_args: cflags,
  install: true,
)
