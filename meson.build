project(
     'openpower-vpd-parser',
     'c',
     'cpp',
     default_options: [
       'warning_level=3',
       'werror=true',
       'cpp_std=c++17',
       'buildtype=debugoptimized'
     ],
     version: '1.0'
)

build_tests = get_option('tests')

add_global_arguments('-Wno-psabi', language : ['c', 'cpp'])

sdbusplus = dependency('sdbusplus')
phosphor_logging = dependency('phosphor-logging')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')

compiler = meson.get_compiler('cpp')
python = find_program('python3', required:true)

compiler.has_header('CLI/CLI.hpp')
compiler.has_header('nlohmann/json.hpp')

# Disable FORTIFY_SOURCE when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-U_FORTIFY_SOURCE',language:['cpp','c'])
  message('Disabling FORTIFY_SOURCE as optimization is set to 0')
endif

configure_file(output: 'config.h',
                       configuration :{
                       'INVENTORY_JSON_DEFAULT': '"'+get_option('INVENTORY_JSON_DEFAULT')+'"',
                       'VPD_FILES_PATH': '"'+get_option('VPD_FILES_PATH')+'"',
                       'INVENTORY_PATH': '"'+get_option('INVENTORY_PATH')+'"',
                       'IPZ_INTERFACE': '"'+get_option('IPZ_INTERFACE')+'"',
                       'INVENTORY_MANAGER_SERVICE': '"'+get_option('INVENTORY_MANAGER_SERVICE')+'"',
                       'BUSNAME' : '"' + get_option('BUSNAME') + '"',
                       'OBJPATH' : '"' + get_option('OBJPATH') + '"',
                       'IFACE' : '"' + get_option('IFACE') + '"',
                       'OBJECT_MAPPER_SERVICE' : '"'+get_option('OBJECT_MAPPER_SERVICE')+'"',
                       'OBJECT_MAPPER_OBJECT' : '"'+get_option('OBJECT_MAPPER_OBJECT')+'"',
                       'POWER_SUPPLY_TYPE_INTERFACE' : '"'+get_option('POWER_SUPPLY_TYPE_INTERFACE')+'"',
                       'INVENTORY_MANAGER_CACHE' : '"'+get_option('INVENTORY_MANAGER_CACHE')+'"',
                       'INVENTORY_JSON_SYM_LINK': '"'+get_option('INVENTORY_JSON_SYM_LINK')+'"',
                       'INVENTORY_JSON_2U': '"'+get_option('INVENTORY_JSON_2U')+'"',
                       'INVENTORY_JSON_4U': '"'+get_option('INVENTORY_JSON_4U')+'"',
                       'INVENTORY_JSON_EVEREST': '"'+get_option('INVENTORY_JSON_EVEREST')+'"',
                       'DBUS_PROP_JSON': '"'+get_option('DBUS_PROP_JSON')+'"',
                       'SYSTEM_JSON' : '"'+get_option('SYSTEM_JSON')+'"',
                       'BAD_VPD_DIR': '"'+get_option('BAD_VPD_DIR')+'"'
                       }
  )

common_SOURCES =['common_utility.cpp',
'vpd-parser/parser_factory.cpp',
                 'vpd-parser/memory_vpd_parser.cpp',
                 'vpd-parser/keyword_vpd_parser.cpp',
                 'vpd-parser/ipz_parser.cpp', 'impl.cpp', 'ibm_vpd_utils.cpp',
                 'vpdecc/vpdecc.c', 'vpdecc/vpdecc_support.c'
]

if get_option('ibm-parser').enabled()
        libgpiodcxx = dependency('libgpiodcxx')
        ibm_read_vpd_SOURCES = ['ibm_vpd_app.cpp'
                               ]+common_SOURCES

        ibm_vpd_exe = executable(
                                'ibm-read-vpd',
                                ibm_read_vpd_SOURCES,
                                dependencies: [
                                        sdbusplus,
                                        phosphor_logging,
                                        libgpiodcxx,
                                ],
                                include_directories : 'vpd-parser/',
                                install: true,
                                cpp_args : '-DIPZ_PARSER'
                            )

        vpd_tool_SOURCES = ['vpd_tool.cpp',
                            'vpd_tool_impl.cpp',
                            'vpd-manager/editor_impl.cpp',
                           ]+common_SOURCES

        vpd_tool_INCLUDE = include_directories('vpd-parser/', 'vpd-manager')

        vpd_tool_exe = executable(
                                 'vpd-tool',
                                 vpd_tool_SOURCES,
                                 dependencies: [
                                   sdbusplus
                                   ],
                                 install: true,
                                 include_directories : vpd_tool_INCLUDE
                                 )
if get_option('vpd-manager').enabled()
    subdir('vpd-manager')
endif

else
        FRUGEN = '$srcdir/extra-properties.py -e' + get_option('FRU_YAML')
        PROPGEN = '$srcdir/extra-properties.py -e' + get_option('PROP_YAML')

        src_dir = meson.source_root()
        FRU_GEN_SCRIPT = src_dir + '/writefru.py'
        FRU_GEN_SCRIPT_FILES = src_dir + '/writefru.yaml'

        PROP_GEN_SCRIPT = src_dir + '/extra-properties.py'
        PROP_GEN_SCRIPT_FILES = src_dir + '/extra-properties-example.yaml'

        writefru_hpp = custom_target('writefru.hpp',
                                     command:[python,
                                              FRU_GEN_SCRIPT,
                                              '-i',
                                              get_option('FRU_YAML')
                                     ],
                                     depend_files :['writefru.mako.hpp',
                                                    'writefru.py',
                                                    get_option('FRU_YAML')
                                     ],
                                     output:'writefru.hpp'
        )

        extra_properties_gen_hpp = custom_target(
                                        'extra-properties-gen.hpp',
                                        command:[
                                                python,
                                                PROP_GEN_SCRIPT,
                                                '-e',
                                                get_option('PROP_YAML')
                                        ],
                                        depend_files : ['extra-properties.mako.hpp',
                                                        'extra-properties.py',
                                                        get_option('PROP_YAML')
                                        ],
                                        output:'extra-properties-gen.hpp'
        )

        openpower_read_vpd_SOURCES = ['app.cpp',
                                      'args.cpp',
                                      'impl.cpp',
                                      'vpd-parser/ipz_parser.cpp',
                                      'write.cpp',
                                      'common_utility.cpp',
                                      writefru_hpp,
                                      extra_properties_gen_hpp
        ]

        openpower_read_vpd_exe= executable(
                'openpower-read-vpd',
                openpower_read_vpd_SOURCES,
                dependencies: [
                        sdbusplus,
                        phosphor_logging,
                ],
                include_directories : 'vpd-parser/',
                install: true,
        )
endif
subdir('test')
