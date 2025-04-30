project('gtk-crusader-village', 'c',
          version: '0.1.0',
    meson_version: '>= 1.0.0',
  default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu11' ],
)

i18n = import('i18n')
gnome = import('gnome')
cc = meson.get_compiler('c')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'gtk-crusader-village')
config_h.set_quoted('APPLICATION_ID', 'am.kolunmi.GtkCrusaderVillage')
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
if get_option('buildtype') == 'debug'
  config_h.set10('DEVELOPMENT_BUILD', true)
endif
configure_file(output: 'config.h', configuration: config_h)
add_project_arguments(['-I' + meson.project_build_root()], language: 'c')


project_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=implicit',
  '-Werror=implicit-function-declaration',
  '-Werror=incompatible-pointer-types',
  '-Werror=init-self',
  '-Werror=int-conversion',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=misleading-indentation',
  '-Werror=missing-braces',
  '-Werror=missing-include-dirs',
  '-Werror=nonnull',
  '-Werror=overflow',
  '-Werror=parenthesis',
  '-Werror=pointer-arith',
  '-Werror=pointer-to-int-cast',
  '-Werror=redundant-decls',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=shadow',
  '-Werror=strict-prototypes',
  '-Werror=trigraphs',
  '-Werror=undef',
  '-Werror=write-strings',
  '-Wformat-nonliteral',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-cast-function-type',
  '-Wno-dangling-pointer',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused',
  '-fno-strict-aliasing',
  ['-Werror=format-security', '-Werror=format=2'],
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif
foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    project_c_args += arg
  endif
endforeach
add_project_arguments(project_c_args, language: 'c')


# for sourcehold dep, make sure the the git submodule is initialized
python_exe_name = 'python'
if host_machine.system() == 'windows'
  python_exe_name += '.exe'
endif
python_install = find_program(python_exe_name)

sourcehold_script = find_program('make-sourcehold-venv.sh')
python_install_dir = get_option('prefix') / get_option('datadir')
python_data_dir = 'gtk-crusader-village-python'

sourcehold = custom_target(
  'sourcehold',
  output : [python_data_dir],
  command : [
    sourcehold_script,
    python_install,
    '@BUILD_ROOT@',
    '@SOURCE_ROOT@',
    '@PRIVATE_DIR@',
    '@OUTPUT@'
  ],
  install: true,
  install_dir: python_install_dir,
)



subdir('data')
subdir('src')
subdir('po')



gnome.post_install(
     glib_compile_schemas: true,
    gtk_update_icon_cache: true,
  update_desktop_database: true,
)
