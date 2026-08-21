gtest_project = subproject('gtest', default_options: ['default_library=static'])
gtest_dep = gtest_project.get_variable('gtest_dep')
cjson_test_dep = dependency('libcjson', required: true, static: true)

test_link_args = []
if host_machine.system() == 'windows' and meson.get_compiler('cpp').get_id() == 'gcc'
  test_link_args = ['-static-libgcc', '-static-libstdc++']
endif

unit_tests = executable(
  'unit_tests',
  sources: [
    'test_main.cpp',
    'curl_m.cpp',
    'predictor_test.cpp',
    'data_test.cpp',
    'news_checker_test.cpp',
    '../src/data.c',
    '../src/predictor.c',
    '../src/news_checker.c',
    '../include/re.c',
  ],
  include_directories: include_directories('..'),
  dependencies: [gtest_dep, cjson_test_dep, math],
  link_args: test_link_args,
  c_args: ['-DCURL_STATICLIB'],
  cpp_args: ['-DCURL_STATICLIB'],
)

test('unit_tests', unit_tests, protocol: 'gtest', verbose: true)
