project(
  'ProbabilityGuesser',
  ['c', 'cpp'],
  default_options: [
    'warning_level=3',
    'b_ndebug=if-release',
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
    build_args = c_compiler.get_supported_arguments(
      '/guard:cf',
      '/sdl',
    )
    build_link_args = c_compiler.get_supported_link_arguments(
      '/DYNAMICBASE',
      '/guard:cf',
      '/NXCOMPAT',
      '/OPT:ICF',
      '/OPT:REF',
    )
  else
    build_args = c_compiler.get_supported_arguments(
      '-D_FORTIFY_SOURCE=2',
      '-ffunction-sections',
      '-fdata-sections',
      '-fstack-protector-strong',
    )
    build_link_args = c_compiler.get_supported_link_arguments(
      '-Wl,--gc-sections',
      '-Wl,-z,noexecstack',
      '-Wl,-z,relro,-z,now',
    )
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
