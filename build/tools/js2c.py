#!/usr/bin/env python
#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https:#github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#

# Generate header file with native library sources
# from a list of JavaScript sources. Usage:
#     js2c.py target.h source1.js source2.js ... sourceN.js

# Store source data as array of chars instead of pointer to string literal
# because of compiler-specific limits (65535 in Visual C++) for the string literal length

import os
import re
import sys

SOURCE_TEMPLATE = 'static char const %(name)s_source[] = { %(data)s };'
LIBRARY_TEMPLATE = '\t{ "%(name)s", %(name)s_source, sizeof(%(name)s_source) },'

HEADER_TEMPLATE = '''\
#ifndef %(include_guard)s
#define %(include_guard)s

namespace aspect { namespace v8_core {

#pragma warning(push, 0)
%(sources)s
#pragma warning(pop)

struct native_library
{
	char const* name;
	char const* source;
	size_t length;
};

static const native_library native_libraries[] =
{
%(libraries)s
};

}} // aspect::v8_core

#endif // %(include_guard)s
'''


def js2c(target_name, source_names):
    sources = []
    libraries = []

    for name in source_names:
        lines = open(name, 'r').read()
        name = os.path.basename(name).split('.')[0]
        data = [str(ord(chr)) for chr in lines]
        data = ', '.join(data)
        variables = dict(name=name, data=data)
        sources.append(SOURCE_TEMPLATE % variables)
        libraries.append(LIBRARY_TEMPLATE % variables)

    output = open(target, 'w')
    output.write(HEADER_TEMPLATE % {
        'include_guard': re.sub('\W', '_', target).upper(),
        'sources': '\n'.join(sources),
        'libraries': '\n'.join(libraries),
    })
    output.close()


if __name__ == "__main__":
    target = sys.argv[1]
    sources = sys.argv[2:]
    js2c(target, sources)
