project(
  'ProbabilityGuesser',
  ['c', 'cpp'],
  default_options: [
    'warning_level=3',
  ],
)

# dependencies
curl = dependency('libcurl', required: true)
cjson = dependency('libcjson')

dependencies = [curl, cjson]
math = meson.get_compiler('c').find_library('m', required: false)
dependencies += math

# Build-specific flags
build_type = get_option('buildtype')
c_compiler = meson.get_compiler('c')

build_args = []
build_link_args = []

if build_type == 'debug' or build_type == 'debugoptimized'
  build_args = ['-DDEBUG']
elif build_type == 'release' or build_type == 'minsize'
  if c_compiler.get_argument_syntax() == 'msvc'
    build_args = ['/DNDEBUG']
    build_link_args = ['/OPT:REF', '/OPT:ICF']
  else
    build_args = [
      '-DNDEBUG',
      '-ffunction-sections',
      '-fdata-sections',
    ]
    build_link_args = ['-Wl,--gc-sections']
  endif
endif

src_files = ['src/main.c', 'src/data.c', 'src/predictor.c', 'src/news_checker.c', 'include/re.c']

# Windows resources are unavailable on other operating systems.
if host_machine.system() == 'windows'
  win = import('windows')
  src_files += win.compile_resources('resources.rc')
endif

executable(
  'ProbabilityGuesser',
  src_files,
  c_args: build_args,
  link_args: build_link_args,
  dependencies: dependencies,
)

subdir('tests')
