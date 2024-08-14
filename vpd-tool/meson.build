sources = ['src/vpd_tool_main.cpp',
          'src/vpd_tool.cpp']

dependency_list = [sdbusplus]

vpd_tool_exe = executable('vpd-tool',
                          sources,
                          include_directories : ['include/', '../include/'],
                          dependencies: dependency_list,
                          install: true
                        )