gtest_dep = dependency('gtest', required: true)

unit_tests = executable(
  'unit_tests',
  sources: ['dummy_test.cpp'],
  include_directories: include_directories('..'),
  dependencies: [gtest_dep, cjson, math],
  c_args: ['-DCURL_STATICLIB'],
  cpp_args: ['-DCURL_STATICLIB'],
)

test('unit_tests', unit_tests, protocol: 'gtest')
