oem_SOURCES = []
if get_option('ibm_system').allowed()
    oem_SOURCES += ['oem-handler/ibm_handler.cpp']
endif

common_SOURCES = [
    'src/logger.cpp',
    'src/parser_factory.cpp',
    'src/ipz_parser.cpp',
    'src/keyword_vpd_parser.cpp',
    'src/ddimm_parser.cpp',
    'src/isdimm_parser.cpp',
    'src/parser.cpp',
    'src/worker.cpp',
    'src/backup_restore.cpp',
    'src/gpio_monitor.cpp',
    'src/event_logger.cpp',
    'src/listener.cpp',
]

vpd_manager_SOURCES = [
    'src/manager_main.cpp',
    'src/manager.cpp',
    'src/bios_handler.cpp',
] + common_SOURCES + oem_SOURCES

parser_dependencies = [
    sdbusplus,
    libgpiodcxx,
    phosphor_logging,
    phosphor_dbus_interfaces,
]

parser_build_arguments = []
if get_option('ibm_system').allowed()
    parser_build_arguments += ['-DIBM_SYSTEM']
    vpd_manager_SOURCES += 'src/single_fab.cpp'
endif

vpd_manager_exe = executable(
    'vpd-manager',
    vpd_manager_SOURCES,
    include_directories: ['../', 'include/', '../configuration/'],
    link_with: libvpdecc,
    dependencies: [parser_dependencies],
    install: true,
    cpp_args: parser_build_arguments,
)

vpd_parser_SOURCES = ['src/vpd_parser_main.cpp'] + common_SOURCES

vpd_parser_exe = executable(
    'vpd-parser',
    vpd_parser_SOURCES,
    include_directories: ['../', 'include/', '../configuration/'],
    link_with: libvpdecc,
    dependencies: parser_dependencies,
    install: true,
)
