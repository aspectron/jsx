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
#include "jsx/runtime.hpp"

#include <stdarg.h>

#include "jsx/async_queue.hpp"
#include "jsx/v8_main_loop.hpp"
#include "jsx/cpus.hpp"

#if OS(DARWIN)
extern "C" {
void cocoa_run();
void cocoa_stop();
}
#endif

namespace aspect {

static boost::recursive_mutex rt_mutex_;
static boost::unordered_map<v8::Isolate*, runtime*> rt_instances_;

runtime& runtime::instance(v8::Isolate* isolate)
{
	boost::recursive_mutex::scoped_lock lock(rt_mutex_);

	boost::unordered_map<v8::Isolate*, runtime*>::const_iterator it =  rt_instances_.find(isolate);
	if (it == rt_instances_.end())
	{
		_aspect_assert(false && "no runtime instance for isolate");
		throw std::runtime_error("no runtime instance for isolate");
	}
	return *it->second;
}

runtime::runtime(int argc, char* argv[], unsigned io_threads, unsigned queue_threads)
	: is_running_(false)
	, exit_code_(0)
	, execution_options_(0)
	, rte_path_("rte")
{
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

	// assign remained args
	args_.assign(argv, argv + argc);

	unsigned const num_cores = cpu_count();

	queue_threads = std::min(queue_threads == 0? 2 : queue_threads, num_cores * 2);
	event_queue_.reset(new async_queue("runtime event queue", queue_threads));

	io_threads = std::min(io_threads == 0? num_cores : io_threads, num_cores * 2);
	io_pool_.reset(new io_thread_pool("runtime IO", io_threads));
	tcp_resolver_.reset(new boost::asio::ip::tcp::resolver(io_pool_->io_service()));

	logger_ = boost::make_shared<logger>(*this);

#if 0 //ZZZ: OS(WINDOWS)
	// TODO - PAVEL - MAKE THIS PLATFORM INDEPENDENT
	// REFER TO process.hpp library header function signal_named_condition()
	// http://www.boost.org/doc/libs/1_36_0/doc/html/boost/interprocess/named_condition.html
	unsigned long pid = ::GetCurrentProcessId();
	char buffer[MAX_PATH];
	sprintf_s(buffer,sizeof(buffer),"HX-%d",pid);
	termination_event_ = CreateEventA(NULL,FALSE,FALSE,buffer);
#endif

	signals_.reset(new boost::asio::signal_set(io_service(), SIGINT));
	signals_->async_wait(boost::bind(&runtime::signal_handler, this,
		boost::asio::placeholders::error, boost::asio::placeholders::signal_number));
}

runtime::~runtime()
{
}

void runtime::done()
{
#if OS(WINDOWS)
//ZZZ:	CloseHandle(termination_event_);
#endif

	logger_.reset();

#if HAVE(LIBUUH)
//	data::release_io_service();
#endif

	// Delete async event queue and I/O thread pool to stop them
	tcp_resolver_.reset();
	io_pool_.reset();
	event_queue_.reset();
}

void runtime::run_v8(options const& opts)
{
	_aspect_assert(!core_);
	_aspect_assert(!main_loop_);

	{
		boost::recursive_mutex::scoped_lock lock(rt_mutex_);

		if (rt_instances_.empty())
		{
			v8::V8::InitializeICU();
		}

		isolate_ = v8::Isolate::New();
		rt_instances_.insert(std::make_pair(isolate_, this));
	}

	{
		v8::Isolate::Scope isolate_scope(isolate_);
		v8::HandleScope scope(isolate_);

		main_loop_.reset(new v8_core::main_loop(*this));
		core_.reset(new v8_core::core(*this));
		core_->init();
#if USING(V8_DEBUGGER)
		if (opts.debug.enabled)
		{
			core_->enable_debug(opts.debug.module, opts.debug.port, opts.debug.wait);
		}
#endif

		try
		{
			core_->run_script_file(script_);
		}
		catch (std::exception const& ex)
		{
			error("Exception has occurred during startup script execution in script: %s\n",script_.string().c_str());
			error("%s\n", ex.what());
		}

		main_loop_->run();

		core_.reset();
		main_loop_.reset();
	}

	{
		boost::recursive_mutex::scoped_lock lock(rt_mutex_);
		rt_instances_.erase(isolate_);
	}

	isolate_->Dispose();
	isolate_ = nullptr;

#if OS(DARWIN)
	boost::recursive_mutex::scoped_lock lock(rt_mutex_);
	if (rt_instances_.empty())
	{
		cocoa_stop();
	}
#endif
}

void runtime::signal_handler(boost::system::error_code const& ec, int)
{
	if (ec)
	{
		return;
	}

	if ( is_terminating() )
	{
		trace("-- SIGHANDLER - %s\n", script_.c_str());
		trace("again? aborting...\n\n");
		exit(1);
	}

	trace("-- SIGHANDLER - %s\nRequesting module shutdown.\nCTRL+C again to abort.\n",script_.string().c_str());
	terminate();
}


int runtime::run(boost::filesystem::path const& script,
		arguments const& script_args, options const& opts)
{
	uuid_ = opts.uuid;
	rte_path_ = opts.rte;
	if ( rte_path_.is_relative() )
	{
		rte_path_ = root_path() / rte_path_;
	}
	rte_path_.make_preferred();

	script_ = script;
	script_args_ = script_args;
	execution_options_ = 0;

	if (script_.is_relative())
	{
		if (!boost::filesystem::is_regular_file(script_))
		{
			script_ = rte_path_ / script;
			if (!boost::filesystem::is_regular_file(script_))
			{
				script_ = boost::filesystem::current_path() / script;
			}
			if (!boost::filesystem::is_regular_file(script_))
			{
				throw std::runtime_error("unable to find " + script.string());
			}
		}
		script_ = boost::filesystem::canonical(script_);
	}
	script_.make_preferred();

	logger_->set_origin(script_.string());


#if OS(DARWIN)
	// run first runtime instance in a separate thread
	// since main thread must handle UI
	if (rt_instances_.empty())
	{
		boost::thread v8_thread = boost::thread(&runtime::run_v8, this, opts);
		cocoa_run();
		v8_thread.join();
	}
	else
#else
	run_v8(opts);
#endif
	done();
	return exit_code_;
}

void runtime::terminate(int exit_code)
{
	exit_code_ = exit_code;
	if ( main_loop_ )
	{
		main_loop_->terminate();
	}
}

bool runtime::is_terminating() const
{
	return !main_loop_ || main_loop_->is_terminating();
}

boost::asio::io_service& runtime::io_service()
{
	// Use io_service from global io thread pool
	return io_pool().io_service();
}

boost::filesystem::path runtime::root_path()
{
	boost::filesystem::path p = os::exe_path().parent_path();

	while ( !p.empty() )
	{
		std::string const leaf = utils::to_lower(p.leaf().string());

		if ( leaf == "release" || leaf == "debug" ||
			 leaf == "release win32" || leaf == "debug win32" ||
			 leaf == "release x86" || leaf == "debug x86" ||
			 leaf == "release x64" || leaf == "debug x64" ||
			 leaf == "lib" || leaf == "bin" ||
			 leaf == "win32" || leaf == "x64" || leaf == "x86" )
		{
			p.remove_leaf();
		}
		else
		{
			break;
		}
	}
	return p;
}

void runtime::set_node_lib_path(boost::filesystem::path const& path)
{
	core().set_node_lib_path(path);
}

void runtime::console_output_impl(uint32_t type, char const* fmt, va_list args) const
{
	(void)type;

	char szBuff[8192]; // Strings longer than this get truncated
	if (vsnprintf(szBuff, sizeof(szBuff)-3, fmt, args) < 0)
		szBuff[sizeof(szBuff)-1] = '\0';
	szBuff[sizeof(szBuff)-3] = 0;
	//strcat(szBuff,"\n");
#if OS(WINDOWS) && ENABLE(MSVC_OUTPUT)
	::OutputDebugStringA(szBuff);
#endif

#if ENABLE(REDIRECT_OUTPUT_TO_LOGS)
	if(logger_)
	{
		logger_->output(type, szBuff);
	}
#else
	printf("%s", szBuff);
#endif
}

void runtime::trace(char const* fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	console_output_impl(log::LEVEL_TRACE, fmt, args);
	va_end(args);
}

void runtime::debug(char const* fmt, ...) const
{
	(void)fmt;
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	console_output_impl(log::LEVEL_DEBUG, fmt, args);
	va_end(args);
#endif
}

void runtime::warning(char const* fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	console_output_impl(log::LEVEL_WARNING, fmt, args);
	va_end(args);
}

void runtime::error(char const* fmt, ...) const
{
	va_list args;
	va_start(args, fmt);
	console_output_impl(log::LEVEL_ERROR, fmt, args);
	va_end(args);
}

} // aspect
