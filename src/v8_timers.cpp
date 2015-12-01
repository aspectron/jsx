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
#include "jsx/v8_timers.hpp"

#include "jsx/runtime.hpp"
#include "jsx/v8_main_loop.hpp"

namespace aspect { namespace v8_core {

#ifdef BOOST_ASIO_HAS_STD_CHRONO
	using namespace std::chrono;
#else
	using namespace boost::chrono;
#endif

enum timer_mode { once, repeat, immediate };

template<timer_mode mode>
static void setup_timer(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	//timer options
	bool const is_enabled = true;
	bool const is_dpc = (mode != repeat);
	bool const is_immediate = (mode == immediate);

	v8::Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsFunction())
	{
		args.GetReturnValue().Set(v8pp::throw_ex(isolate, "require 1st argument for callback function", v8::Exception::TypeError));
		return;
	}

	if (!is_immediate && !args[1]->IsInt32())
	{
		args.GetReturnValue().Set(v8pp::throw_ex(isolate, "require 2nd argument delay in ms", v8::Exception::TypeError));
		return;
	}

	uint32_t const delay = is_immediate? 0 : args[1]->Uint32Value();
	int const argc = is_immediate? 1 : 2;
	timer* t = new timer(delay, new callback(args, 0, argc), is_enabled, is_immediate, is_dpc);

	args.GetReturnValue().Set(v8pp::class_<timer>::import_external(isolate, t));
}

static void stop_timer(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (timer* t = v8pp::from_v8<timer*>(isolate, args[0]))
	{
		t->stop();
	}
	else
	{
		args.GetReturnValue().Set(v8pp::throw_ex(isolate, "require valid timeoutId", v8::Exception::TypeError));
	}
}

void timer::register_functions(v8pp::module& global)
{
	/**
	@module globals
	**/
	global
		/**
		@function setTimeout(function, delay [, args...])
		@param function {Function}
		@param delay    {Number}
		@return {Timer} timer instance to use it with #clearTimeout
		Schedule one time `function` call after `delay` ms.
		Optional arguments `args` will be passed to the `function`.
		See also #dpc
		**/
		.set("setTimeout", setup_timer<once>)

		/**
		@function clearTimeout(timer)
		@param timer {Timer}
		Cancel one time function call scheduled with #setTimeout
		**/
		.set("clearTimeout", stop_timer)

		/**
		@function setInterval(function, delay [, args])
		@param function {Function}
		@param delay    {Number}
		@return {Timer} timer instance to use it with #clearInterval
		Schedule repeated `function` call every `delay` milliseconds.
		Optional arguments `args` will be passed to the `function`.
		**/
		.set("setInterval", setup_timer<repeat>)

		/**
		@function clearInterval(timer)
		@param timer {Timer}
		Cancel a repeated function call scheduled with #setInterval
		**/
		.set("clearInterval", stop_timer)

		/**
		@function setImmediate(function [, args])
		@param function {Function}
		@return {Timer} timer instance to use it with #clearImmediate
		Equals to #setTimeout with 0 ms delay.
		**/
		.set("setImmediate", setup_timer<immediate>)

		/**
		@function clearImmediate(timer)
		@param timer {Timer}
		Cancel an immediate function call scheduled with #setImmediate
		**/
		.set("clearImmediate", stop_timer)
		;
}

void timer::setup_bindings(v8pp::module& bindings)
{
	/**
	@module timer Timer

	@class Timer
	Timer class

	@function Timer(interval, function [, enabled = true, immediate = true, once = false])
	@param interval          {Number}    Timer interval in milliseconds
	@param function          {Function}  Timer function
	@param [enabled=true]    {Boolean}   Is timer enabled
	@param [immediate=true]  {Boolean}   Call timer `function` immediately
	@param [once=false]      {Boolean}   Call timer `function` only once

	Create a timer instance to call `function` every `interval` milliseconds
	**/
	v8pp::class_<timer> timer_class(bindings.isolate(), v8pp::v8_args_ctor);
	timer_class
		/**
		@function start(immediate)
		@param immediate {Boolean}
		Start or resume disabled timer. Immediately call the timer
		`function` if `immediate == true`, otherwise wait for the
		timer `interval` milliseconds before the `function` call.
		**/
		.set("start", &timer::start)

		/**
		@function stop()
		Stop and disable the timer, cancel next timer function call.
		**/
		.set("stop", &timer::stop)
		.set("cancel", &timer::cancel)

		/**
		@function changeInterval(interval)
		@param interval {Number}
		Set new timer interval in milliseconds.
		Enabled timer will be restarted with the new `interval` value.
		See also #Timer.interval
		**/
		.set("changeInterval", &timer::set_interval)

		/**
		@property enabled {Boolean}
		Wether the timer is enabled. Changing this property will cause
		the timer to start or stop.
		**/
		.set("enabled", v8pp::property(&timer::is_enabled, &timer::set_enabled))

		/**
		@property interval {Number}
		Timer interval in milliseconds. Changing this property is similar to
		call #Timer.changeInterval function.
		**/
		.set("interval", v8pp::property(&timer::interval, &timer::set_interval))
		;
	bindings.set("timer", timer_class);
}

timer::timer(uint64_t interval, callback* cb, bool enabled, bool immediate, bool is_dpc)
	: impl_(runtime::instance(cb->isolate).main_loop().io_service_)
{
	init(interval, cb, enabled, immediate, is_dpc);
}

timer::timer(v8::FunctionCallbackInfo<v8::Value> const& args)
	: impl_(runtime::instance(args.GetIsolate()).main_loop().io_service_)
{
	v8::Isolate* isolate = args.GetIsolate();

	int argc = 2;

	if ( args.Length() < argc )
	{
		throw std::invalid_argument("timer(interval, function [,enabled]) requires at least two parameters");
	}
	if ( !args[0]->IsUint32() )
	{
		throw std::invalid_argument("first parameter to function must be timer interval in milliseconds (numeric value)");
	}
	if ( !args[1]->IsFunction() )
	{
		throw std::invalid_argument("second parameter to timer() must be a callback function");
	}

	uint32_t const interval = args[0]->ToUint32()->Uint32Value();

	bool enabled = true;
	if ( args.Length() > argc )
	{
		enabled = v8pp::from_v8<bool>(isolate, args[argc++]);
	}

	bool immediate = true;
	if ( args.Length() > argc )
	{
		immediate = v8pp::from_v8<bool>(isolate, args[argc++]);
	}

	bool is_dpc = false;
	if ( args.Length() > argc )
	{
		is_dpc = v8pp::from_v8<bool>(isolate, args[argc++]);
	}

	init(interval, new callback(args, 1, argc), enabled, immediate, is_dpc);
}

timer::~timer()
{
	stop();
	callback_.reset();
}

void timer::init(uint64_t interval, callback* cb, bool enabled, bool immediate, bool is_dpc)
{
	interval_ = milliseconds(interval);
	callback_.reset(cb);
	dpc_ = is_dpc;
	enabled_ = false;
	if ( enabled )
	{
		start(immediate);
	}
}

void timer::execute(boost::system::error_code const& ec)
{
	if (ec || !callback_ || !callback_->isolate)
	{
		return;
	}

	_aspect_assert(callback_->isolate == v8::Isolate::GetCurrent());
	callback_->call(v8pp::to_v8(callback_->isolate, this)->ToObject(), 0, nullptr);
	if ( !dpc_ && enabled_ )
	{
		impl_.expires_from_now(interval_);
		impl_.async_wait(boost::bind(&timer::execute, this, boost::asio::placeholders::error));
	}
}

void timer::start(bool immediate)
{
	if ( !enabled_ )
	{
		enabled_ = true;
		impl_.expires_from_now(immediate? impl::duration() : interval_);
		impl_.async_wait(boost::bind(&timer::execute, this, boost::asio::placeholders::error));
	}
}

void timer::stop()
{
	if ( enabled_ )
	{
		enabled_ = false;
		impl_.cancel();
	}
}

uint64_t timer::interval() const
{
	return duration_cast<milliseconds>(interval_).count();
}

void timer::set_interval(uint64_t interval)
{
	impl::duration const new_interval = milliseconds(interval);
	if (new_interval != interval_)
	{
		interval_ = new_interval;
		if ( enabled_ )
		{
			stop();
			start(false);
		}
	}
}

}} // aspect::v8_core
