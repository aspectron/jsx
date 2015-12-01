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
    'targets': [
        {
            'target_name': 'jsx-app',
            'type': 'executable',
            'product_name': 'jsx',
            # jsx-lib target already produces 'product_name': 'jsx', set different PDB file name in Visual C++
            'msvs_settings': { 'VCLinkerTool': { 'ProgramDatabaseFile': '$(TargetDir)jsx$(TargetExt).pdb' } },
            'msvs_guid': 'CA6CE428-89DF-4060-B52B-11D0C912F3C2',
            'dependencies': [
                'extern/extern.gyp:*',
                'jsx-lib.gyp:jsx-lib',
            ],
            'sources': [ 'src/jsx.cpp' ],
            'conditions': [
                ['OS=="win"', {
                    'libraries': [], # Boost uses autolinking
                }],
                ['OS=="mac"', {
                    'libraries': [
                        'libboost_exception.a',
                        'libboost_filesystem.a',
                        'libboost_program_options.a',
                    ],
                }],
                ['OS=="linux"', {
                    'libraries': [
                        '-lboost_exception',
                        '-lboost_filesystem',
                        '-lboost_program_options',
                    ],
                    'ldflags': ['-Wl,-R\$$ORIGIN'],
                }],
            ],
        },
    ]
}
