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
#include "jsx/v8_callback.hpp"

#include "jsx/runtime.hpp"

namespace aspect { namespace v8_core {

callback::callback(v8::FunctionCallbackInfo<v8::Value> const& args, int fn_arg, int fn_args_start)
	: isolate(args.GetIsolate())
	, fn(args.GetIsolate(), args[fn_arg].As<v8::Function>())
	, recv(args.GetIsolate(), args.This())
{
	if (fn.IsEmpty())
	{
		throw std::runtime_error("require function for " + std::to_string((long long)fn_arg) + "-nth argument");
	}

	if (fn_args_start == -1)
	{
		fn_args_start = fn_arg + 1;
	}
	int const fn_argc = args.Length() - fn_args_start;
	if (fn_argc > 0)
	{
		fn_args.reserve(fn_argc);
		for (int i = 0; i < fn_argc; ++i)
		{
			fn_args.emplace_back(v8pp::persistent<v8::Value>(isolate, args[i + fn_args_start]));
		}
	}
}

void callback::call(int argc, v8::Handle<v8::Value> argv[])
{
	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> recv = v8pp::to_local(isolate, this->recv);
	if (recv.IsEmpty())
	{
		recv = isolate->GetCurrentContext()->Global();
	}
	call(recv, argc, argv);
}

void callback::call(v8::Handle<v8::Object> recv, int argc, v8::Handle<v8::Value> args[])
{
	v8::HandleScope scope(isolate);

	v8::Local<v8::Function> function = v8pp::to_local(isolate, fn);

	std::vector<v8::Local<v8::Value>> arguments(argc + fn_args.size());

	std::copy(args, args + argc, arguments.begin());
	std::transform(fn_args.begin(), fn_args.end(), arguments.begin() + argc,
		[this](v8pp::persistent<v8::Value> const& arg)
		{
			return v8pp::to_local(isolate, arg);
		});

	v8::TryCatch try_catch;

	function->Call(recv, static_cast<int>(arguments.size()), arguments.data());
	if (try_catch.HasCaught())
	{
		runtime::instance(isolate).core().report_exception(try_catch);
	}
}

}} // aspect::v8_core
