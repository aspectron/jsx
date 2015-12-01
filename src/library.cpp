//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#include "jsx/core.hpp"
#include "jsx/library.hpp"

#include "jsx/os.hpp"

#if OS(UNIX)
#include <dlfcn.h>
#endif

namespace aspect {

#if OS(WINDOWS)

void library::load(std::string const& name)
{
	unload();

	boost::filesystem::path fullname = os::exe_path().parent_path() / name;
	fullname.replace_extension(".dll");

	UINT const prev_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	handle_  = LoadLibraryW(fullname.native().c_str());
	SetErrorMode(prev_error_mode);

	if (!handle_)
	{
		using namespace boost::system;
		throw system_error(GetLastError(), get_system_category(),
			"Unable to load library \'" + name + "\'");
	}
	name_ = name;
}

void library::unload()
{
	if (handle_ && !name_.empty())
	{
		FreeLibrary(handle_);

		handle_ = 0;
		name_.clear();
	}
}

void* library::symbol(char const* symname) const
{
	return GetProcAddress(handle_, symname);
}

#else

void library::load(std::string const& name)
{
	unload();

	// TODO - check for file existence
	boost::filesystem::path fullname = os::exe_path().parent_path() / ("lib" + name);
#if OS(DARWIN)
	fullname.replace_extension(".dylib");
#else
	fullname.replace_extension(".so");
#endif

	handle_ = dlopen(fullname.c_str(), RTLD_LAZY);
	if (!handle_)
	{
		throw std::runtime_error("Unable to load library \'" + name + "\': " + dlerror());
	}
	name_ = name;
}

void library::unload()
{
	if (handle_ && !name_.empty())
	{
		::dlclose(handle_);

		handle_ = 0;
		name_.clear();
	}
}

void* library::symbol(char const* symname) const
{
	return dlsym(handle_, symname);
}

#endif

typedef v8::Handle<v8::Value> (C_CALL *LIBRARY_STARTUP)(v8::Isolate* isolate);
typedef void (C_CALL *LIBRARY_SHUTDOWN)(v8::Isolate* isolate, v8::Handle<v8::Value>);

v8::Handle<v8::Value> library::startup(v8::Isolate* isolate)
{
	LIBRARY_STARTUP startup_fun = (LIBRARY_STARTUP)symbol("library_startup");
	if(!startup_fun)
	{
		throw std::runtime_error(name_ + " - library startup failed");
	}
	return startup_fun(isolate);
}

void library::shutdown(v8::Isolate* isolate, v8::Handle<v8::Value> lib)
{
	LIBRARY_SHUTDOWN shutdown_fun = (LIBRARY_SHUTDOWN)symbol("library_shutdown");
	if(!shutdown_fun)
	{
		throw std::runtime_error(name_ + " - library shutdown failed");
	}
	shutdown_fun(isolate, lib);
}

} // aspect
