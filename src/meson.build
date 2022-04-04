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

le_bon_mot_sources = [
  'main.c',
  'le_bon_mot-application.c',
  'le_bon_mot-window.c',
  ]

#
# GTK application
#

le_bon_mot_deps = [
  dependency('gtk4'),
  dependency('icu-uc'),
  dependency('icu-i18n'),
  dependency('libadwaita-1'),
]

gnome = import('gnome')

le_bon_mot_sources += gnome.compile_resources('le_bon_mot-resources',
  'le_bon_mot.gresource.xml',
  c_name: 'le_bon_mot'
)

executable('le-bon-mot', le_bon_mot_sources,
  dependencies: le_bon_mot_deps,
  link_with: liblebonmot,
  install: true,
)
