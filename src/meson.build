gtk = dependency('gtk+-3.0')
gtkmm = dependency('gtkmm-3.0', version: '>=3.24')
lxpanel = dependency('lxpanel-pi')
wfpanel = dependency('wf-panel-pi')
libpulse = dependency('libpulse')

lsources = files(
  'volumepulse.c',
  'commongui.c',
  'pulse.c',
  'bluetooth.c'
)

ldeps = [ gtk, lxpanel, libpulse ]

largs = [ '-DPACKAGE_DATA_DIR="' + lresource_dir + '"', '-DGETTEXT_PACKAGE="lxplug_' + meson.project_name() + '"' ]

shared_module(meson.project_name(), lsources,
        dependencies: ldeps,
        install: true,
        install_dir: get_option('libdir') / 'lxpanel-pi',
        c_args : largs,
        name_prefix: ''
)

wsources = files(
  'volumepulse.cpp'
)

wdeps = [ gtkmm, wfpanel, libpulse ]

wargs = [ '-DPLUGIN_NAME="' + meson.project_name() + '"', '-DPACKAGE_DATA_DIR="' + wresource_dir + '"', '-DGETTEXT_PACKAGE="wfplug_' + meson.project_name() +'"' ]

shared_module('lib' + meson.project_name(), [ lsources, wsources ],
        dependencies: wdeps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        c_args : wargs,
        cpp_args : wargs,
        name_prefix: ''
)

metadata = files()
install_data(metadata, install_dir: metadata_dir)
