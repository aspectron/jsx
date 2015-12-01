//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_EVENTS_HPP_INCLUDED
#define JSX_EVENTS_HPP_INCLUDED

#include <boost/container/flat_map.hpp>

#include "jsx/utils.hpp"
#include "jsx/v8_core.hpp"

namespace aspect { namespace v8_core {

/// Event emitter to JavaScript side
class CORE_API event_emitter : boost::noncopyable
{
public:
	static void setup_bindings(v8pp::module& bindings);

	virtual ~event_emitter()
	{
		std::for_each(callbacks_.begin(), callbacks_.end(), dispose);
	}

	/// Add event callback, return *this
	event_emitter& on(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Function> cb, bool once = false);

	/// Remove event callback, return *this
	event_emitter& off(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Function> cb = v8::Handle<v8::Function>());

	/// Is there any handler for event 'name'?
	bool has(std::string const& name) const
	{
		return callbacks_.find(name) != callbacks_.end();
	}

	/// Emit event 'name' with arguments array argv[argc]
	/// Return true if the event has been emitted
	bool emit(v8::Isolate* isolate, std::string const& name, size_t argc, v8::Handle<v8::Value> argv[]);

	/// Emit event 'name' with arguments array argv[argc] using recv as `this` in event callbacks
	/// Return true if the event has been emitted
	bool emit(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Object> recv, size_t argc, v8::Handle<v8::Value> argv[]);

private:
	template<bool once>
	event_emitter& add_listener(v8::FunctionCallbackInfo<v8::Value> const& args);

	event_emitter& remove_listener(v8::FunctionCallbackInfo<v8::Value> const& args);
	event_emitter& remove_all_listeners(v8::FunctionCallbackInfo<v8::Value> const& args);

	void set_max_listeners(size_t n);

	void listeners(v8::FunctionCallbackInfo<v8::Value> const& args);

	static size_t listener_count(event_emitter& emitter, std::string const& name);

	void emit_(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	struct callback : boost::noncopyable
	{
		v8pp::persistent<v8::Function> cb;
		bool once;

		callback(v8::Isolate* isolate, v8::Handle<v8::Function> cb, bool once)
			: cb(isolate, cb)
			, once(once)
		{
		}
	};

	typedef boost::container::flat_multimap<std::string, callback*> callbacks;
	typedef callbacks::iterator iterator;
	typedef std::pair<iterator, iterator> iterator_range;

	callbacks callbacks_;

	static void dispose(callbacks::value_type& kv) { delete kv.second; }

	struct equal : std::unary_function<callbacks::value_type, bool>
	{
		callbacks::value_type const& lhs;

		explicit equal(callbacks::value_type const& lhs)
			:lhs(lhs)
		{
		}

		bool operator()(callbacks::value_type const& rhs) const
		{
			_aspect_assert(lhs.first == rhs.first);
			return lhs.second->cb == rhs.second->cb;
		}
	};
};

}} // aspect::v8_core

#endif // JSX_EVENTS_HPP_INCLUDED
