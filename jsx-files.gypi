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
    'variables': {
        'include_files': [
            'include/jsx/aes.hpp',
            'include/jsx/aligned_allocator.hpp',
            'include/jsx/api.hpp',
            'include/jsx/async_queue.hpp',
            'include/jsx/config.hpp',
            'include/jsx/console.hpp',
            'include/jsx/core.hpp',
            'include/jsx/cpus.hpp',
            'include/jsx/crypto.hpp',
            'include/jsx/db.hpp',
            'include/jsx/directory_monitor.hpp',
            'include/jsx/events.hpp',
            'include/jsx/fs.hpp',
            'include/jsx/geometry.hpp',
            'include/jsx/library.hpp',
            'include/jsx/log.hpp',
            'include/jsx/node_support.hpp',
            'include/jsx/os.hpp',
            'include/jsx/platform.hpp',
            'include/jsx/process.hpp',
            'include/jsx/runtime.hpp',
            'include/jsx/scriptstore.hpp',
            'include/jsx/tcp.hpp',
            'include/jsx/threads.hpp',
            'include/jsx/types.hpp',
            'include/jsx/utils.hpp',
            'include/jsx/v8_buffer.hpp',
            'include/jsx/v8_callback.hpp',
            'include/jsx/v8_core.hpp',
            'include/jsx/v8_main_loop.hpp',
            'include/jsx/v8_timers.hpp',
            'include/jsx/v8_uuid.hpp',
            'include/jsx/xml.hpp',

            # HTTP files
            'include/jsx/http.hpp',
            'include/jsx/http_client.hpp',
            'include/jsx/http_server.hpp',
            'include/jsx/http_logger.hpp',
            'include/jsx/url.hpp',
            'include/jsx/websocket.hpp',
            'include/jsx/websocket_hixie_76.hpp',
            'include/jsx/websocket_hybi_17.hpp',
        ],
        'source_files': [
            'src/aes.cpp',
            'src/async_queue.cpp',
            'src/console.cpp',
            'src/core.cpp',
            'src/cpus.cpp',
            'src/crypto.cpp',
            'src/crypto_groups.hpp',
            'src/db.cpp',
            'src/directory_monitor.cpp',
            'src/events.cpp',
            'src/fs.cpp',
            'src/library.cpp',
            'src/log.cpp',
            'src/node_support.cpp',
            'src/os.cpp',
            'src/process.cpp',
            'src/runtime.cpp',
            'src/scriptstore.cpp',
            'src/tcp.cpp',
            'src/utils.cpp',
            'src/v8_buffer.cpp',
            'src/v8_callback.cpp',
            'src/v8_core.cpp',
            'src/v8_functions.cpp',
            'src/v8_main_loop.cpp',
            'src/v8_require.cpp',
            'src/v8_timers.cpp',
            'src/v8_uuid.cpp',
            'src/xml.cpp',

            # HTTP files
            'src/http.cpp',
            'src/http_client.cpp',
            'src/http_server.cpp',
            'src/http_logger.cpp',
            'src/url.cpp',
            'src/websocket.cpp',
            'src/websocket_hixie_76.cpp',
            'src/websocket_hybi_17.cpp',
            'src/FileService.cpp',
        ],
        'library_files': [
            'rte/libraries/colors.js',
            'rte/libraries/console.js',
            'rte/libraries/crypto.js',
            'rte/libraries/db.js',
            'rte/libraries/events.js',
            'rte/libraries/fs.js',
            'rte/libraries/http.js',
            'rte/libraries/json.js',
            'rte/libraries/log.js',
            'rte/libraries/process.js',
            'rte/libraries/rte.js',
            'rte/libraries/underscore.js',
            'rte/libraries/util.js',
            'rte/libraries/uuid.js',
            'rte/libraries/xml.js',
        ],
        'conditions': [
            ['OS=="win"', {
                'include_files': [
                    'include/jsx/firewall.hpp',
                    'include/jsx/registry.hpp',
                ],
                'source_files': [
                    'src/firewall.cpp',
                    'src/netinfo.cpp',
                    'src/registry.cpp',
                    'src/win32_minidump.cpp',
                ],
                'library_files': [
                    'rte/libraries/netinfo.js',
                    'rte/libraries/registry.js',
                ],
            }],
        ],
    },
}