//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CALLBACK_HPP_INCLUDED
#define JSX_CALLBACK_HPP_INCLUDED

#include "jsx/v8_core.hpp"

namespace aspect { namespace v8_core {

/// Persistent callback function with persistent handle to V8 function and bound arguments
struct CORE_API callback : boost::noncopyable
{
	v8::Isolate* isolate;
	v8pp::persistent<v8::Function> fn;
	std::vector<v8pp::persistent<v8::Value>> fn_args;
	v8pp::persistent<v8::Object> recv;

	/// Create a callback from args with v8::Function at fn_arg and bound arguments at fn_args_start
	/// If fn_args_start == -1 bound all arguments after fn_arg
	callback(v8::FunctionCallbackInfo<v8::Value> const& args, int fn_arg, int fn_args_start = -1);

	/// Invoke callback for recv object with argv[argc] arguments
	void call(int argc, v8::Handle<v8::Value> argv[]);

	/// Invoke callback for recv object with argv[argc] arguments
	void call(v8::Handle<v8::Object> recv, int argc, v8::Handle<v8::Value> argv[]);
};


}} // aspect::v8_core

#endif // JSX_CALLBACK_HPP_INCLUDED
