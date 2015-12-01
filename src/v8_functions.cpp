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
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_main_loop.hpp"
#include "jsx/v8_timers.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace aspect { namespace v8_core {

void execute_in_current_context(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	if (!args[0]->IsString())
	{
		throw std::invalid_argument("expecting at least one parameter with script source code");
	}

	v8::Local<v8::String> script_source = args[0]->ToString();
	v8::Local<v8::String> script_name = args[1]->IsString()? args[1]->ToString() : v8::String::NewFromUtf8(isolate, "");

	v8::String::Utf8Value const source(script_source);
	v8::String::Utf8Value const name(script_name);
	v8::Local<v8::Value> result = runtime::instance(isolate).core().run_script(*source, source.length(), *name);

	args.GetReturnValue().Set(scope.Escape(result));
}

v8::Handle<v8::Value> execute_in_private_context_impl(v8::Isolate* isolate, char const* source, size_t length,
	boost::filesystem::path const& filename, v8::Handle<v8::Object> args)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::ObjectTemplate> object_template = v8pp::to_local(isolate, runtime::instance(isolate).core().global_template());
	v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, object_template);
	v8::Local<v8::Object> global = context->Global();

	context->Enter();
	set_option(isolate, global, "global", global);

	// TODO - is therer a more efficient way of doing this?
#if 1
	if (!args.IsEmpty())
	{
		v8::Local<v8::Array> properties = args->GetPropertyNames();
		for (uint32_t i = 0; i < properties->Length(); i++)
		{
			v8::Local<v8::Value> property = properties->Get(i);
			global->Set(property, args->Get(property));
		}
	}
#endif

	v8::Local<v8::Value> result = runtime::instance(isolate).core().run_script(source, length, filename);

	context->DetachGlobal();
	context->Exit();

	return scope.Escape(result);
}

void execute_in_private_context(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	if (!args[0]->IsString())
	{
		throw std::invalid_argument("expecting at least one parameter with script source code");
	}

	v8::Local<v8::String> script_source = args[0]->ToString();
	v8::Local<v8::String> script_name = args[1]->IsString()? args[1]->ToString() : v8pp::to_v8(isolate, "");

	v8::Local<v8::Object> relay;
	if (args.Length() > 1 && args[args.Length() - 1]->IsObject())
	{
		relay = args[args.Length() - 1]->ToObject();
	}

	v8::String::Utf8Value const source(script_source);
	v8::String::Utf8Value const name(script_name);

	v8::Local<v8::Value> result = execute_in_private_context_impl(isolate, *source, source.length(), *name, relay);
	args.GetReturnValue().Set(scope.Escape(result));
}

static void trace(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	
	v8::HandleScope scope(isolate);

	runtime const& rt = runtime::instance(isolate);

	for (int i = 0, count = args.Length(); i != count; ++i)
	{
		if (buffer const* buf = v8pp::from_v8<buffer*>(isolate, args[i]))
		{
			size_t const max_len = (std::min<size_t>)(buf->size(), 64);

			std::string buf_data(max_len, 0);
			std::transform(buf->data(), buf->data() + max_len, buf_data.begin(),
				[](char c) { return std::isprint(c)? c : '?'; });
			if (buf->size() > max_len) buf_data += "...";

			rt.trace("[buffer size: %d, data: %s]\n", buf->size(), buf_data.c_str());
		}
		else
		{
			v8::String::Utf8Value const str(args[i]);
			rt.trace("%s\n", *str);
		}
	}
}

static void dpc_0(callback* cb)
{
	cb->call(0, nullptr);
	delete cb;
}

static void dpc(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	uint32_t delay = 0;

	int fn_arg = 0;
	if (args[0]->IsUint32() && args[1]->IsFunction())
	{
		delay = args[0]->Uint32Value();
		fn_arg = 1;
	}
	else if (args[0]->IsFunction())
	{
		fn_arg = 0;
	}
	else
	{
		throw std::invalid_argument("dpc([delay,] function) requires at least one parameter");
	}

	callback* cb = new callback(args, fn_arg);
	if (delay)
	{
		v8pp::class_<timer>::import_external(isolate, new timer(delay, cb, true, false, true));
	}
	else
	{
		runtime::instance(isolate).main_loop().schedule(boost::bind(dpc_0, cb));
	}
}

static void message_box(v8::FunctionCallbackInfo<v8::Value> const& args)
{
#if OS(WINDOWS)
	v8::Isolate* isolate = args.GetIsolate();
	std::wstring const msg = v8pp::from_v8<std::wstring>(isolate, args[0]);
	std::wstring const caption = v8pp::from_v8<std::wstring>(isolate, args[1], L"");
	::MessageBoxW(NULL, msg.c_str(), caption.c_str(), MB_OK | MB_ICONEXCLAMATION);
#else
	//TODO: show message
#endif
}

static void generate_uuid(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	using namespace boost::uuids;

	if (args[0]->IsString())
	{
		uuid nil_uuid = nil_generator()();
		name_generator gen_name(nil_uuid);
		uuid const u = gen_name(*v8::String::Utf8Value(args[0]));

		args.GetReturnValue().Set(v8pp::to_v8(isolate, to_string(u)));
	}
	else if (args[0]->IsString() && args[1]->IsString())
	{
		name_generator gen_name(string_generator()(*v8::String::Utf8Value(args[0])));
		uuid const u = gen_name(*v8::String::Utf8Value(args[1]));

		args.GetReturnValue().Set(v8pp::to_v8(isolate, to_string(u)));
	}
	else
	{
		// random
		uuid u = random_generator()();
		args.GetReturnValue().Set(v8pp::to_v8(isolate, to_string(u)));
	}
}

static void pragma(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if ( args.Length() == 1 && args[0]->IsString() )
	{
		std::string const p = v8pp::from_v8<std::string>(isolate, args[0]);
		std::vector<std::string> const options = utils::split(p,' ');

		runtime& rt = runtime::instance(isolate);
		uint32_t flags = rt.execution_options();
		for (std::vector<std::string>::const_iterator iter = options.begin(); iter != options.end(); ++iter)
		{
			if ( *iter == "event-processing" || *iter == "event-queue" || *iter == "events")
			{
				flags |= runtime::EVENT_QUEUE;
			}
			else if ( *iter == "gc-notify" )
			{
				flags |= runtime::GC_NOTIFY;
			}
			else if ( *iter== "private-context" )
			{
				flags |= runtime::PRIVATE_CONTEXT;
			}
			else if ( *iter == "silent" )
			{
				flags |= runtime::SILENT;
			}
		}

		rt.set_execution_options(flags);
	}
}

void include(v8::FunctionCallbackInfo<v8::Value> const& args);
void require(v8::FunctionCallbackInfo<v8::Value> const& args);

void register_functions(v8pp::module& target)
{
	/**
	@module globals
	Global objects and functions available in all modules
	**/
	target
		/**
		@function include(filename)
		@param filename {String}
		@return {Value} script run result

		Try to load and run JavaScript file.
		Path order used in the include file search:

		  1. runtime#rt.local.rtePath
		  2. runtime#rt.local.rootPath
		  3. runtime#rt.local.scriptPath
		**/
		.set("include", include)

		/**
		@function require(name)
		@param name {String}
		@return {Object} module.exports object

		Try to load a module with specified `name`.
		Path order used in the module search:

		  1. Built-in modules, see bindings#
		  2. Current script scope path
		  3. runtime#rt.local.scriptPath, runtime#rt.local.scriptPath`/libraries/`
		  4. In the `libraries/` directory of the current script scope path and to upper directories in the file system
		  5. runtime#rt.local.rtePath, runtime#rt.local.rtePath`/libraries/`
		  6. Additional runtime search paths, see runtime#rt.addRteIncludePath
		  7. Node library path, see runtime#rt.setNodeLibPath
		**/
		.set("require", require)

		/**
		@function trace(args)
		@param [args]*
		Print arguments to console, each argument on a new line.
		**/
		.set("trace", trace)

		/**
		@function getTimestamp()
		@return {Number} timestamp in milliseconds
		Return timestamp in milliseconds, relative to some point in past.
		The main purpose is to measure duration between timestamps.
		**/
		.set("getTimestamp", utils::get_ts)

		/**
		@function dpc([delay = 0,] function [, args..])
		@param [delay=0] {Number}, delay value in milliseconds
		@param function {Function} callback function
		@param [args]* arguments to pass into callback
		Run `function` after `delay` milliseconds.
		Optional arguments `args` will be passed to the `function`.
		If `delay` is zero or unspecified, the function will be scheduled for
		immediate call.

		See also timer#
		**/
		.set("dpc", dpc)

		/**
		@function messageBox(message [, caption])
		@param message {String}
		@param [caption] {String}
		Show notification message dialog with optional caption, Windows only.
		**/
		.set("messageBox", message_box)

		/**
		@function generateUUID([name] [, namespace])
		@param name {String}
		@param [namespace] {String}
		@return {String}
		Generate a name based UUID from a namespace UUID and a name.
		If `namespace` is not specified a nil namespace UUID is used.
		If `name` is not specified a random UUID is generated.
		See also uuid#
		**/
		.set("generateUUID", generate_uuid)
		/**
		@function pragma(options)
		@param options {String}
		Setup runtime execution options.
		Options is a string of space delimited pragmas:
		  * `event-queue`      Run a loop to process events
		  * `gc-notify`        Notify V8 about idle in the event loop to reduce memory footprint
		  * `private-context`  Use private context for each module loaded with #require
		  * `silent`           Run in silent mode with no log output
		**/
		.set("pragma", pragma)
		;
}

}} // aspect::v8_core
