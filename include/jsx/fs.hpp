//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_FS_HPP_INCLUDED
#define JSX_FS_HPP_INCLUDED

#include "jsx/v8_core.hpp"

namespace aspect { namespace fs {

void setup_bindings(v8pp::module& target);

class v8_path
{
public:
	explicit v8_path(v8::FunctionCallbackInfo<v8::Value> const& args)
		: path_(from_v8(args.GetIsolate(), args[0]))
	{
	}

	explicit v8_path(v8::Isolate* isolate, v8::Handle<v8::Value> value)
		: path_(from_v8(isolate, value))
	{
	}

	explicit v8_path(boost::filesystem::path const& path)
		: path_(path)
	{
	}

	operator boost::filesystem::path const&() const { return path_; }

	bool empty() const { return path_.empty(); }

	void string(v8::FunctionCallbackInfo<v8::Value> const& args);
	void parent_path(v8::FunctionCallbackInfo<v8::Value> const& args);
	void stem(v8::FunctionCallbackInfo<v8::Value> const& args);
	void filename(v8::FunctionCallbackInfo<v8::Value> const& args);
	void extension(v8::FunctionCallbackInfo<v8::Value> const& args);

	static boost::filesystem::path from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value);
	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, boost::filesystem::path const& p);

private:
	boost::filesystem::path path_;
};

}} // aspect::fs

#endif // JSX_FS_HPP_INCLUDED
