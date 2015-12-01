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
#include "jsx/xml.hpp"

#include "expat.h"

#include "jsx/runtime.hpp"

namespace aspect { namespace xml {

void setup_bindings(v8pp::module& target)
{
	/**
	@module xml
	**/
	v8pp::module xml_module(target.isolate());

	/**
	@class Parser
	XML SAX parser
	**/
	v8pp::class_<sax_parser> parser_class(target.isolate());
	parser_class
		/**
		@function parse(chunk, finished)
		@param chunk {String}
		@param finished {Boolean}
		@return {Boolean} `true` on success
		Parse a `chunk` of XML document. Set `finished = true` for the last chunk.
		**/
		.set("parse", &sax_parser::parse)

		/**
		@function reset()
		@return {Parser} `this` to chain calls
		Reset the parser state.
		**/
		.set("reset", &sax_parser::reset)

		/**
		@function setHandler(name, handler)
		@param name {String}
		@param handler {Function}
		@return {Parser} `this` to chain calls
		Set `handler`function for the parser event `name`.

		@event start_node(name, attrs)
		@param name {String}   XML node name
		@param attrs {Object}  XML node attributes

		@event end_node(name)
		@param name {String}   XML node name

		@event char_handler(text)
		@param text {String}   XML node character data
		**/
		.set("setHandler", &sax_parser::set_handler)
		;

	xml_module.set("Parser", parser_class);
	target.set("xml", xml_module.new_instance());
}

sax_parser::sax_parser()
	: parser_(nullptr)
{
	reset();
}

sax_parser::~sax_parser()
{
	if (parser_)
	{
		XML_ParserFree(parser_);
	}
}

bool sax_parser::parse(std::string const& chunk, bool finished)
{
	if (chunk.empty())
	{
		return true;
	}

	bool const result = (XML_Parse(parser_, chunk.c_str(), static_cast<int>(chunk.size()), finished) != XML_STATUS_ERROR);

	if (!result)
	{
		std::stringstream msg;
		msg << "XML parse error at input line "
			<< XML_GetCurrentLineNumber(parser_)
			<< ": "
			<< XML_ErrorString(XML_GetErrorCode(parser_))
			//<< ": input=["<<chunk<<"]"
			<< '.'
			;

		reset();
		throw std::runtime_error(msg.str());
	}

	if (result && finished)
	{
		reset();
	}

	return result;
}

sax_parser& sax_parser::reset()
{
	if (!parser_)
	{
		parser_ = XML_ParserCreate(nullptr);
		if (!parser_)
		{
			throw std::runtime_error("XML_ParserCreate() failed (OOM?)");
		}

		XML_SetElementHandler(parser_, &start_node, &end_node);
		XML_SetCharacterDataHandler(parser_, &chars);
		XML_SetUserData(parser_, this);
	}
	else
	{
		XML_ParserReset(parser_, nullptr);
	}

	return *this;
}

void sax_parser::set_handler(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (!args[0]->IsString() || !args[1]->IsFunction())
	{
		throw std::runtime_error("require name, function arguments");
	}

	args.This()->Set(args[0], args[1]);
	args.GetReturnValue().Set(args.This());
}

void sax_parser::start_node(void* ptr, char const* name, char const** attrs)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> self = v8pp::to_v8(isolate, static_cast<sax_parser*>(ptr))->ToObject();
	v8::Local<v8::Function> handler;
	if (!get_option(isolate, self, "start_node", handler))
	{
		return;
	}

	v8::Local<v8::Object> oattr = v8::Object::New(isolate);
	if (attrs)
	{
		for (int i = 0; attrs[i]; i += 2)
		{
			char const* const key = attrs[i];
			char const* const val = attrs[1+i];
			set_option(isolate, oattr, key, val);
		}
	}

	v8::Handle<v8::Value> args[2] = { v8pp::to_v8(isolate, name), oattr };

	v8::TryCatch try_catch;
	handler->Call(self, 2, args);
	if (try_catch.HasCaught())
	{
		runtime::instance(isolate).core().report_exception(try_catch);
	}
}

void sax_parser::end_node(void* ptr, char const* name)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> self = v8pp::to_v8(isolate, static_cast<sax_parser*>(ptr))->ToObject();
	v8::Local<v8::Function> handler;
	if (!get_option(isolate, self, "end_node", handler))
	{
		return;
	}

	v8::Handle<v8::Value> args[1] = { v8pp::to_v8(isolate, name) };

	v8::TryCatch try_catch;
	handler->Call(self, 1, args);
	if (try_catch.HasCaught())
	{
		runtime::instance(isolate).core().report_exception(try_catch);
	}
}

void sax_parser::chars(void* ptr, char const* txt, int len)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> self = v8pp::to_v8(isolate, static_cast<sax_parser*>(ptr))->ToObject();
	v8::Local<v8::Function> handler;
	if (!get_option(isolate, self, "char_handler", handler))
	{
		return;
	}

	v8::Handle<v8::Value> args[1] = { v8pp::to_v8(isolate, txt, len) };

	v8::TryCatch try_catch;
	handler->Call(self, 1, args);
	if (try_catch.HasCaught())
	{
		runtime::instance(isolate).core().report_exception(try_catch);
	}
}

}} // ::aspect::xml
