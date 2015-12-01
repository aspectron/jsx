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
            'target_name': 'jsx-doc',
            'type': 'none',
            'dependencies': ['jsx-app.gyp:jsx-app'],
            'variables': {
                'jsx_app': '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)jsx<(EXECUTABLE_SUFFIX)',
                'doc_dir': 'doc/jsx',
                'conditions': [
                    ['OS=="win"', {
                        'doxygen_check_code': '<!(where /q doxygen.exe & echo %errorlevel%)',
                    },{
                        'doxygen_check_code': '<!(command -v doxygen; echo $?)',
                    }],
                    ['OS=="linux"', {
                        'jsx_app': '<(out_dir)/<(EXECUTABLE_PREFIX)jsx<(EXECUTABLE_SUFFIX)'
                    }],
                ],
            },
            'actions': [
                {
                    'action_name': 'gendoc',
                    'inputs': ['<@(source_files)', '<@(library_files)'],
                    'outputs': ['<(doc_dir)/all.md'],
                    'action': ['<(jsx_app)', 'build/tools/gendoc/run.js', '<(doc_dir)', 'rte/libraries/.+[.]js', 'src/.+[.]cpp'],
                    'msvs_cygwin_shell': 0,
                    'message': 'Building JavaScript API documentation...',
                },
            ],
            'conditions': [
                ['doxygen_check_code==0', {
                    'actions': [
                    {
                        'action_name': 'doxygen',
                        'inputs': ['Doxyfile', 'README.md', '<@(include_files)', '<@(source_files)'],
                        'outputs': ['doc/html/index.html'],
                        'action': ['doxygen'],
                        'msvs_cygwin_shell': 0,
                        'message': 'Building Doxygen documentation...',
                    }]
                }],
            ],
        },
    ]
}