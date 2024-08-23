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

add_global_arguments('-Wno-psabi', '-Wno-ignored-attributes', language : ['c', 'cpp'])

# Disable FORTIFY_SOURCE when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-U_FORTIFY_SOURCE',language:['cpp','c'])
  message('Disabling FORTIFY_SOURCE as optimization is set to 0')
endif

# Setup googletest before we import any projects that also depend on it to make
# sure we have control over its configuration
build_tests = get_option('tests')

sdbusplus = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])
phosphor_logging = dependency('phosphor-logging')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')

if not build_tests.disabled()
    subdir('test')
endif

compiler = meson.get_compiler('cpp')

conf_data = configuration_data()
conf_data.set_quoted('BUSNAME', get_option('BUSNAME'))
conf_data.set_quoted('OBJPATH', get_option('OBJPATH'))
conf_data.set_quoted('IFACE', get_option('IFACE'))
conf_data.set_quoted('BAD_VPD_DIR', get_option('BAD_VPD_DIR'))
conf_data.set_quoted('INVENTORY_JSON_DEFAULT', get_option('INVENTORY_JSON_DEFAULT'))
conf_data.set_quoted('INVENTORY_JSON_SYM_LINK', get_option('INVENTORY_JSON_SYM_LINK'))
conf_data.set_quoted('JSON_ABSOLUTE_PATH_PREFIX', get_option('JSON_ABSOLUTE_PATH_PREFIX'))
conf_data.set_quoted('SYSTEM_VPD_FILE_PATH', get_option('SYSTEM_VPD_FILE_PATH'))
conf_data.set_quoted('VPD_SYMLIMK_PATH', get_option('VPD_SYMLIMK_PATH'))
conf_data.set_quoted('PIM_PATH_PREFIX', get_option('PIM_PATH_PREFIX'))
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

libgpiodcxx = dependency(
            'libgpiodcxx',
            default_options: ['bindings=cxx'],
        )

common_SOURCES = ['src/logger.cpp',
                  'src/parser_factory.cpp',
                  'src/ipz_parser.cpp',
                  'src/keyword_vpd_parser.cpp',
                  'src/ddimm_parser.cpp',
                  'src/isdimm_parser.cpp',
                  'src/parser.cpp',
                  'src/worker.cpp',
                  'src/backup_restore.cpp',
                  'src/gpio_monitor.cpp',
                  'src/bios_handler.cpp']

vpd_manager_SOURCES = ['src/manager_main.cpp',
                    'src/manager.cpp',
                    ] + common_SOURCES

parser_dependencies = [sdbusplus, libgpiodcxx, phosphor_logging, phosphor_dbus_interfaces]

parser_build_arguments = []
if get_option('ibm_system').enabled()
    parser_build_arguments += ['-DIBM_SYSTEM']
endif

vpd_manager_exe = executable(
                'vpd-manager',
                vpd_manager_SOURCES,
                include_directories : ['include/', 'configuration/'],
                link_with : libvpdecc,
                dependencies : [
                                parser_dependencies,
                                ],
                install : true,
                cpp_args : parser_build_arguments,
                )

vpd_parser_SOURCES = ['src/vpd_parser_main.cpp',
                     ]+ common_SOURCES

vpd_parser_exe = executable(
                'vpd-parser',
                vpd_parser_SOURCES,
                include_directories : ['include/', 'configuration/'],
                link_with : libvpdecc,
                dependencies : parser_dependencies,
                install : true,
                )

package_datadir = join_paths('share', 'vpd')
install_subdir('configuration/ibm/', install_mode: 'rwxr-xr-x', install_dir: package_datadir, strip_directory: true)
