#!/usr/bin/env python
#
# Copyright (c) 2011 - 2015 ASPECTRON Inc.
# All Rights Reserved.
#
# This file is part of JSX (https://github.com/aspectron/jsx) project.
#
# Distributed under the MIT software license, see the accompanying
# file LICENSE
#

"""
build script with python for JSX project
should detect the current system and environment and fully automate the build process
"""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
EXTERN_DIR = os.path.join(SCRIPT_DIR, 'extern')

sys.path.insert(0, EXTERN_DIR)
from build_utils import *


def build_extern(args):
    EXTERN_PROJECTS = ['build_boost.py', 'build_v8.py', 'build_openssl.py']

    cwd = os.getcwd()
    os.chdir(EXTERN_DIR)

    command = '{python} {project}'

    if args.config:
        command += ' --config=' + args.config
    if args.platform:
        command += ' --platform=' + args.platform
    if args.force_external:
        command += ' --force'
    if sys.platform == 'win32' and args.msvc:
        command += ' --msvc=' + args.msvc

    for project in EXTERN_PROJECTS:
        execute_sync(command.format(python=sys.executable, project=project))

    os.chdir(cwd)


def main():
    args = parse_args()
    if not args.gyp_file:
        args.gyp_file = 'jsx.gyp'
    if not args.jsx_root:
        args.jsx_root = SCRIPT_DIR

#    build_extern(args)
    build_project(args)


if __name__ == '__main__':
    main()
