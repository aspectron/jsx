//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_V8_CORE_HPP_INCLUDED
#define JSX_V8_CORE_HPP_INCLUDED

#include "jsx/core.hpp"

#pragma warning(push, 0)
#include <v8.h>
#include <v8-util.h>

#include <v8pp/module.hpp>
#include <v8pp/class.hpp>
#include <v8pp/convert.hpp>
#include <v8pp/persistent_ptr.hpp>
#pragma warning(pop)

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

#include "jsx/library.hpp"

namespace aspect {

/// Get optional value form V8 object by name, return true if the value exists in the object
template<typename T>
inline bool get_option(v8::Isolate* isolate, v8::Handle<v8::Object> options, char const* name, T& value)
{
	char const* dot = strchr(name, '.');
	if (dot)
	{
		std::string const subname(name, dot);
		v8::HandleScope scope(isolate);
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, subname.c_str(), suboptions)
			&& get_option(isolate, suboptions, dot+1, value);
	}
	v8::Local<v8::Value> val = options->Get(v8pp::to_v8(isolate, name));
	if (val.IsEmpty() || val == v8::Undefined(isolate))
	{
		return false;
	}
	value = v8pp::from_v8<T>(isolate, val);
	return true;
}

/// Set named value in V8 object
template<typename T>
bool set_option(v8::Isolate* isolate, v8::Handle<v8::Object> options, char const* name, T const& value)
{
	char const* dot = strchr(name, '.');
	if (dot)
	{
		std::string const subname(name, dot);
		v8::HandleScope scope(isolate);
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, subname.c_str(), suboptions)
			&& set_option(isolate, suboptions, dot+1, value);
	}
	options->Set(v8pp::to_v8(isolate, name), v8pp::to_v8(isolate, value));
	return true;
}

/// Set named constant in V8 object
template<typename T>
void set_const(v8::Isolate* isolate, v8::Handle<v8::Object> options, char const* name, T const& value)
{
	options->ForceSet(v8pp::to_v8(isolate, name), v8pp::to_v8(isolate, value),
		v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
}

class runtime;

namespace v8_core {

void execute_in_current_context(v8::FunctionCallbackInfo<v8::Value> const& args);
void execute_in_private_context(v8::FunctionCallbackInfo<v8::Value> const& args);

class CORE_API core : boost::noncopyable
{
	// friends to create and destroy
	friend class ::aspect::runtime;
	template <class T> friend void boost::checked_delete(T*);
private:
	explicit core(runtime& rt);
	~core();

	void init();
public:

	v8::UniquePersistent<v8::Context>& context() { return context_; }
	v8::UniquePersistent<v8::ObjectTemplate>& global_template() { return global_template_; }
	v8::UniquePersistent<v8::Function>& at_exit_handler() { return at_exit_handler_; }

	v8::Handle<v8::Value> run_script_file(boost::filesystem::path const& filename) const;
	v8::Handle<v8::Value> run_script(char const* source, size_t length, boost::filesystem::path filename) const;

	boost::filesystem::path resolve(std::string const& name, v8::Handle<v8::Value>& result) const;
	v8::Handle<v8::Value> require(std::string const& name);

	v8::Handle<v8::Value> load_library(std::string const& name);

	boost::filesystem::path const& current_script_path() const { return current_script_path_; }
	void set_current_script_path(boost::filesystem::path const& csp) { current_script_path_ = csp; }

	boost::filesystem::path const& node_lib_path() const { return node_lib_path_; }
	void set_node_lib_path(boost::filesystem::path const& path);

	void enable_debug(std::string const& module, uint16_t port, bool wait);

	void report_exception(v8::TryCatch const& try_catch, char const* origin = nullptr) const;

	/// Stringify a V8 value to JSON format
	std::string json_str(v8::Handle<v8::Value> value) const;

	/// Parse JSON string into V8 value, on exception return the string value
	v8::Handle<v8::Value> json_parse(std::string const& str) const;

	unsigned require_counter;
private:
	static void setup_bindings(v8pp::module& bindings);

	runtime& rt_;

	v8::UniquePersistent<v8::ObjectTemplate> global_template_;
	v8::UniquePersistent<v8::Context> context_;
	v8::UniquePersistent<v8::Object> runtime_;
	v8::UniquePersistent<v8::Function> at_exit_handler_;

	v8::UniquePersistent<v8::Object> json_;
	v8::UniquePersistent<v8::Function> json_stringify_, json_parse_;

	boost::filesystem::path current_script_path_;
	boost::filesystem::path node_lib_path_;

	v8::Handle<v8::Value> module_load(char const* source, size_t length,
		std::string const& name, boost::filesystem::path const& fullname) const;

	v8::Handle<v8::Value> require_impl(char const* source, size_t length,
		std::string const& name, boost::filesystem::path const& fullname) const;

	void unload_libraries();

	typedef boost::unordered_map<std::string, v8pp::persistent<v8::Value>> persistent_map;
	persistent_map script_cache_;

	std::vector<library> libraries_;
	persistent_map v8_libraries_;
};

}} // aspect::v8_core

namespace v8pp {

template<>
struct convert<boost::filesystem::path>
{
	typedef boost::filesystem::path result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return result_type(v8pp::from_v8<result_type::string_type>(isolate, value));
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, boost::filesystem::path const& value)
	{
		return v8pp::to_v8(isolate, value.native());
	}
};

} // v8pp

#endif // JSX_V8_CORE_HPP_INCLUDED
