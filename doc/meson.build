toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

gidocgen = find_program('gi-docgen', required: get_option('muttum_doc'))

gidocgen_common_args = [
  '--quiet',
  '--no-namespace-dir',
]

if get_option('werror')
  gidocgen_common_args += ['--fatal-warnings']
endif

if get_option('muttum_doc')
  if not build_gir
    error('API reference requires introspection.')
  endif

  muttum_toml = configure_file(
    input: 'muttum.toml.in',
    output: 'muttum.toml',
    configuration: toml_conf,
    install: true,
    install_dir: 'doc',
  )

  custom_target('muttum-doc',
    input: [ muttum_toml, libmuttum_gir[0] ],
    output: 'muttum',
    command: [
      gidocgen,
      'generate',
      gidocgen_common_args,
      '--add-include-path=@0@'.format(meson.current_build_dir() / 'src'),
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_build_dir()),
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depends: [],
    depend_files: [ ],
    build_by_default: true,
    install: true,
    install_dir: 'doc',
  )
endif
