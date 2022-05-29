gnome = import('gnome')

#
# Shared library
#

lib_muttum_sources = [
  'muttum-engine.c',
  ]

lib_muttum_deps = [
  dependency('gio-2.0'),
  dependency('icu-i18n'),
]

libmuttum = shared_library(
  'muttum',
  lib_muttum_sources,
  dependencies: lib_muttum_deps,
  install: true,
)

gir_args = [
  '--quiet',
  '--warn-all',
]

libmuttum_gir = gnome.generate_gir(
  libmuttum,
  sources: ['muttum.h', 'muttum-engine.h'] + lib_muttum_sources,
  namespace: 'Muttum',
  nsversion: '1.0',
  identifier_prefix: 'Muttum',
  symbol_prefix: 'muttum',
  export_packages: 'muttum',
  includes: [ 'GObject-2.0', 'Gio-2.0', ],
  header: 'muttum.h',
  install: true,
  extra_args: gir_args,
  fatal_warnings: true,
)

#
# GTK application
#

muttum_sources = [
  'main.c',
  'muttum-application.c',
  'muttum-window.c',
  ]

muttum_deps = [
  dependency('gtk4'),
  dependency('icu-uc'),
  dependency('icu-i18n'),
  dependency('libadwaita-1'),
]

muttum_sources += gnome.compile_resources('muttum-resources',
  'muttum.gresource.xml',
  c_name: 'muttum'
)

executable('muttum', muttum_sources,
  dependencies: muttum_deps,
  link_with: libmuttum,
  install: true,
)
