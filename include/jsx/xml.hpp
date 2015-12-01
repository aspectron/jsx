//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_XML_HPP_INCLUDED
#define JSX_XML_HPP_INCLUDED

#include "jsx/v8_core.hpp"

#include <boost/noncopyable.hpp>

struct XML_ParserStruct;

namespace aspect { namespace xml {

void setup_bindings(v8pp::module& target);

class sax_parser : boost::noncopyable
{
public:
	sax_parser();
	~sax_parser();

	bool parse(std::string const& chunk, bool finished);

	sax_parser& reset();

	void set_handler(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	static void start_node(void* ptr, const char* name, char const** attrs);
	static void end_node(void* ptr, char const* name);
	static void chars(void* ptr, char const* txt, int len);

	XML_ParserStruct* parser_;
};

}} // ::aspect::xml

#endif // JSX_XML_HPP_INCLUDED
