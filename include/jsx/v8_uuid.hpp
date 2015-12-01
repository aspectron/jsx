//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_V8_UUID_GENERATOR_HPP_INCLUDED
#define JSX_V8_UUID_GENERATOR_HPP_INCLUDED

#include <boost/uuid/uuid.hpp>

#include "jsx/v8_core.hpp"

namespace aspect { namespace v8_core {

class CORE_API uuid_generator
{
public:
	static void setup_bindings(v8pp::module& target);

	uuid_generator();
	explicit uuid_generator(std::string const& ns);
	explicit uuid_generator(v8::FunctionCallbackInfo<v8::Value> const& args);

	std::string name(std::string const& ns) const;
	std::string random() const;

private:
	boost::uuids::uuid namespace_uuid_;
};

}} // aspect::v8_core

#endif // JSX_V8_UUID_GENERATOR_HPP_INCLUDED
