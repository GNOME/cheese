i18n.merge_file(
  input: 'org.gnome.Cheese.appdata.xml.in',
  output: 'org.gnome.Cheese.appdata.xml',
  type: 'xml',
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

cheese_resources = gnome.compile_resources('cheese-resources',
                                           'org.gnome.Cheese.gresource.xml',
                                           source_dir : '.',
                                           c_name : 'cheese')

compiled = gnome.compile_schemas()
install_data('org.gnome.Cheese.gschema.xml',
             install_dir : 'share/glib-2.0/schemas')

subdir('icons')
