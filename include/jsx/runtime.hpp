//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_RUNTIME_HPP_INCLUDED
#define JSX_RUNTIME_HPP_INCLUDED

#include "jsx/log.hpp"
#include "jsx/os.hpp"
#include "jsx/utils.hpp"

#include <boost/asio/ip/tcp.hpp>

namespace aspect {

class logger;
class io_thread_pool;
class async_queue;

namespace v8_core { class core; class main_loop; }

/// JSX Runtime
class CORE_API runtime : boost::noncopyable
{
public:
	/// Create a JSX runtime
	runtime(int argc = 0, char* argv[] = nullptr, unsigned io_threads = 0, unsigned queue_threads = 0);

	~runtime();

	typedef std::vector<std::string> arguments;

	arguments const& args() const { return args_; }

	/// Get app root path (os::exe_path().parent_path() without debug/release)
	static boost::filesystem::path root_path();

	struct debug_info
	{
		debug_info()
			: enabled(false)
			, module("jsx")
			, wait(false)
			, port(9222)
		{
#if FORCE_V8_DEBUGGER
			enabled = true;
#endif
		}

		bool enabled;
		std::string module;
		bool wait;
		uint16_t port;
	};

	struct options
	{
		boost::filesystem::path rte;
		std::string uuid;

		debug_info debug;

		options()
			: rte("rte")
			, uuid("")
			, debug()
		{
		}
	};

	std::string const& uuid() const { return uuid_; }
	void set_uuid(std::string const& value) { uuid_ = value; }

	boost::filesystem::path const& script() const { return script_; }

	arguments const& script_args() const { return script_args_; }

	boost::filesystem::path const& rte_path() const { return rte_path_; }

	static char const* cfg_file() { return "rte.cfg"; }

	typedef std::vector<boost::filesystem::path> rte_include_paths;

	void reset_rte_include_paths() { rte_include_paths_.clear(); }
	void add_rte_include_path(boost::filesystem::path const& path) { rte_include_paths_.push_back(os::resolve(path)); }
	rte_include_paths const& get_rte_include_paths() const { return rte_include_paths_; }

	void set_node_lib_path(boost::filesystem::path const& path);

	int run(boost::filesystem::path const& script,
		arguments const& script_args = arguments(), options const& runtime_options = options());

	void terminate(int exit_code = 0);
	bool is_terminating() const;

	enum execution_options
	{
		EVENT_QUEUE     = 0x00000001,
		GC_NOTIFY       = 0x00000002,
		PRIVATE_CONTEXT = 0x00000004,
		SILENT          = 0x00000008,
	};

	void set_execution_options(uint32_t opt) { execution_options_ = opt; }
	uint32_t execution_options() const { return execution_options_; }

	boost::shared_ptr<aspect::logger> get_logger() { return logger_; }

	io_thread_pool& io_pool()
	{
		_aspect_assert(io_pool_);
		return *io_pool_;
	}

	boost::asio::ip::tcp::resolver& tcp_resolver()
	{
		_aspect_assert(tcp_resolver_);
		return *tcp_resolver_;
	}

	async_queue& event_queue()
	{
		_aspect_assert(event_queue_);
		return *event_queue_;
	}

	v8_core::core& core()
	{
		_aspect_assert(core_);
		return *core_;
	}

	v8_core::main_loop& main_loop()
	{
		_aspect_assert(main_loop_);
		return *main_loop_;
	}

	boost::asio::io_service& io_service();

	v8::Isolate* isolate() const { return isolate_; }

	static runtime& instance(v8::Isolate* isolate);

	void trace(const char* fmt, ...) const;
	void debug(const char* fmt, ...) const;
	void error(const char* fmt, ...) const;
	void warning(const char* fmt, ...) const;

private:
	void done();
	void run_v8(options const& opts);

	void signal_handler(boost::system::error_code const& ec, int signal_number);
	void console_output_impl(uint32_t type, char const* fmt, va_list args) const;

	v8::Isolate* isolate_;

	arguments args_;
	bool is_running_;
	int exit_code_;

	std::string uuid_;

	uint32_t execution_options_;

	boost::filesystem::path script_;
	arguments script_args_;
	boost::filesystem::path rte_path_;
	rte_include_paths rte_include_paths_;

	boost::scoped_ptr<io_thread_pool> io_pool_;
	boost::scoped_ptr<boost::asio::ip::tcp::resolver> tcp_resolver_;
	boost::scoped_ptr<async_queue> event_queue_;
	boost::scoped_ptr<v8_core::core> core_;
	boost::scoped_ptr<v8_core::main_loop> main_loop_;

	boost::shared_ptr<aspect::logger> logger_;

	boost::scoped_ptr<boost::asio::signal_set> signals_;
};

} // aspect

#endif // JSX_RUNTIME_HPP_INCLUDED
