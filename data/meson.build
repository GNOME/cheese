resource_data = files(
  'cheese-viewport.json',
  'cheese-main-window.ui',
  'cheese-prefs.ui',
  'headerbar.ui',
  'menus.ui',
  'pixmaps/cheese-1.svg',
  'pixmaps/cheese-2.svg',
  'pixmaps/cheese-3.svg',
  'pixmaps/thumbnail-frame.png',
)

resource_sources = gnome.compile_resources(
  'cheese-resource',
  cheese_namespace + '.gresource.xml',
  c_name: cheese_name,
  dependencies: resource_data,
  export: true,
  install_header: true,
  install_dir: cheese_includedir / cheese_name,
)

desktop = cheese_namespace + '.desktop'

i18n.merge_file(
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: cheese_datadir / 'applications',
)

appdata = cheese_namespace + '.appdata.xml'

i18n.merge_file(
  input: appdata + '.in',
  output: appdata,
  po_dir: po_dir,
  install: true,
  install_dir: cheese_datadir / 'metainfo',
)

configure_file(
  input: cheese_namespace + '.service.in',
  output: '@BASENAME@',
  configuration: {'bindir': cheese_prefix / cheese_bindir},
  install: true,
  install_dir: dbus_session_bus_services_dir,
)

compile_schemas = gnome.compile_schemas()

install_data(
  cheese_namespace + '.gschema.xml',
  install_dir: gio_schemasdir,
)

install_data(
  'icons/scalable/' + cheese_namespace + '.svg',
  install_dir: cheese_datadir / 'icons/hicolor/scalable/apps',
)

install_data(
  'icons/symbolic/' + cheese_namespace + '-symbolic.svg',
  install_dir: cheese_datadir / 'icons/hicolor/symbolic/apps',
)
