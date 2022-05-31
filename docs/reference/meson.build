cheese_xml = files('cheese.xml')

if enable_gtk_doc
  private_headers = [
    'cheese-enums.h',
    'cheese-widget-private.h',
    'totem-aspect-frame.h',
    'um-crop-area.h',
  ]

  glib_prefix = glib_dep.get_variable('prefix')
  gtk_prefix = gtk_dep.get_variable('prefix')

  fixxref_args = [
    '--html-dir=' + (cheese_prefix / gnome.gtkdoc_html_dir(cheese_name)),
    '--extra-dir=' + (glib_prefix / gnome.gtkdoc_html_dir('glib')),
    '--extra-dir=' + (glib_prefix / gnome.gtkdoc_html_dir('gio')),
    '--extra-dir=' + (gtk_prefix / gnome.gtkdoc_html_dir('gdk')),
    '--extra-dir=' + (gtk_prefix / gnome.gtkdoc_html_dir('gdk-pixbuf')),
    '--extra-dir=' + (gtk_prefix / gnome.gtkdoc_html_dir('gtk')),
  ]

  version_xml = configure_file(
    input: 'version.xml.in',
    output: '@BASENAME@',
    configuration: {'PACKAGE_VERSION': cheese_version},
  )

  gnome.gtkdoc(
    cheese_name,
    main_xml: cheese_name + '-docs.xml',
    src_dir: libcheese_inc,
    ignore_headers: private_headers,
    include_directories: top_inc,
    dependencies: libcheese_gtk_dep,
    namespace: cheese_name,
    scan_args: '--rebuild-types',
    fixxref_args: fixxref_args,
    html_assets: 'images/cheese_architecture.png',
    content_files: cheese_xml + [version_xml],
    install: true,
  )
endif

if enable_man
  xsltproc_cmd = [
    find_program('xsltproc'),
    '--output', '@OUTPUT@',
    '--nonet',
    '--path', meson.current_build_dir(),
    'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
    '@INPUT@',
  ]

  output = cheese_name + '.1'

  custom_target(
    output,
    input: cheese_xml,
    output: output,
    command: xsltproc_cmd,
    install: true,
    install_dir: cheese_mandir / 'man1',
  )
endif
