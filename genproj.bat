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

python --version 2>nul
if %ERRORLEVEL% == 0 goto python_ok

set PYTHON_PATH=c:\python27
set PATH=%PYTHON_PATH%;%PATH%
python --version 2>nul
if %ERRORLEVEL% == 0 goto python_ok

echo Python not found in %PYTHON_PATH% - Unable to continue
goto end

:python_ok

set GYPFILE=%1
if not defined GYPFILE (
	set GYPFILE=jsx.gyp
)

set JSX_ROOT=%2
if not defined JSX_ROOT (
	set JSX_ROOT=%CD%
)

python build.py --gyp-file="%GYPFILE%" --jsx-root="%JSX_ROOT%" --no-build

:end
popd
endlocal
