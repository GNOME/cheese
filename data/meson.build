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

desktop_conf = configuration_data()
desktop_conf.set('PACKAGE_VERSION', cheese_version)

desktop = cheese_namespace + '.desktop'

desktop_in = configure_file(
  input: desktop + '.in.in',
  output: desktop + '.in',
  configuration: desktop_conf,
)

custom_target(
  desktop,
  input: desktop_in,
  output: desktop,
  command: intltool_desktop_cmd,
  install: true,
  install_dir: cheese_datadir / 'applications',
)

appdata = cheese_namespace + '.appdata.xml'

custom_target(
  appdata,
  input: appdata + '.in',
  output: appdata,
  command: intltool_xml_cmd,
  install: true,
  install_dir: cheese_datadir / 'metainfo',
)

service_conf = configuration_data()
service_conf.set('bindir', cheese_prefix / cheese_bindir)

service = cheese_namespace + '.service'

configure_file(
  input: service + '.in',
  output: service,
  configuration: service_conf,
  install: true,
  install_dir: dbus_servicedir,
)

install_data(
  cheese_namespace + '.gschema.xml',
  install_dir: schemasdir,
)

foreach size: ['16x16', '22x22', '24x24', '32x32', '48x48', '256x256']
  install_data(
    'icons' / size / cheese_namespace + '.png',
    install_dir: cheese_datadir / 'icons' / 'hicolor' / size / 'apps',
  )
endforeach

install_data(
  'icons' / 'symbolic' / cheese_namespace + '-symbolic.svg',
  install_dir: cheese_datadir / 'icons' / 'hicolor' / 'symbolic' / 'apps',
)
