//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_V8_MAIN_LOOP_HPP_INCLUDED
#define JSX_V8_MAIN_LOOP_HPP_INCLUDED

#include <boost/algorithm/clamp.hpp>
#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "jsx/threads.hpp"
#include "jsx/utils.hpp"

namespace aspect {

class runtime;

namespace v8_core {

class timer;

class CORE_API main_loop : boost::noncopyable
{
	// friends to create and destroy
	friend class ::aspect::runtime;
	template <class T> friend void boost::checked_delete(T*);
	friend class timer; // to access io_service_
private:
	explicit main_loop(runtime const& rt);

	/// Execute scheduled callbacks
	void execute_callbacks();

	/// Start the main V8 event loop
	void run();

	/// Terminate the main loop
	void terminate()
	{
		io_service_work_.reset();
		io_service_.stop();
	}

	/// Is the main loop terminating?
	bool is_terminating() const { return io_service_.stopped(); }

public:

	/// Callback function
	typedef boost::function<void ()> callback;

	/// Schedule callback call in context of V8
	bool schedule(callback cb)
	{
		_aspect_assert(cb);
		if (cb && !io_service_.stopped())
		{
			io_service_.post(cb);
			return true;
		}
		return false;
	}

	/// Maximum callback count to handle in one cycle
	unsigned max_callbacks() const { return max_callbacks_; }
	void set_max_callbacks(unsigned value)
	{
		max_callbacks_ = boost::algorithm::clamp(value, MIN_CALLBACKS, MAX_CALLBACKS);
	}

private:
	void check_global_termination();
	void idle_notification();
	void heap_statistics();

	runtime const& rt_;
	boost::asio::io_service io_service_;
	boost::scoped_ptr<boost::asio::io_service::work> io_service_work_;

	enum { MIN_CALLBACKS = 100, MAX_CALLBACKS = 10000 };

	unsigned max_callbacks_;
	double last_ts_heap_;
};

}} // aspect::v8_core

#endif // JSX_V8_MAIN_LOOP_HPP_INCLUDED
