project('muttum', 'c',
          version: '0.1.0',
    meson_version: '>= 0.55.0',
  default_options: [ 'warning_level=2',
                     'c_std=gnu11',
                   ],
)

i18n = import('i18n')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'muttum')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
  output: 'muttum-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.current_build_dir(),
  '-DFRENCH_DICTIONARY_PATH_URI="' + get_option('french_dictionary_path_uri') + '"',
], language: 'c')


subdir('data')
subdir('src')
subdir('po')

meson.add_install_script('build-aux/meson/postinstall.py')
