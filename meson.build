project(
     'vpd-manager',
     'c',
     'cpp',
     default_options: [
       'warning_level=3',
       'werror=true',
       'cpp_std=c++20',
       'buildtype=debugoptimized'
     ],
     version: '1.0',
     meson_version: '>=0.58.1',
)

add_global_arguments('-Wno-psabi', language : ['c', 'cpp'])

# Disable FORTIFY_SOURCE when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-U_FORTIFY_SOURCE',language:['cpp','c'])
  message('Disabling FORTIFY_SOURCE as optimization is set to 0')
endif

# Setup googletest before we import any projects that also depend on it to make
# sure we have control over its configuration
build_tests = get_option('tests')
if not build_tests.disabled()
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
            assert(not get_option('tests').enabled(),
                   'Googletest is required if tests are enabled')
        endif
    endif
endif

sdbusplus = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])

if not build_tests.disabled()
    #subdir('test')
endif

compiler = meson.get_compiler('cpp')

conf_data = configuration_data()
conf_data.set_quoted('BUSNAME', get_option('BUSNAME'))
conf_data.set_quoted('OBJPATH', get_option('OBJPATH'))
conf_data.set_quoted('IFACE', get_option('IFACE'))
conf_data.set_quoted('BAD_VPD_DIR', get_option('BAD_VPD_DIR'))
configure_file(output: 'config.h',
            configuration : conf_data)

libvpdecc_src = files(
    'vpdecc/vpdecc.c',
    'vpdecc/vpdecc_support.c'
)

libvpdecc = shared_library(
    'vpdecc',
    libvpdecc_src,
    version: meson.project_version(),
    install: true,
)

vpd_manager_SOURCES = ['src/main.cpp',
                       'src/manager.cpp',
                       'src/logger.cpp',
                       'src/utils.cpp',
                       'src/parser_factory.cpp',
                       'src/ipz_parser.cpp',
		       'src/keyword_vpd_parser.cpp']

vpd_manager_dependencies = [sdbusplus]

vpd_manager_exe = executable(
                'vpd-manager',
                vpd_manager_SOURCES,
                include_directories : 'include/',
                link_with : libvpdecc,
                dependencies : [
                                vpd_manager_dependencies,
                                ],
                install : true,
                cpp_args : '-DECC_CHECK'
                )
