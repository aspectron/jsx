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
from build_utils import *


def cleanup(args):
    if os.path.isdir(args.include_dir):
        shutil.rmtree(args.include_dir)
    if os.path.isdir(args.lib_dir):
        shutil.rmtree(args.lib_dir)


def make_perl_env():
    if sys.platform.startswith('win'):
        os.environ['PERLIO'] = ':unix:crlf'
        if os.system('where /Q perl.exe') != 0:
            perl_paths = [
                'C:\\Perl', '\\Perl',
                'C:\\Perl64', '\\Perl64',
                os.path.join(os.environ['PROGRAMFILES'], 'Git'),
                os.path.join(os.environ['PROGRAMFILES(X86)'], 'Git'),
            ]
            for path in perl_paths:
                path = os.path.join(path, 'bin')
                if os.path.isfile(os.path.join(path, 'perl.exe')):
                    os.environ['PATH'] = path + os.pathsep + os.environ.get('PATH', '')
                    break
    return os.environ


def build_posix(args):
    execute_sync('{configure} --prefix={prefix} no-shared no-dso no-asm -DPIC -fPIC'\
        .format(configure='./Configure darwin64-x86_64-cc' if sys.platform == 'darwin' else './config',
                prefix=args.prefix))
    execute_sync('make build_libs')
    copytree('lib*.a', args.lib_dir)


def build_win32(args):
    env = make_perl_env()
    platforms = [args.platform] if args.platform else ['x86', 'x64']
    for platform in platforms:
        args.platform = platform
        build_variant(args, env)


def build_variant(args, env):
    dest_dir = os.path.join(args.lib_dir, args.platform)

    print 'Building {} openssl'.format(args.platform)

    if os.path.isdir('tmp32'):
        shutil.rmtree('tmp32')
    if os.path.isdir('out32'):
        shutil.rmtree('out32')

    arch = 'VC-WIN64A' if args.platform == 'x64' else 'VC-WIN32'
    command = 'perl Configure {arch} no-asm enable-static-engine --prefix={prefix} '\
        '& call ms\\do_nt.bat & nmake -f ms\\nt.mak install'\
        .format(arch=arch, prefix=args.prefix)
    execute_sync_in_msvc_env(args, command, env)

    if os.path.isdir(dest_dir):
        shutil.rmtree(dest_dir)

    movetree(os.path.join(args.lib_dir, '*.lib'), dest_dir)
    shutil.copy(os.path.join('tmp32', 'lib.pdb'), dest_dir)


def main():
    args = parse_args()

    cwd = os.getcwd()
    if os.path.basename(cwd) != 'extern':
        os.chdir('extern')

    os.chdir('openssl')
    args.prefix = os.getcwd()
    args.include_dir = os.path.join(args.prefix, 'include')
    args.lib_dir = os.path.join(args.prefix, 'lib')

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
