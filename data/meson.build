desktop_file = i18n.merge_file(
        input: 'am.kolunmi.GtkCrusaderVillage.desktop.in',
       output: 'am.kolunmi.GtkCrusaderVillage.desktop',
         type: 'desktop',
       po_dir: '../po',
      install: true,
  install_dir: get_option('datadir') / 'applications'
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

appstream_file = i18n.merge_file(
        input: 'am.kolunmi.GtkCrusaderVillage.metainfo.xml.in',
       output: 'am.kolunmi.GtkCrusaderVillage.metainfo.xml',
       po_dir: '../po',
      install: true,
  install_dir: get_option('datadir') / 'metainfo'
)

appstreamcli = find_program('appstreamcli', required: false, disabler: true)
test('Validate appstream file', appstreamcli,
     args: ['validate', '--no-net', '--explain', appstream_file])






gschema_conf = configuration_data()
if host_machine.system() == 'windows'
  gschema_conf.set('python', 'python.exe')
  gschema_conf.set('pythonpath', '')
else
  gschema_conf.set('python', python_install.full_path())
  gschema_conf.set('pythonpath', python_install_dir / python_data_dir)
endif
configure_file(
  input: 'am.kolunmi.GtkCrusaderVillage.gschema.xml.in',
  output: 'am.kolunmi.GtkCrusaderVillage.gschema.xml',
  configuration: gschema_conf,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)

compile_schemas = find_program('glib-compile-schemas', required: false, disabler: true)
test('Validate schema file',
     compile_schemas,
     args: ['--strict', '--dry-run', meson.current_source_dir()])





service_conf = configuration_data()
service_conf.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
  input: 'am.kolunmi.GtkCrusaderVillage.service.in',
  output: 'am.kolunmi.GtkCrusaderVillage.service',
  configuration: service_conf,
  install_dir: get_option('datadir') / 'dbus-1' / 'services'
)

subdir('icons')
