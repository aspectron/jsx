{
    'includes': ['../../common.gypi'],

    'targets': [
        {
            'target_name': 'pion',
            'type': 'static_library',
            'msvs_guid': 'DBF05BE8-0201-4A5C-A8BD-812B6CDA5C2E',
            'dependencies': ['../extern.gyp:boost', '../extern.gyp:openssl', '../zlib/zlib.gyp:zlib'],
            'defines': ['PION_STATIC_LINKING'],
            'include_dirs': ['include'],
            'direct_dependent_settings': {
                'defines': ['PION_STATIC_LINKING'],
                'include_dirs': ['include'],
            },
            'actions': [{
                'variables': {
                    'cp': 'cp',
                    'suffix': 'in',
                    'conditions': [
                        ['OS=="win"', {
                            'cp': 'copy',
                            'suffix': 'win',
                        }],
                        ['OS=="mac"', {
                            'suffix': 'xcode',
                        }],
                    ]
                },
                'action_name': 'copy_config.hpp',
                'message': 'Copy config.hpp',
                'inputs':  ['include/pion/config.hpp.<(suffix)'],
                'outputs': ['include/pion/config.hpp'],
                'action': ['<(cp)', '<@(_inputs)', '<(_outputs)'],
                'msvs_cygwin_shell': 0,
            }],
            'sources': [
                # includes
                'include/pion/admin_rights.hpp',
                'include/pion/algorithm.hpp',
                'include/pion/config.hpp',
                'include/pion/error.hpp',
                'include/pion/hash_map.hpp',
                'include/pion/logger.hpp',
                'include/pion/plugin.hpp',
                'include/pion/plugin_manager.hpp',
                'include/pion/process.hpp',
                'include/pion/scheduler.hpp',
                'include/pion/user.hpp',
                'include/pion/http/auth.hpp',
                'include/pion/http/basic_auth.hpp',
                'include/pion/http/cookie_auth.hpp',
                'include/pion/http/message.hpp',
                'include/pion/http/parser.hpp',
                'include/pion/http/plugin_server.hpp',
                'include/pion/http/plugin_service.hpp',
                'include/pion/http/reader.hpp',
                'include/pion/http/request.hpp',
                'include/pion/http/request_reader.hpp',
                'include/pion/http/request_writer.hpp',
                'include/pion/http/response.hpp',
                'include/pion/http/response_reader.hpp',
                'include/pion/http/response_writer.hpp',
                'include/pion/http/server.hpp',
                'include/pion/http/types.hpp',
                'include/pion/http/writer.hpp',
                'include/pion/spdy/decompressor.hpp',
                'include/pion/spdy/parser.hpp',
                'include/pion/spdy/types.hpp',
                'include/pion/tcp/connection.hpp',
                'include/pion/tcp/server.hpp',
                'include/pion/tcp/stream.hpp',
                'include/pion/tcp/timer.hpp',

                # sources
                'src/admin_rights.cpp',
                'src/algorithm.cpp',
                'src/http_auth.cpp',
                'src/http_basic_auth.cpp',
                'src/http_cookie_auth.cpp',
                'src/http_message.cpp',
                'src/http_parser.cpp',
                'src/http_plugin_server.cpp',
                'src/http_reader.cpp',
                'src/http_server.cpp',
                'src/http_types.cpp',
                'src/http_writer.cpp',
                'src/logger.cpp',
                'src/plugin.cpp',
                'src/process.cpp',
                'src/scheduler.cpp',
                'src/spdy_decompressor.cpp',
                'src/spdy_parser.cpp',
                'src/tcp_server.cpp',
                'src/tcp_timer.cpp',
            ],
        },
    ],
}