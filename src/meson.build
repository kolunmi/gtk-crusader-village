gtk_crusader_village_sources = [
  'main.c',
  'gtk-crusader-village-application.c',
  'gtk-crusader-village-window.c',
  'gtk-crusader-village-dialog-window.c',
  'gtk-crusader-village-preferences-window.c',
  'gtk-crusader-village-util-bin.c',
  'gtk-crusader-village-item-area.c',
  'gtk-crusader-village-item-area-item.c',
  'gtk-crusader-village-timeline-view.c',
  'gtk-crusader-village-timeline-view-item.c',
  'gtk-crusader-village-map-editor.c',
  'gtk-crusader-village-map-editor-overlay.c',
  'gtk-crusader-village-map-editor-status.c',
  'gtk-crusader-village-item.c',
  'gtk-crusader-village-item-store.c',
  'gtk-crusader-village-item-stroke.c',
  'gtk-crusader-village-map.c',
  'gtk-crusader-village-map-handle.c',
  'gtk-crusader-village-theme-utils.c',
  'gtk-crusader-village-brushable.c',
  'gtk-crusader-village-square-brush.c',
  'gtk-crusader-village-image-mask-brush.c',
  'gtk-crusader-village-brush-area-item.c',
  'gtk-crusader-village-brush-area.c',
]

gtk_crusader_village_deps = [
  cc.find_library('m', required: true),
  dependency('gtk4'),
  dependency('json-glib-1.0', version: '>= 1.2.0'),
]

gtk_crusader_village_sources += gnome.compile_resources('gtk-crusader-village-resources',
  'gtk-crusader-village.gresource.xml',
  c_name: 'gtk_crusader_village'
)

gtk_crusader_village = executable('gtk-crusader-village', gtk_crusader_village_sources,
  dependencies: gtk_crusader_village_deps,
       install: true,
 win_subsystem: 'windows'
)
