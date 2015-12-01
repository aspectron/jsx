#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https://github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#
# Common settings include file
{
    'variables': {
        'configuration%': 'Release',
        'target_arch%': 'x86',
        'out_dir%': '<(jsx)/bin',
    },

    'target_defaults': {
        'default_configuration': '<(configuration)',
        'msbuild_props': ['<(jsx)/build/msvs.props'],
        'msvs_cygwin_shell': 0,
        'configurations':
        {
            'Debug': {
                'defines': ['DEBUG', '_DEBUG'],
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'BasicRuntimeChecks': 3, # EnableFastChecks
                        'RuntimeLibrary': 3, # MultiThreadedDebugDLL
                        'Optimization': 0, # Disable
                    },
                    'VCLinkerTool': {
                        'LinkIncremental': 2, # YES
                    },
                },
            },
            'Release': {
                'defines': ['NDEBUG'],
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'RuntimeLibrary': 2, # MultiThreadedDLL
                        'Optimization': 2, # Speed
                        'WholeProgramOptimization': 'true',
                        'EnableFunctionLevelLinking': 'true',
                        'EnableIntrinsicFunctions': 'true',
                    },
                    'VCLinkerTool': {
                        'LinkIncremental': 1, # NO
                        'LinkTimeCodeGeneration': 1, # YES
                    },
                    'VCLibrarianTool': {
                        'LinkTimeCodeGeneration': 'true',
                    },
                },
            },
            'conditions': [
                ['OS=="win"', {
                    'Debug_x64': {
                        'inherit_from': ['Debug'],
                        'msvs_configuration_platform': 'x64',
                    },
                    'Release_x64': {
                        'inherit_from': ['Release'],
                        'msvs_configuration_platform': 'x64',
                    },
                }],
            ],
        },

        'target_conditions': [
            ['OS=="linux" and _type in ["executable", "shared_library", "loadable_module"]', {
                'product_dir': '<(out_dir)'
            }],
        ],

        'conditions': [
            ['OS=="win"', {
                'msvs_configuration_attributes': {
                    'OutputDirectory':'<(jsx)/bin/$(Configuration) $(Platform)/',
                    'IntermediateDirectory' : '<(jsx)/obj/$(Configuration) $(Platform)/$(ProjectName)/',
                    'CharacterSet': 1, # Unicode
                },
                'msvs_settings':{
                    'VCCLCompilerTool': {
                        'PreprocessorDefinitions': [
                            '_WIN32', '_WIN32_WINNT=0x0601', 'WIN32', '_CRT_SECURE_NO_DEPRECATE',
                            '_CRT_SECURE_NO_WARNINGS', '_CRT_NONSTDC_NO_WARNINGS', '_SCL_SECURE_NO_WARNINGS'
                        ],
                        'AdditionalOptions': ['/Zm392', '/MP'],
                        'target_conditions': [ ['_type=="static_library"', { 'WarningLevel': 1 }, { 'WarningLevel': 4 }], ],
                        'DebugInformationFormat': 3, # ProgramDatabase
                        'DisableSpecificWarnings': ['4127', '4275', '4512'], #'4351', '4482', '4503', '4510', '4610',],
                    },
                    'VCLinkerTool': {
                        'GenerateDebugInformation': 'true',
                       # 'AdditionalOptions': ['/IGNORE:4099'],
                        'AdditionalDependencies': ['%(AdditionalDependencies)'], #['ws2_32.lib', 'winmm.lib']
                    },
                },
            },
            {
                'cflags': ['-fPIC'],
                'cflags_cc': ['-std=c++0x'],
                'ldflags': ['-Wl,-rpath=\$$ORIGIN'],
            }], 
            ['OS=="mac"', {
                'xcode_settings': {
                    'SYMROOT': '<(out_dir)',
#                    'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',
                    'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11'],
#                   'OTHER_LDFLAGS': ['-stdlib=libc++'],
                    'LD_DYLIB_INSTALL_NAME': '@executable_path/$EXECUTABLE_NAME',
                },
            }],
        ],
    },
}
