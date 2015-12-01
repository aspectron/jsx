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

#if ENABLE(DIRECTORY_MONITOR)

#include "jsx/fs.hpp"
#include "jsx/directory_monitor.hpp"

#include "jsx/runtime.hpp"
#include "jsx/v8_main_loop.hpp"

namespace aspect { namespace fs {

using boost::system::error_code;
using boost::asio::dir_monitor_event;

#if OS(WINDOWS)
static bool enable_privilege(wchar_t const* privilege, bool enable)
{
	bool ret = false;
	HANDLE token;
	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token) )
	{
		TOKEN_PRIVILEGES tp = { 1 };
		if ( LookupPrivilegeValue(NULL, privilege,  &tp.Privileges[0].Luid) )
		{
			tp.Privileges[0].Attributes = (enable ?  SE_PRIVILEGE_ENABLED : 0);
			AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
			ret = (GetLastError() == ERROR_SUCCESS);
		}
		CloseHandle(token);
	}
	return ret;
}

static void setup_privileges(runtime const& rt)
{
	wchar_t const* const arPrivelegeNames[] =
	{
		SE_BACKUP_NAME,        // these two are required for the FILE_FLAG_BACKUP_SEMANTICS flag used in the call to
		SE_RESTORE_NAME,       // CreateFile() to open the directory handle for ReadDirectoryChangesW
		SE_CHANGE_NOTIFY_NAME  // just to make sure...it's on by default for all users.
		//<others here as needed>
	};

	for (int i = 0; i < sizeof(arPrivelegeNames) / sizeof(arPrivelegeNames[0]); ++i)
	{
		if( !enable_privilege(arPrivelegeNames[i], true) )
		{
			rt.error("Unable to enable privilege %S - file change monitoring may not work correctly\n", arPrivelegeNames[i]);
		}
	}
}

#else

void setup_privileges(runtime const&)
{
}

#endif

directory_monitor::directory_monitor(v8::FunctionCallbackInfo<v8::Value> const& args)
	: impl_(runtime::instance(args.GetIsolate()).io_service())
	, is_stopped_(false)
{
	setup_privileges(runtime::instance(args.GetIsolate()));
}

void directory_monitor::add_dir(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8_path dirname(isolate, args[0]);
	if (dirname.empty())
	{
		throw std::runtime_error("require string or path");
	}
	dirname = v8_path(boost::filesystem::canonical(dirname));
	dirname = v8_path(boost::filesystem::absolute(dirname));

	bool watch_subdirs = true;
	if (args.Length() > 1)
	{
		if (!args[1]->IsBoolean())
		{
			throw std::invalid_argument("expecting boolean argument");
		}
		watch_subdirs = v8pp::from_v8<bool>(isolate, args[1]);
	}

	path_regex filter;
	if (args.Length() > 2)
	{
		if (!args[2]->IsString())
		{
			throw std::invalid_argument("expecting string argument");
		}
#if OS(WINDOWS)
		v8::String::Value const str(args[2]);
#else
		v8::String::Utf8Value const str(args[2]);
#endif
		filter = reinterpret_cast<path_regex::value_type const*>(*str);
	}

	impl_.add_directory(dirname, watch_subdirs);
	if (!filter.empty())
	{
		filters_[dirname] = filter;
	}

	args.GetReturnValue().Set(v8pp::to_v8(isolate, this));
}

void directory_monitor::remove_dir(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8_path const dirname(isolate, args[0]);
	if (dirname.empty())
	{
		throw std::runtime_error("require string or path");
	}

	impl_.remove_directory(dirname);
	filters_.erase(dirname);

	args.GetReturnValue().Set(v8pp::to_v8(isolate, this));
}

void directory_monitor::dirs(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set(v8pp::to_v8(isolate, impl_.directories()));
}

static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, dir_monitor_event const& ev)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> result = v8::Object::New(isolate);
	set_option(isolate, result, "type", ev.type);
	set_option(isolate, result, "path", v8_path::to_v8(isolate, ev.path));

	return scope.Escape(result);
}

void directory_monitor::monitor(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	is_stopped_ = false;

	if (!args[0]->IsFunction())
	{
		dir_monitor_event ev;
		do
		{
			ev = impl_.monitor();
		} while (!filter(ev));

		args.GetReturnValue().Set(to_v8(isolate, ev));
	}
	else
	{
		impl_.async_monitor(boost::bind(&directory_monitor::on_event, this,
			boost::make_shared<v8_core::callback>(args, 0), _1, _2));
	}
}

void directory_monitor::stop()
{
	is_stopped_ = true;
}

bool directory_monitor::filter(boost::asio::dir_monitor_event const& ev) const
{
	if (ev.type == dir_monitor_event::null)
	{
		// special case for sync. monitor termination
		return true;
	}

	filters::const_iterator it = filters_.find(ev.path.parent_path());
	return it == filters_.end()
		|| boost::regex_match(ev.path.filename().native(), it->second);
}

void directory_monitor::on_event(boost::shared_ptr<v8_core::callback> cb, boost::system::error_code const& err, boost::asio::dir_monitor_event const& ev)
{
	if (is_stopped_)
	{
		return;
	}

	if (err || filter(ev))
	{
		runtime& rt = runtime::instance(cb->isolate);
		if (!rt.is_terminating())
		{
			rt.main_loop().schedule(boost::bind(&directory_monitor::on_event_v8, cb, err, ev));
		}
	}

	if (!err)
	{
		impl_.async_monitor(boost::bind(&directory_monitor::on_event, this, cb, _1, _2));
	}
}

void directory_monitor::on_event_v8(boost::shared_ptr<v8_core::callback> cb, boost::system::error_code err, boost::asio::dir_monitor_event ev)
{
	if (cb->fn.IsEmpty())
	{
		if (err)
		{
			runtime::instance(cb->isolate).error("%s", err.message().c_str());
		}
		return;
	}

	v8::HandleScope scope(cb->isolate);

	int argc = 0;
	v8::Handle<v8::Value> argv[2];
	if (err)
	{
		argc = 1;
		argv[0] = v8pp::to_v8(cb->isolate, err.message());
	}
	else
	{
		argc = 2;
		argv[0] = v8::Undefined(cb->isolate);
		argv[1] = to_v8(cb->isolate, ev);
	}

	cb->call(argc, argv);
}

}} // aspect::fs

#endif // ENABLE_DIRECTORY_MONITOR
