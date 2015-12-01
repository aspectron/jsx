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

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "jsx/runtime.hpp"
#include "jsx/v8_core.hpp"
#include "jsx/scriptstore.hpp"

#include "native_libraries.hpp"

namespace aspect { namespace v8_core {

v8::Handle<v8::Value> execute_in_private_context_impl(v8::Isolate* isolate, char const* source, size_t length,
	boost::filesystem::path const& filename, v8::Handle<v8::Object> args);

v8::Handle<v8::Value> core::module_load(char const* source, size_t length,
	std::string const& name, boost::filesystem::path const& fullname) const
{
	v8::Isolate* isolate = rt_.isolate();

	v8::EscapableHandleScope scope(isolate);

	char const wrapper_begin[] = "(function(exports, module, __filename, __dirname){";
	char const wrapper_end[] = "\n});";

	std::string wrapped_source;
	wrapped_source.reserve(length + sizeof(wrapper_begin) + sizeof(wrapper_end) - 2);

	wrapped_source.append(wrapper_begin, sizeof(wrapper_begin) - 1);
	wrapped_source.append(source, length);
	wrapped_source.append(wrapper_end, sizeof(wrapper_end) - 1);

	v8::Local<v8::Function> wrapped_script =
		run_script(wrapped_source.data(), wrapped_source.size(), fullname).As<v8::Function>();

	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Object> exports = v8::Object::New(isolate);
	set_option(isolate, module, "exports", exports);
	set_const(isolate, module, "id", name);
	set_const(isolate, module, "filename", fullname.string());
	set_option(isolate, module, "loaded", false);

	v8::Local<v8::Value> args[4] = { exports, module,
		v8pp::to_v8(isolate, fullname.string()), v8pp::to_v8(isolate, fullname.parent_path().string()) };

	v8::TryCatch try_catch;
	wrapped_script->Call(module, 4, args);

	if (try_catch.HasCaught())
	{
		report_exception(try_catch);
		return scope.Escape(try_catch.Exception());
	}
	else
	{
		set_option(isolate, module, "loaded", true);
		get_option(isolate, module, "exports", exports);
		return scope.Escape(exports);
	}
}

v8::Handle<v8::Value> core::require_impl(char const* source, size_t length,
	std::string const& name, boost::filesystem::path const& fullname) const
{
	v8::Isolate* isolate = rt_.isolate();

	v8::EscapableHandleScope scope(isolate);

	script_container container;
	container.decrypt(source, length);

	v8::Local<v8::Object> exports;

	if (rt_.execution_options() & runtime::PRIVATE_CONTEXT)
	{
		v8::Local<v8::Object> args = v8::Object::New(isolate);
		exports = v8::Object::New(isolate);
		set_option(isolate, args, "exports", exports);

		execute_in_private_context_impl(isolate, container.data(), container.size(), fullname, args);
		get_option(isolate, args, "exports", exports);
	}
	else
	{
		exports = module_load(container.data(), container.size(), name, fullname)->ToObject();
	}

	v8::Local<v8::Value> library;
	if (get_option(isolate, exports, "$", library))
	{
		return scope.Escape(library);
	}

	return scope.Escape(exports);
}

class current_script_path_scope
{
public:
	current_script_path_scope(core& c, boost::filesystem::path const& filename)
		: core_(c)
	{
		previous_script_path_ = core_.current_script_path();
		core_.set_current_script_path(filename.parent_path());
	}

	~current_script_path_scope()
	{
		core_.set_current_script_path(previous_script_path_);
	}

private:
	core& core_;
	boost::filesystem::path previous_script_path_;
};

void include(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string const filename = v8pp::from_v8<std::string>(isolate, args[0]);
	if (filename.empty())
	{
		throw std::runtime_error("empty file name");
	}

	runtime& rt = runtime::instance(isolate);

	boost::filesystem::path p;

	p = rt.rte_path() / filename;
	if ( is_regular_file(p) )
	{
		args.GetReturnValue().Set(rt.core().run_script_file(p));
		return;
	}

	p = rt.root_path() / filename;
	if ( is_regular_file(p) )
	{
		args.GetReturnValue().Set(rt.core().run_script_file(p));
		return;
	}

	p = rt.core().current_script_path() / filename;
	if ( is_regular_file(p) )
	{
		args.GetReturnValue().Set(rt.core().run_script_file(p));
		return;
	}

	throw std::runtime_error("unable to load file " + filename);
}

static bool check_path(boost::filesystem::path const& base, std::string const& name, boost::filesystem::path& p)
{
	p = base / name;
	if (is_regular_file(p)) return true;

	if (p.extension().empty())
	{
		p.replace_extension(".js");
		if (is_regular_file(p)) return true;
	}

	p = base / name / "package.json";
	if ( is_regular_file(p) )
	{
		using namespace boost::property_tree;
		ptree package;
		json_parser::read_json(p.string(), package);

		std::string const package_main = package.get("main", "");
		if (package_main.empty())
		{
			throw std::runtime_error("package contents do not declare 'main' property");
		}

		p = p.parent_path() / package_main;
		if (p.extension().empty())
		{
			p.replace_extension(".js");
		}
		if (is_regular_file(p)) return true;
		else throw std::runtime_error("unable to locate package main: " + p.string());
	}

	p = base / name / "index.js";
	if (is_regular_file(p)) return true;

	return false;
}

boost::filesystem::path core::resolve(std::string const& name, v8::Handle<v8::Value>& result) const
{
	boost::filesystem::path p;

	native_library const* native = std::find_if(std::begin(native_libraries), std::end(native_libraries),
		[&name](native_library const& lib) { return lib.name == name; });
	if (native != std::end(native_libraries))
	{
		result = require_impl(native->source, native->length, name, "");
		return p;
	}
	else
	{
		v8::Isolate* isolate = rt_.isolate();
		v8::Local<v8::Object> rt = v8pp::to_local(isolate, runtime_), bindings;
		v8::Local<v8::Value> native;
		if (get_option(isolate, rt, "bindings", bindings)
			&& get_option(isolate, bindings, name.c_str(), native))
		{
			result = native;
			return p;
		}
	}

	bool resolved = false;
	boost::filesystem::path const curr_path = current_script_path();

	if (!resolved)
	{
		resolved = check_path(curr_path, name, p);
	}

	if (!resolved)
	{
		boost::filesystem::path const script_path = rt_.script().parent_path();
		resolved = check_path(script_path, name, p)
			|| check_path(script_path / "libraries", name, p);
	}

	if (!resolved)
	{
		for (boost::filesystem::path path = curr_path.parent_path();
			!resolved && !path.empty(); path = path.parent_path())
		{
			resolved = check_path(path, name, p)
				|| check_path(path / "libraries", name, p);
		}
	}

	if (!resolved)
	{
		resolved = check_path(rt_.rte_path(), name, p)
			|| check_path(rt_.rte_path() / "libraries", name, p);
	}

	if (!resolved)
	{
		runtime::rte_include_paths const& rte_include_paths = rt_.get_rte_include_paths();
		runtime::rte_include_paths::const_iterator iter;
		for (iter = rte_include_paths.begin(); !resolved && iter != rte_include_paths.end(); ++iter)
		{
			boost::filesystem::path const& relative_path = *iter;
			for (boost::filesystem::path base_path = boost::filesystem::current_path();
				!resolved && !base_path.empty(); base_path = base_path.parent_path())
			{
				resolved = check_path(base_path / relative_path, name, p);
			}
		}
	}

	if (!resolved && !node_lib_path().empty())
	{
		resolved = check_path(node_lib_path(), name, p);
	}

	if (!resolved)
	{
		throw std::runtime_error("unable to locate target file or package " + name);
	}

	return boost::filesystem::canonical(p, curr_path);
}

v8::Handle<v8::Value> core::require(std::string const& name)
{
	if (name.empty())
	{
		throw std::runtime_error("empty name");
	}

	v8::Isolate* isolate = rt_.isolate();
	v8::EscapableHandleScope scope(isolate);

	persistent_map::iterator cache_it = script_cache_.find(name);
	if (cache_it == script_cache_.end())
	{
		v8::Local<v8::Value> result;
		boost::filesystem::path const fullname = resolve(name, result);
		std::string script_name = fullname.string();
		if (script_name.empty())
		{
			// loaded built in module
			_aspect_assert(!result.IsEmpty());
			script_name = name;
		}
		else
		{
			_aspect_assert(result.IsEmpty());
			cache_it = script_cache_.find(script_name);
			if (cache_it == script_cache_.end())
			{
				boost::iostreams::mapped_file_source script(fullname);
				if (!script.is_open())
				{
					throw std::runtime_error("Unable to open file " + script_name);
				}

				current_script_path_scope path_scope(*this, fullname);
				result = require_impl(script.data(), script.size(), name, fullname);
			}
		}
		v8pp::persistent<v8::Value> script(isolate, result);
		cache_it = script_cache_.emplace_hint(cache_it, script_name, std::move(script));
	}

	return scope.Escape(v8pp::to_local(isolate, cache_it->second));
}

class require_recursor : boost::noncopyable
{
public:
	explicit require_recursor(unsigned& recursion_count)
		: recursion_count_(recursion_count)
	{
		if ( recursion_count_ >= MAX_REQUIRE_RECURSIONS )
		{
			throw std::runtime_error("require() too many recursions");
		}
		++recursion_count_;
	}

	~require_recursor() { --recursion_count_; }

private:
	unsigned& recursion_count_;
};

void require(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	core& core = runtime::instance(isolate).core();

	require_recursor rr(core.require_counter);

	std::string const name = v8pp::from_v8<std::string>(isolate, args[0]);
	args.GetReturnValue().Set(core.require(name));
}

}} // ::aspect::v8_core
