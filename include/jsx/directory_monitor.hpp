//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_DIRECTORY_MONITOR_HPP_INCLUDED
#define JSX_DIRECTORY_MONITOR_HPP_INCLUDED

#if ENABLE(DIRECTORY_MONITOR)

#include <boost/atomic.hpp>
#include <boost/regex.hpp>
#include <boost/make_shared.hpp>

#include "dir_monitor/src/dir_monitor.hpp"

#include "jsx/v8_callback.hpp"

namespace aspect { namespace fs {

/// Directory monitor
class directory_monitor : boost::noncopyable
{
public:
	explicit directory_monitor(v8::FunctionCallbackInfo<v8::Value> const& args);
	~directory_monitor() { stop(); }

	/// Add directory to watch, dir name is v8_path or UTF-8 encoded string
	void add_dir(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Remove watching directory, dir name is v8_path or UTF-8 encoded string
	void remove_dir(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Get watched directories
	void dirs(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Monitor directories
	void monitor(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Stop directories monitoring
	void stop();

private:
	boost::asio::dir_monitor impl_;
	boost::atomic<bool> is_stopped_;

	typedef boost::basic_regex<boost::filesystem::path::value_type> path_regex;

	typedef std::map<boost::filesystem::path, path_regex> filters;
	filters filters_;

	// check filename filter for directory
	bool filter(boost::asio::dir_monitor_event const& ev) const;

	// async_monitor handler
	void on_event(boost::shared_ptr<v8_core::callback> cb, boost::system::error_code const& err, boost::asio::dir_monitor_event const& ev);

	static void on_event_v8(boost::shared_ptr<v8_core::callback> cb, boost::system::error_code err, boost::asio::dir_monitor_event ev);
};

}} // ::aspect::fs

#endif  // ENABLE_DIRECTORY_MONITOR
#endif // JSX_DIRECTORY_MONITOR_HPP_INCLUDED
