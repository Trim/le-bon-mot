gnome = import('gnome')

#
# Shared library
#

lib_le_bon_mot_sources = [
  'le_bon_mot-engine.c',
  ]

lib_le_bon_mot_deps = [
  dependency('gio-2.0'),
  dependency('icu-i18n'),
]

liblebonmot = shared_library(
  'lebonmot',
  lib_le_bon_mot_sources,
  dependencies: lib_le_bon_mot_deps,
  install: true,
)

gir_args = [
  '--quiet',
  '--warn-all',
]

liblebonmot_gir = gnome.generate_gir(
  liblebonmot,
  sources: ['le_bon_mot.h', 'le_bon_mot-engine.h'] + lib_le_bon_mot_sources,
  namespace: 'LeBonMot',
  nsversion: '1.0',
  identifier_prefix: 'LeBonMot',
  symbol_prefix: 'le_bon_mot',
  export_packages: 'le_bon_mot',
  includes: [ 'GObject-2.0', 'Gio-2.0', ],
  header: 'le_bon_mot.h',
  install: true,
  extra_args: gir_args,
  fatal_warnings: true,
)

#
# GTK application
#

le_bon_mot_sources = [
  'main.c',
  'le_bon_mot-application.c',
  'le_bon_mot-window.c',
  ]

le_bon_mot_deps = [
  dependency('gtk4'),
  dependency('icu-uc'),
  dependency('icu-i18n'),
  dependency('libadwaita-1'),
]

le_bon_mot_sources += gnome.compile_resources('le_bon_mot-resources',
  'le_bon_mot.gresource.xml',
  c_name: 'le_bon_mot'
)

executable('le-bon-mot', le_bon_mot_sources,
  dependencies: le_bon_mot_deps,
  link_with: liblebonmot,
  install: true,
)
