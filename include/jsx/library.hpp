//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_LIBRARY_HPP_INCLUDED
#define JSX_LIBRARY_HPP_INCLUDED

#include <v8.h>

namespace aspect {

/// Dynamic library (TODO: make moveable in C++11)
class library
{
public:
	explicit library(std::string const& name)
		: name_()
		, handle_()
	{
		load(name);
	}

	void load(std::string const& name);
	void unload();
	void* symbol(char const* symname) const;

	std::string const& name() const { return name_; }

	v8::Handle<v8::Value> startup(v8::Isolate* isolate);
	void shutdown(v8::Isolate* isolate, v8::Handle<v8::Value>);

private:
	std::string name_;
#if OS(WINDOWS)
	HMODULE handle_;
#else
	void* handle_;
#endif
};

} // aspect


#if COMPILER(MSVC)
#define SUPPRES_EXPORT_WARNING __pragma(warning(suppress: 4190))
#else
#define SUPPRES_EXPORT_WARNING // _Pragma("GCC diagnostic ignored \"-Wexport-warning\"")
#endif

#if COMPILER(MSVC)
#define CORE_LIBRARY_EXPORT __declspec(dllexport)
#define C_CALL __cdecl
#else
#define CORE_LIBRARY_EXPORT  __attribute__((__visibility__("default")))
#define C_CALL               // nothing, GCC has C style calling convention by default
#endif

#define DECLARE_LIBRARY_ENTRYPOINTS(__startup__, __shutdown__) \
extern v8::Handle<v8::Value> __startup__(v8::Isolate* isolate); \
extern void __shutdown__(v8::Isolate* isolate, v8::Handle<v8::Value> library); \
extern "C" { \
SUPPRES_EXPORT_WARNING CORE_LIBRARY_EXPORT v8::Handle<v8::Value> C_CALL library_startup(v8::Isolate* isolate) { return __startup__(isolate); } \
SUPPRES_EXPORT_WARNING CORE_LIBRARY_EXPORT void C_CALL library_shutdown(v8::Isolate* isolate, v8::Handle<v8::Value> library) { __shutdown__(isolate, library); } \
}

#endif // JSX_LIBRARY_HPP_INCLUDED
