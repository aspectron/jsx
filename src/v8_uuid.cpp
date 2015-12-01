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
#include "jsx/v8_uuid.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace aspect { namespace v8_core {

using namespace boost::uuids;

void uuid_generator::setup_bindings(v8pp::module& target)
{
	/**
	@module uuid UUID
	UUID generation

	@class Uuid
	UUID generator

	@function Uuid([namespace])
	@param [namespace] {String}
	Constructor. Create a UUID generator with optional `namespace` argument
	used in #Uuid.name method.
	**/
	v8pp::class_<uuid_generator> uuid_class(target.isolate(), v8pp::v8_args_ctor);
	uuid_class
		/**
		@function name(str)
		@param name {String}
		@return {String}
		Generate a `name` based UUID using a namespace UUID passed to contructor.
		**/
		.set("name", &uuid_generator::name)

		/**
		@function random()
		@return {String}
		Generate and return a random number based UUID string.
		**/
		.set("random", &uuid_generator::random)
		;
	target.set("Uuid", uuid_class);
}

uuid_generator::uuid_generator()
	: namespace_uuid_(nil_generator()())
{
}

uuid_generator::uuid_generator(std::string const& ns)
	: namespace_uuid_(string_generator()(ns))
{
}

uuid_generator::uuid_generator(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if ( args.Length() == 0 )
	{
		*this = uuid_generator();
	}
	else if ( args.Length() == 1 && args[0]->IsString() )
	{
		std::string const ns = v8pp::from_v8<std::string>(args.GetIsolate(), args[0]);
		*this = uuid_generator(ns);
	}
	else
	{
		throw std::runtime_error("invalid parameter on input (expecting nothing or namespace uuid)");
	}
}

std::string uuid_generator::name(std::string const& name) const
{
	if ( name.empty() )
	{
		throw std::invalid_argument("expecting string parameter");
	}

	name_generator gen_name(namespace_uuid_);
	uuid const u = gen_name(name.c_str());
	return to_string(u);
}

std::string uuid_generator::random() const
{
	uuid const u = random_generator()();
	return to_string(u);
}

}} // aspect::v8_core
