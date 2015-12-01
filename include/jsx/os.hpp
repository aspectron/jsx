//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_OS_HPP_INCLUDED
#define JSX_OS_HPP_INCLUDED

#include "jsx/v8_core.hpp"

namespace aspect { namespace os {

void setup_bindings(v8pp::module& target);

#if OS(WINDOWS)
/// Get error message from Win32 error code
CORE_API std::string win32_error_message(int code);

/// Read registry value at root\key_name\value_name
CORE_API std::string read_reg_value(HKEY root, char const* key_name, char const* value_name);

/// Set exception handlers for calling thread
CORE_API void set_thread_exception_handlers();

/// Set global process exception handler to generate minidump on application
/// crash in a %TEMP%\app_name-crash.dmp file. If app_name is nullptr .exe name
/// will be used. Optional report_contact is used to specify a contact used
/// to send the report.
CORE_API void set_process_exception_handler(wchar_t const* app_name = nullptr, wchar_t const* report_contact = nullptr);
#endif

/// Computer name
CORE_API std::string computer_name();

/// Fully qualified domain name
CORE_API std::string fqdn();

/// OS version string
CORE_API std::string version();

CORE_API void set_thread_name(char const* name);

/// Get executable path
CORE_API boost::filesystem::path exe_path();

/// Resolve path
CORE_API boost::filesystem::path resolve(boost::filesystem::path const& p);

}} // aspect::os

#endif //JSX_OS_HPP_INCLUDED
