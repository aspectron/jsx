//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CONSOLE_HPP_INCLUDED
#define JSX_CONSOLE_HPP_INCLUDED

#include "jsx/platform.hpp"

namespace v8pp { class module; }

namespace aspect {

/// Console functions
namespace console {

void setup_bindings(v8pp::module& bindings);

/// Supported console output colors
enum color { BLACK = 0, RED = 1, GREEN = 2, YELLOW = 3, BLUE = 4,
	MAGENTA = 5, CYAN = 6, WHITE = 7, MAX_COLORS = 8, DEFAULT_COLOR = 9 };

/// Supported console output attributes
enum attribute { RESET = 0, BRIGHT = 1, DIM = 2, UNDERLINE = 3,
	BLINK = 4, REVERESE = 7, HIDDEN = 8 };

/// Print line with specified colors and attributes
CORE_API void println(char const* line,
	color fg = DEFAULT_COLOR, color bg = DEFAULT_COLOR, attribute attr = RESET);

#if OS(WINDOWS)

/// Write a string into a windows console
/// with restricted VT100 control codes support
void vt100_write(wchar_t const* str, HANDLE console);

/// Get console title
CORE_API std::wstring get_title();

/// Set console title
CORE_API void set_title(std::wstring const& title);

#else

/// Get console title
CORE_API std::string get_title();

/// Set console title
CORE_API void set_title(std::string const& title);

#endif

}} // aspect::console

#endif // JSX_CONSOLE_HPP_INCLUDED
