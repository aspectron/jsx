//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CORE_API_HPP_INCLUDED
#define JSX_CORE_API_HPP_INCLUDED

// CORE_API definition

#include "jsx/platform.hpp"

#if OS(WINDOWS)
	#pragma warning ( disable : 4251 )
	#if defined(CORE_EXPORTS)
	#define CORE_API __declspec(dllexport)
	#else
	#define CORE_API __declspec(dllimport)
	#endif
#elif __GNUC__ >= 4
# define CORE_API __attribute__((visibility("default")))
#else
#define CORE_API // nothing, symbols in a shared library are exported by default
#endif

//#define USING_V8_SHARED 1

#endif // JSX_CORE_API_HPP_INCLUDED
