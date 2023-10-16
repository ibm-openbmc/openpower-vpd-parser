gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
libgpiodcxx = dependency('libgpiodcxx')

dependecy_list = [gtest, gmock, sdbusplus, phosphor_logging, libgpiodcxx]

configuration_inc = include_directories('..', '../vpd-manager', 'vpd-manager-test', '../vpd-parser')

vpd_test = ['store/store.cpp',
            'ipz_parser/parser.cpp',
            'keyword_vpd_parser_test/kw_vpd_test.cpp',
            #'vpd-manager-test/reader_test.cpp',
            'vpd-manager-test/editor_test.cpp'
]

application_src =['../impl.cpp',
                  '../ibm_vpd_utils.cpp',
                  '../common_utility.cpp',
                  '../vpd-parser/keyword_vpd_parser.cpp',
                  '../vpd-parser/ipz_parser.cpp',
                  '../vpd-parser/parser_factory.cpp',
                  '../vpd-parser/memory_vpd_parser.cpp',
                  '../vpd-parser/isdimm_vpd_parser.cpp',
                  '../vpd-manager/editor_impl.cpp']

#application_src =['../impl.cpp',
#                   '../vpd-parser/ipz_parser.cpp',
#                   '../ibm_vpd_utils.cpp',
#                   '../common_utility.cpp',
#                   '../vpd-manager/reader_impl.cpp',
#                   '../vpd-parser/keyword_vpd_parser.cpp',
#                   '../vpd-manager/editor_impl.cpp',
#                   '../vpd-parser/parser_factory.cpp',
#                   '../vpd-parser/memory_vpd_parser.cpp',
#                   '../vpd-parser/isdimm_vpd_parser.cpp'
#                  ]

foreach t : vpd_test
    test(t, executable(t.underscorify(),
            [t, application_src],
    cpp_args: ['-DIPZ_PARSER', '-DManagerTest'],
    dependencies: dependecy_list,
    include_directories: configuration_inc,
    link_with : libvpdecc,
    ),
    workdir: meson.current_source_dir())
endforeach