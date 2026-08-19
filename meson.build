project('ProbabilityGuesser', 'c')

# dependencies
curl = dependency('libcurl', required: true)
cjson = dependency('libcjson')

dependencies = [curl, cjson]

# Release build flags
build_type = get_option('buildtype')

release_args = []

if build_type == 'release'
  if meson.get_compiler('c').get_id() == 'msvc'
    release_args = [
      '/O2',
      '/DNDEBUG',
    ]
  else
    release_args = [
      '-O3',
      '-DNDEBUG',
      '-ffunction-sections',
      '-fdata-sections',
    ]
  endif
endif

# App Icon
win = import('windows')
app_icon = win.compile_resources('resources.rc')

src_files = ['ProbabilityGuesser.c', 'data.c', 'predictor.c']
executable('ProbabilityGuesser', src_files, app_icon, c_args: release_args, dependencies: dependencies)