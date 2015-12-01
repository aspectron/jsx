#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https://github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#
# External libraries built with non-GYP scripts
{
    'targets': [
        {
            'target_name': 'v8',
            'type': 'none',
            'msvs_guid': '269B5A2C-508E-4766-A7B1-13704C27AF91',
            'variables': {
                'build_args': [],
                'include_dir': 'v8/include',
                'library_dir': 'v8/lib',
                'conditions': [
                    ['OS=="win"', {
                        'build_args': ['--msvc=$(VisualStudioVersion)', '--platform=$(ShortPlatform)', '--config=$(Configuration)'],
                        'library_dir': 'v8/lib/$(ShortPlatform)/$(Configuration)',
                    }],
                    ['OS=="linux"', {
                        'library_dir': '<(jsx)/extern/v8/lib',
                    }],
                ],
                # evaluate 'library_files' in another variables subdictionary
                # since it depends on 'library_dir' variable
                'variables': {
                    'library_dir': '<(library_dir)',
                    'library_files': [
                        '<(library_dir)/<(SHARED_LIB_PREFIX)v8<(SHARED_LIB_SUFFIX)',
                        '<(library_dir)/<(SHARED_LIB_PREFIX)icui18n<(SHARED_LIB_SUFFIX)',
                        '<(library_dir)/<(SHARED_LIB_PREFIX)icuuc<(SHARED_LIB_SUFFIX)',
                    ],
                    'conditions': [
                        ['OS=="win"', {
                            'library_files': [
                                '<(library_dir)/<(STATIC_LIB_PREFIX)v8<(STATIC_LIB_SUFFIX)',
                                '<(library_dir)/<(SHARED_LIB_PREFIX)icudt<(SHARED_LIB_SUFFIX)',
                            ],
                        }],
                    ],
                },
                'library_files': '<(library_files)',
            },
            'actions': [
                {
                    'action_name': 'build_V8',
                    'inputs': ['build_v8.py'],
                    'outputs': ['<@(library_files)'],
                    'action': ['<(PYTHON)', 'build_v8.py', '<@(build_args)'],
                    'message': 'Building V8...', 
                    'msvs_cygwin_shell': 0,
                },
            ],
            'conditions': [
                ['OS=="win"', {
                    'copies': [
                        { 'destination': '<(PRODUCT_DIR)', 'files': [
                            '<(library_dir)/v8.dll',
                            '<(library_dir)/icudt.dll',
                            '<(library_dir)/icui18n.dll',
                            '<(library_dir)/icuuc.dll',
                        ] },
                    ],
                }],
                ['OS=="mac"', {
                    'copies': [
                        { 'destination': '<(PRODUCT_DIR)', 'files': ['<@(library_files)'] },
                    ],
                }],
                ['OS=="linux"', {
                    'copies': [
                        { 'destination': '<(out_dir)', 'files': ['<@(library_files)'] },
                    ],
                }],
            ],
            'direct_dependent_settings': {
                'defines': ['V8_USE_UNSAFE_HANDLES', 'V8_DISABLE_DEPRECATIONS'],
                'include_dirs': ['<(include_dir)'],
                'library_dirs': ['<(library_dir)'],
                'conditions': [
                    ['OS=="win"',   { 'libraries': ['-lv8'] }],
                    ['OS=="mac"',   { 'libraries': ['libv8.dylib'] }],
                    ['OS=="linux"', { 'libraries': ['-lv8', '-licui18n', '-licuuc'] }],
                ],
            },
        },
        {
            'target_name': 'v8pp',
            'type': 'none',
            'msvs_guid': '2D92FB1E-EDB9-4362-B760-5F226A7878D4',
#            'dependencies': ['v8'],
            'direct_dependent_settings': {
                'include_dirs': ['.'],
            },
            'sources': [
                'v8pp/call_from_v8.hpp',
                'v8pp/call_v8.hpp',
                'v8pp/class.hpp',
                'v8pp/config.hpp',
                'v8pp/context.cpp',
                'v8pp/context.hpp',
                'v8pp/convert.hpp',
                'v8pp/factory.hpp',
                'v8pp/forward.hpp',
                'v8pp/module.hpp',
                'v8pp/persistent_ptr.hpp',
                'v8pp/property.hpp',
                'v8pp/proto.hpp',
                'v8pp/throw_ex.hpp',
            ],
        },
        {
            'target_name': 'openssl',
            'type': 'none',
            'msvs_guid': '672FF39E-CF14-4742-848E-7E44A0058B35',
            'variables': {
                'build_args': [],
                'include_dir': 'openssl/include',
                'library_dir': 'openssl/lib',
                'conditions': [
                    ['OS=="win"', {
                        'build_args': ['--msvc=$(VisualStudioVersion)', '--platform=$(ShortPlatform)', '--config=$(Configuration)'],
                        'library_dir': 'openssl/lib/$(ShortPlatform)',
                    }],
                    ['OS=="linux"', {
                        'library_dir': '<(jsx)/extern/openssl/lib',
                    }],
                ],
                # evaluate 'library_files' in another variables subdictionary
                # since it depends on 'library_dir' variable
                'variables': {
                    'library_dir': '<(library_dir)',
                    'conditions': [
                        ['OS=="win"', {
                            'library_files=': [
                                '<(library_dir)/libeay32.lib',
                                '<(library_dir)/ssleay32.lib',
                            ],
                        },
                        {
                            'library_files': [
                                '<(library_dir)/<(STATIC_LIB_PREFIX)crypto<(STATIC_LIB_SUFFIX)',
                                '<(library_dir)/<(STATIC_LIB_PREFIX)ssl<(STATIC_LIB_SUFFIX)',
                            ],
                        }],
                    ],
                },
                'library_files': '<(library_files)',
            },
            'actions': [
                {
                    'action_name': 'build_openssl',
                    'inputs': ['build_openssl.py'],
                    'outputs': ['<@(library_files)'],
                    'action': ['<(PYTHON)', 'build_openssl.py', '<@(build_args)'],
                    'message': 'Building OpenSSL...',
                    'msvs_cygwin_shell': 0,
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': ['<(include_dir)'],
                'library_dirs': ['<(library_dir)'],
                'conditions': [
                    ['OS=="win"',   { 'libraries': ['-llibeay32', '-lssleay32'], }],
                    ['OS=="mac"',   { 'libraries': ['libcrypto.a', 'libssl.a'], }],
                    ['OS=="linux"', { 'libraries': ['-lcrypto', '-lssl'], }],
                ],
            },
        },
        {
            'target_name': 'boost',
            'type': 'none',
            'msvs_guid': '6E3AB9C7-A31C-45FB-80AC-FFBDCB08A834',
            'variables': {
                'build_args': [],
                'include_dir': 'boost',
                'library_dir': 'boost/stage/lib',
                'conditions': [
                    ['OS=="win"', {
                        'build_args': ['--msvc=$(VisualStudioVersion)', '--platform=$(ShortPlatform)', '--config=$(Configuration)'],
                        'library_dir': 'boost/stage/$(ShortPlatform)/lib',
                    }],
                    ['OS=="linux"', {
                        'library_dir': '<(jsx)/extern/boost/stage/lib',
                    }],
                ],
            },
            'actions': [
                {
                    'action_name': 'build_boost',
                    'inputs': ['build_boost.py'],
                    'outputs': ['<(library_dir)/build.done'],
                    'action': ['<(PYTHON)', 'build_boost.py', '<@(build_args)'],
                    'message': 'Building Boost...',
                    'msvs_cygwin_shell': 0,
                },
            ],
            'direct_dependent_settings': {
                'include_dirs': ['<(include_dir)'],
                'library_dirs': ['<(library_dir)'],
            },
        },
    ],
}
