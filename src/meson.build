le_bon_mot_sources = [
  'main.c',
  'le_bon_mot-engine.c',
  'le_bon_mot-window.c',
  'le_bon_mot-application.c',
  ]

le_bon_mot_deps = [
  dependency('gtk4'),
]

gnome = import('gnome')

le_bon_mot_sources += gnome.compile_resources('le_bon_mot-resources',
  'le_bon_mot.gresource.xml',
  c_name: 'le_bon_mot'
)

executable('le-bon-mot', le_bon_mot_sources,
  dependencies: le_bon_mot_deps,
  install: true,
)
