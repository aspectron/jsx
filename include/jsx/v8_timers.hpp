//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_TIMERS_HPP_INCLUDED
#define JSX_TIMERS_HPP_INCLUDED

#include <boost/asio/high_resolution_timer.hpp>

#include "jsx/v8_core.hpp"
#include "jsx/v8_callback.hpp"

namespace aspect { namespace v8_core {

class CORE_API timer
{
public:
	static void setup_bindings(v8pp::module& bindings);
	static void register_functions(v8pp::module& global);

	explicit timer(v8::FunctionCallbackInfo<v8::Value> const& args);
	timer(uint64_t interval, callback* cb, bool enabled, bool immediate, bool is_dpc);
	~timer();

	void start(bool immediate);
	void stop();

	void cancel() { impl_.cancel(); }

	bool is_enabled() const { return enabled_; }
	void set_enabled(bool enable)
	{
		if (enabled_ != enable)
		{
			enable? start(false) : stop();
		}
	}

	uint64_t interval() const;
	void set_interval(uint64_t interval);

private:
	void init(uint64_t interval, callback* cb, bool enabled, bool immediate, bool is_dpc);
	void execute(boost::system::error_code const& ec);

	typedef boost::asio::high_resolution_timer impl;
	impl impl_;
	impl::duration interval_;
	bool enabled_;
	bool dpc_;
	boost::scoped_ptr<callback> callback_;
};

}} // aspect::v8_core

#endif // JSX_TIMERS_HPP_INCLUDED
