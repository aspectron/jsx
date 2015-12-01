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
            'target_name': 'build-all',
            'type': 'none',
            'dependencies': [
                'jsx-app.gyp:*',
                'jsx-lib.gyp:*',
                'jsx-doc.gyp:*',
            ],
        },
    ],
}
