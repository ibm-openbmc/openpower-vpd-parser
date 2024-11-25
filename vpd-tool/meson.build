compiler = meson.get_compiler('cpp')
if compiler.has_header('CLI/CLI.hpp')
    CLI11_dep = declare_dependency()
else
    CLI11_dep = dependency('CLI11')
endif

sdbusplus = dependency('sdbusplus', fallback: [ 'sdbusplus', 'sdbusplus_dep' ])
dependency_list = [CLI11_dep, sdbusplus]

conf_data = configuration_data()
conf_data.set_quoted('INVENTORY_JSON_SYM_LINK', get_option('INVENTORY_JSON_SYM_LINK'))
configure_file(output: 'config.h',
            configuration : conf_data)

sources = ['src/vpd_tool_main.cpp',
            'src/vpd_tool.cpp']

vpd_tool_exe = executable('vpd-tool',
                          sources,
                          include_directories : ['include/'],
                          dependencies: dependency_list,
                          install: true
                        )