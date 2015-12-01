#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https://github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#
{
    'includes': ['jsx-files.gypi'],
    'targets': [
        {
            'target_name': 'js2c',
            'type': 'none',
            'actions': [
                {
                    'action_name': 'js2c',
                    'inputs': ['build/tools/js2c.py', '<@(library_files)'],
                    'outputs': ['<(SHARED_INTERMEDIATE_DIR)/native_libraries.hpp'],
                    'action': ['<(PYTHON)', 'build/tools/js2c.py', '<@(_outputs)', '<@(library_files)'],
                    'message': 'Generate <@(_outputs)...',
                    'msvs_cygwin_shell': 0,
                },
            ],
        },
        {
            'target_name': 'jsx-lib',
            'product_name': 'jsx',
            'type': 'shared_library',
            'msvs_guid': '4C602428-76A8-47C4-B525-795DC28FED62',
            'defines': ['CORE_EXPORTS', 'NOMINMAX'],
            'dependencies': [
                'extern/extern.gyp:*',
                'extern/cppdb/cppdb.gyp:cppdb',
                'extern/expat/expat.gyp:expat',
                'extern/pion/pion.gyp:pion',
                'extern/zlib/zlib.gyp:zlib',
                'js2c',
            ],
            'direct_dependent_settings': {
                'include_dirs': ['include'],
                'defines': ['NOMINMAX'],
            },
            'include_dirs': [
                'include',
                '<(SHARED_INTERMEDIATE_DIR)', # for native_libraries.hpp
            ],
            'sources': [
                '<@(include_files)',
                '<@(source_files)',
                '<@(library_files)',
            ],
            'conditions': [
                ['OS=="win"', {
                    'libraries': [
                        'iphlpapi.lib',
                        'dbghelp.lib',
                    ], # Boost uses autolinking
                }],
                ['OS=="mac"', {
                    'sources': ['src/runtime.osx.m'],
                    'libraries': [
                        'libboost_system.a',
                        'libboost_chrono.a',
                        'libboost_thread.a',
                        'libboost_filesystem.a',
                        'libboost_iostreams.a',
                        'libboost_regex.a',
                        'CoreFoundation.framework',
                        'CoreServices.framework',
                        'Cocoa.framework',
                    ]
                }],
                ['OS=="linux"', {
                    'libraries': [
                        '-lboost_system',
                        '-lboost_chrono',
                        '-lboost_thread',
                        '-lboost_filesystem',
                        '-lboost_iostreams',
                        '-lboost_regex',
                        '-lrt', '-ldl', '-lpthread'
                    ],
                }],
            ],
        },
    ],
}
