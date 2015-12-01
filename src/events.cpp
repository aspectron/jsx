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
#include "jsx/events.hpp"

#include "jsx/runtime.hpp"

namespace aspect { namespace v8_core {

void event_emitter::setup_bindings(v8pp::module& bindings)
{
	/**
	@module events - Events
	@class EventEmitter

	EventEmitter is a class for named callback functions.
	EventEmitter stores event handlers and emit events by name.
	The class interface is compatible to
	`EventEmitter` class in [Node.js](http://nodejs.org/api/events.html)
	**/
	v8pp::class_<event_emitter> event_emitter_class(bindings.isolate());
	event_emitter_class
		/**
		@function on(name, handler)
		@param name {String}
		@param handler {Function}
		@return {EventEmitter} this to chain calls.

		Add event `handler` function for event `name`. Returns emitter to chain
		the calls:

			var e = new EventEmitter();
			e.on(`event1`, event1_handler).on(`event2`, event2_handler);

		Emit `newListener` event.
		**/
		.set("on", &event_emitter::add_listener<false>)

		/**
		@function once(name, handler)
		@param name {String}
		@param handler {Function}
		@return {EventEmitter} this to chain calls.

		Same as #EventEmitter.on, but call `hander` only **one time**. Removes `handler` after
		the event is occured.
		**/
		.set("once", &event_emitter::add_listener<true>)

		/**
		@function off(name, handler)
		@param name {String}
		@param handler {Function}
		@return {EventEmitter} this to chain calls.
		Remove `handler` for specfied event `name`. Returns emitter to chain
		the calls.

		Emit `removeListener` event.
		**/
		.set("off", &event_emitter::remove_listener)

		/**
		@function addListener()
		Alias for #EventEmitter.on method.
		**/
		.set("addListener", &event_emitter::add_listener<false>)

		/**
		@function removeListener()
		Alias for #EventEmitter.off method.
		**/
		.set("removeListener", &event_emitter::remove_listener)

		/**
		@function emit(name [, arg_1, .. arg_n])
		@param name {String}
		@return {Boolean}
		Execute all handlers for event `name` in order they were added with supplied
		optional arguments.

		Returns `true` if at least one handler for the event was been executed, and
		`false` otherwise.
		**/
		.set("emit", &event_emitter::emit_)

		/**
		@function removeAllListeners([name])
		@param name {String}
		@return {EventEmitter} this to chain calls.
		Remove all handlers for event `name`. If name is not specfied remove all
		handlers for all events. Returns emitter.
		**/
		.set("removeAllListeners", &event_emitter::remove_all_listeners)

		/**
		@function setMaxListeners(n)
		@param n {Number}
		No effect, used for API comaptibilty with Node.js
		**/
		.set("setMaxListeners", &event_emitter::set_max_listeners)

		/**
		@function listeners(name)
		@param name {String}
		@return {Array}
		Returns an array of event handlers for specified event `name`.
		**/
		.set("listeners", &event_emitter::listeners)

		/**
		@function listenerCount(emitter, name)
		@param emitter {EventEmitter}
		@param name {String}
		@return {Number}
		Class method returns the numer of handlers for specified event `name`.
		**/
		.set("listenerCount", &event_emitter::listener_count)
		;

	v8pp::module events(bindings.isolate());
	events.set("EventEmitter", event_emitter_class);
	bindings.set("events", events);
}

bool event_emitter::emit(v8::Isolate* isolate, std::string const& name, size_t argc, v8::Handle<v8::Value> argv[])
{
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> self = v8pp::to_v8(isolate, this);
	if (self.IsEmpty())
	{
		// already destroyed
		return false;
	}
	return emit(isolate, name, self->ToObject(), argc, argv);
}

bool event_emitter::emit(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Object> recv, size_t argc, v8::Handle<v8::Value> argv[])
{
	_aspect_assert(!recv.IsEmpty());
	if (recv.IsEmpty())
	{
		return false;
	}

	iterator_range range = callbacks_.equal_range(name);
	size_t count = range.second - range.first;
	if (count == 0)
	{
		return false;
	}

	for (iterator it = range.first; count > 0; --count)
	{
		v8::HandleScope scope(isolate);

		callback* event = it->second;

		v8::TryCatch try_catch;
		v8pp::to_local(isolate, event->cb)->Call(recv, static_cast<int>(argc), argv);
		if (try_catch.HasCaught())
		{
			runtime::instance(isolate).core().report_exception(try_catch);
		}

		if (event->once)
		{
			dispose(*it);
			it = callbacks_.erase(it);
		}
		else
		{
			++it;
		}
	}
	return true;
}

event_emitter& event_emitter::on(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Function> cb, bool once)
{
	callbacks::value_type value(name, new callback(isolate, cb, once));

	iterator_range range = callbacks_.equal_range(name);
	range.first = std::find_if(range.first, range.second, equal(value));
	if (range.first == range.second)
	{
		v8::HandleScope scope(isolate);
		v8::Local<v8::Value> args[2] = { v8pp::to_v8(isolate, name), cb };
		emit(isolate, "newListener", 2, args);

		callbacks_.insert(range.second, value);
	}
	else
	{
		dispose(value);
	}
	return *this;
}

event_emitter& event_emitter::off(v8::Isolate* isolate, std::string const& name, v8::Handle<v8::Function> cb)
{
	iterator_range range;
	if (name.empty())
	{
		range.first = callbacks_.begin();
		range.second = callbacks_.end();
	}
	else
	{
		range = callbacks_.equal_range(name);
		if (range.first != range.second && !cb.IsEmpty())
		{
			callbacks::value_type value(name, new callback(isolate, cb, false));
			range.first = std::find_if(range.first, range.second, equal(value));
			if (range.first != range.second)
			{
				range.second = range.first + 1;
			}
			dispose(value);
		}
	}

	if (range.first != range.second)
	{
		std::vector<callbacks::value_type> remove_listeners;

		// dispose callback handles at first
		for (iterator it = range.first; it != range.second; ++it)
		{
			v8::HandleScope scope(isolate);
			v8::Local<v8::Value> args[2] = {
				v8pp::to_v8(isolate, it->first),
				v8pp::to_local(isolate, it->second->cb)
			};
			emit(isolate, "removeListener", 2, args);

			if (it->first != "removeListener")
			{
				dispose(*it);
			}
			else
			{
				// delay "removeListener" callbacks disposing
				remove_listeners.push_back(*it);
			}
		}

		std::for_each(remove_listeners.begin(), remove_listeners.end(), dispose);

		// erase all callbacks after handles disposing
		callbacks_.erase(range.first, range.second);
	}
	return *this;
}

template<bool once>
event_emitter& event_emitter::add_listener(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::String> str = args[0]->ToString();
	v8::Local<v8::Function> callback = args[1].As<v8::Function>();

	if (str.IsEmpty() || callback.IsEmpty())
	{
		throw std::runtime_error("require event name string and listener function arguments");
	}

	v8::String::Utf8Value const name(str);
	return on(isolate, *name, callback, once);
}

event_emitter& event_emitter::remove_listener(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::String> str = args[0]->ToString();
	v8::Local<v8::Function> callback = args[1].As<v8::Function>();

	if (str.IsEmpty() || callback.IsEmpty())
	{
		throw std::runtime_error("require event name string and listener function arguments");
	}

	v8::String::Utf8Value const name(str);
	return off(isolate, *name, callback);
}

event_emitter& event_emitter::remove_all_listeners(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	return off(isolate, args.Length() > 0? *v8::String::Utf8Value(args[0]->ToString()) : "");
}

void event_emitter::set_max_listeners(size_t)
{
	// empty implementation
}

void event_emitter::listeners(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	std::string const name = v8pp::from_v8<std::string>(isolate, args[0]);
	iterator_range range = callbacks_.equal_range(name);
	
	v8::Local<v8::Array> result = v8::Array::New(isolate, static_cast<int>(range.second - range.first));
	for (uint32_t i = 0; range.first != range.second; ++range.first, ++i)
	{
		result->Set(i, v8pp::to_local(isolate, range.first->second->cb));
	}
	args.GetReturnValue().Set(scope.Escape(result));
}

size_t event_emitter::listener_count(event_emitter& emitter, std::string const& name)
{
	iterator_range range = emitter.callbacks_.equal_range(name);
	return (range.second - range.first);
}

void event_emitter::emit_(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::String> name = args[0]->ToString();
	if (name.IsEmpty())
	{
		throw std::runtime_error("require event name string");
	}

	std::vector<v8::Local<v8::Value>> argv;
	for (int i = 1; i < args.Length(); ++i)
	{
		argv.push_back(args[i]);
	}
	args.GetReturnValue().Set(emit(isolate, *v8::String::Utf8Value(name),
		argv.size(), argv.empty()? nullptr : &argv[0]));
}

}} // aspect::v8_core
