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
import re
import shutil
from build_utils import *

WITHOUT_LIBS = [
    'context',
    'coroutine',
    'graph',
    'graph_parallel',
    'locale',
    'log',
    'math',
    'mpi',
    'python',
    'serialization',
    'signals',
    'test',
    'timer',
    'wave',
]

def cleanup(args):
    if os.path.isdir(args.include_dir):
        shutil.rmtree(args.include_dir)
    if os.path.isdir(args.stage_dir):
        shutil.rmtree(args.stage_dir)


def bjam_command(bjam, bjam_args, stage_dir, zlib_src):
    without_libs = ['--without-' + lib for lib in WITHOUT_LIBS]
    command = '{bjam} headers && {bjam} -j{cpu_count} --stagedir={stage_dir} {bjam_args} '\
                 '-sNO_BZIP2=1 -sZLIB_SOURCE={zlib_src} {without_libs} '\
                 'link=static threading=multi release stage'\
        .format(bjam=bjam, cpu_count=cpu_count(), stage_dir=stage_dir, zlib_src=zlib_src,
            bjam_args=bjam_args, without_libs=' '.join(without_libs))
    return command


def build_posix(args):
    done_file = os.path.join(args.stage_dir, 'lib', 'build.done')
    if os.path.isfile(done_file):
        return

    if not os.path.isfile('b2'):
        execute_sync('./bootstrap.sh')

    bjam_args = 'cxxflags=-fPIC'
    command = bjam_command('./b2', bjam_args, args.stage_dir, args.zlib_src)
    execute_sync(command)
    touch(done_file)


def build_win32(args):
    if not os.path.isfile('b2.exe'):
        execute_sync_in_msvc_env(args, 'bootstrap.bat')
    execute_sync_in_msvc_env(args, 'b2.exe headers')
    platforms = [args.platform] if args.platform else ['x86', 'x64']
    for platform in platforms:
        args.platform = platform
        build_variant(args)


def build_variant(args):
    stage_dir = os.path.join(args.stage_dir, args.platform)
    done_file = os.path.join(stage_dir, 'lib', 'build.done')
    if os.path.isfile(done_file):
        return

    bjam_args = 'debug toolset=msvc-{msvc} address-model={address_model}'\
        .format(msvc=args.msvc, address_model=64 if args.platform=='x64' else 32)
    command = bjam_command('b2.exe', bjam_args, stage_dir, args.zlib_src)
    execute_sync_in_msvc_env(args, command)
    touch(done_file)

def main():
    args = parse_args()

    cwd = os.getcwd()
    if os.path.basename(cwd) != 'extern':
        os.chdir('extern')

    if args.force or args.force_external:
        cleanup(args)

    os.chdir('boost')
    args.prefix = os.getcwd()
    args.include_dir = os.path.join(args.prefix, 'boost')
    args.stage_dir = os.path.join(args.prefix, 'stage')
    args.zlib_src =  os.path.join(args.prefix, os.pardir, 'zlib')

    if sys.platform.startswith('win'):
        build_win32(args)
    else:
        build_posix(args)

    if cwd != os.getcwd():
        os.chdir(cwd)


if __name__ == '__main__':
    main()
