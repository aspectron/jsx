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

if "%JSX_ROOT%"=="" set JSX_ROOT=%CD%\..

set SYMBOLS_SERVER=c:\Projects\zymbols\jsx

set symstore="c:\Program Files (x86)\Windows Kits\8.1\Debuggers\x86\symstore.exe"

if exist %symstore% (
  %symstore% add /f %JSX_ROOT%\bin /r /s %SYMBOLS_SERVER% /t jsx /c "%1" /v "%2"
) else (
  echo symstore.exe not found, please install Debugging tools for Windows.
  echo http://msdn.microsoft.com/library/windows/hardware/ff551063.aspx
)

popd
endlocal