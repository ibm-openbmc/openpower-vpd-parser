gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    cmake = import('cmake')
    gtest_opts = cmake.subproject_options()
    gtest_opts.set_override_option('warning_level', '1')
    gtest_opts.set_override_option('werror', 'false')
    gtest_proj = cmake.subproject('googletest',
                                  options: gtest_opts,
                                  required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
  else
        assert(
            not get_option('tests').allowed(),
            'Googletest is required if tests are enabled'
        )
  endif
endif

dependency_list = [gtest_dep, gmock_dep, sdbusplus]

configuration_inc = include_directories('..', '../include')

test_sources = [
  '../src/utils.cpp',
  '../src/logger.cpp'
]

tests = [
  'utest_utils.cpp'
]

foreach test_file : tests
  test(test_file, executable(test_file.underscorify(),
                     test_file,
                     test_sources,
                     include_directories: configuration_inc,
                     dependencies: dependency_list
                     ),
       workdir: meson.current_source_dir())
endforeach