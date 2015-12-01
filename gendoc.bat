@echo off
rem
rem Copyright (c) 2011 - 2015 ASPECTRON Inc.
rem All Rights Reserved.
rem
rem This file is part of JSX (https://github.com/aspectron/jsx) project.
rem
rem Distributed under the MIT software license, see the accompanying
rem file LICENSE
rem
setlocal
pushd %~dp0

if not defined JSX_ROOT set JSX_ROOT=%CD%

set JSX="%JSX_ROOT%\bin\Release Win32\jsx.exe"
if not exist %JSX% (set JSX="%JSX_ROOT%\bin\Release x64\jsx.exe")

set HLSTYLE=default.css

set JS_SRC_DIR=%1
set CPP_SRC_DIR=%2
set DOCDIR=%3

if not defined JS_SRC_DIR set JS_SRC_DIR=%JSX_ROOT%\rte\libraries
if not defined CPP_SRC_DIR set CPP_SRC_DIR=%JSX_ROOT%\src
if not defined DOCDIR set DOCDIR=%JSX_ROOT%\doc\sdk

set JS_SRC_FILTER=%JS_SRC_DIR%\.+[.]js$
set CPP_SRC_FILTER=%CPP_SRC_DIR%\.+[.]cpp$

%JSX% build\tools\gendoc\run.js %DOCDIR% %JS_SRC_FILTER% %CPP_SRC_FILTER%

doxygen

popd
endlocal