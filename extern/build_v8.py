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
import os
import sys
import argparse
import itertools
from build_utils import *


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

def cleanup(args):
    shutil.rmtree(os.path.join(args.build_dir, 'build', 'Debug'))
    shutil.rmtree(os.path.join(args.build_dir, 'build', 'Release'))
    shutil.rmtree(args.lib_dir)


def build_posix(args):
    cwd = os.getcwd()

    os.chdir(args.build_dir)
    if not os.path.exists('build/gyp'):
        os.symlink('../../../build/tools/gyp', 'build/gyp')

    env = dict(os.environ, CXXFLAGS='-Wno-error')
    if sys.platform.startswith('linux'):
        env['LDFLAGS'] = '-Wl,-rpath=\$$ORIGIN'
    elif sys.platform.startswith('darwin'):
        env['DYLD_LIBRARY_PATH'] = os.path.join(os.getcwd(), 'out/native')

    execute_sync('make -j{num_cpu} native library=shared werror=no' \
        .format(num_cpu=cpu_count()), env)

    if sys.platform.startswith('darwin'):
        change_id = 'install_name_tool -id @executable_path/lib{name}.dylib out/native/lib{name}.dylib'
        change_lib = 'install_name_tool -change /usr/local/lib/lib{name}.dylib @executable_path/lib{name}.dylib out/native/lib{target}.dylib'
        for lib in ['v8', 'icui18n', 'icuuc']:
            execute_sync(change_id.format(name=lib))
            execute_sync(change_lib.format(name='icui18n', target=lib))
            execute_sync(change_lib.format(name='icuuc', target=lib))

    if os.path.isdir('out/native/lib.target'):
        copytree('out/native/lib.target/lib*',  args.lib_dir)
    else:
        copytree('out/native/lib*', args.lib_dir)

    os.chdir(cwd)


def build_win32(args):
    os.environ['PATH'] = os.path.dirname(sys.executable) + os.pathsep + os.environ.get('PATH', '')
    platforms = [args.platform] if args.platform else ['x86', 'x64']
    configs = [args.config] if args.config else ['Debug', 'Release']
    for platform in platforms:
        for config in configs:
            args.platform = platform
            args.config = config
            build_variant(args)


def build_variant(args):
    dest_dir = os.path.join(args.lib_dir, args.platform, args.config)

    print 'Building v8 {} {}'.format(args.platform, args.config)

    gyp_command = '"{python}" {gyp_cmd} -I{nocygwin} '\
        '-Dtarget_arch={gyp_arch} -Dcomponent=shared_library --format={format}'\
        .format(python=sys.executable,
            gyp_cmd=os.path.join(args.build_dir, 'build', 'gyp_v8.py'),
            nocygwin=os.path.join(os.path.relpath(SCRIPT_DIR), 'nocygwin.gypi'),
            gyp_arch='ia32' if args.platform=='x86' else args.platform,
            format = gyp_format(args))
    print gyp_command

    gyp_dir = os.path.join(SCRIPT_DIR, os.pardir, 'build', 'tools', 'gyp', 'pylib')
    env = dict(os.environ, PYTHONPATH=gyp_dir + os.pathsep + os.environ.get('PYTHONPATH', ''))
    execute_sync(gyp_command, env)

    project = os.path.join(args.build_dir, 'tools', 'gyp', 'v8.vcxproj')
    msbuild_project(args, project)

    if os.path.isdir(dest_dir):
        shutil.rmtree(dest_dir)

    src_dir = os.path.join(args.build_dir, 'build', args.config, 'lib', 'v8.*')
    copytree(src_dir, dest_dir)

    src_dir = os.path.join(args.build_dir, 'build', args.config, 'v8.*')
    copytree(src_dir, dest_dir)

    src_dir = os.path.join(args.build_dir, 'build', args.config, 'icu*')
    copytree(src_dir, dest_dir)


def main():
    args = parse_args()

    cwd = os.getcwd()
    if os.path.basename(cwd) != 'extern':
        os.chdir('extern')

    args.build_dir = os.path.join(os.getcwd(), 'v8')
    args.include_dir = os.path.join(args.build_dir, 'include')
    args.lib_dir = os.path.join(args.build_dir, 'lib')

    if args.force or args.force_external:
        cleanup(args)

    if sys.platform.startswith('win'):
        build_win32(args)
    else:
        build_posix(args)

    if cwd != os.getcwd():
        os.chdir(cwd)

if __name__ == '__main__':
    main()
