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
#include "jsx/v8_core.hpp"

#include "jsx/console.hpp"
#include "jsx/crypto.hpp"
#include "jsx/library.hpp"
#include "jsx/os.hpp"
#include "jsx/process.hpp"
#include "jsx/runtime.hpp"
#include "jsx/scriptstore.hpp"
#include "jsx/fs.hpp"
#include "jsx/db.hpp"
#include "jsx/tcp.hpp"
#include "jsx/http.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_timers.hpp"
#include "jsx/v8_uuid.hpp"
#include "jsx/xml.hpp"
#include "jsx/node_support.hpp"

#if OS(WINDOWS)
#include "jsx/firewall.hpp"
#include "jsx/registry.hpp"
#endif

#include <boost/iostreams/device/mapped_file.hpp>

#include <zlib.h> // for ZLIB_VERSION
#include <openssl/opensslv.h> // for OPENSSL_VERSION_NUMBER
#include <pion/config.hpp> // for PION_VERSION

#include <v8-debug.h>

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#elif !defined(_MSC_VER)
extern char **environ;
#endif

namespace aspect {

void setup_netinfo_bindings(v8pp::module& target);

namespace v8_core {

void register_functions(v8pp::module& global_template);

static void env_get(v8::Local<v8::String> property, v8::PropertyCallbackInfo<v8::Value> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Value> result = v8::Undefined(isolate);
#if OS(WINDOWS)
	v8::String::Value const name(property);
	DWORD const max_value_length = 32767; // see GetEnvironmentVariable() in MSDN
	wchar_t value[max_value_length];
	DWORD const length = ::GetEnvironmentVariableW(reinterpret_cast<wchar_t const*>(*name), value, max_value_length);
	if (length)
	{
		result = v8pp::to_v8(isolate, value, length);
	}
#else
	v8::String::Utf8Value const name(property);
	char const* value = getenv(*name);
	if (value)
	{
		result = v8pp::to_v8(isolate, value);
	}
#endif
	info.GetReturnValue().Set(scope.Escape(result));
}

static void env_set(v8::Local<v8::String> property, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<v8::Value> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::HandleScope scope(isolate);

#if OS(WINDOWS)
	v8::String::Value const name(property);
	v8::String::Value const str(value);
	::SetEnvironmentVariableW(reinterpret_cast<wchar_t const*>(*name), reinterpret_cast<wchar_t const*>(*str));
#else
	v8::String::Utf8Value const name(property);
	v8::String::Utf8Value const val(value->ToString());
	setenv(*name, *val, 1);
#endif
	info.GetReturnValue().Set(value);
}

static void env_query(v8::Local<v8::String> property, v8::PropertyCallbackInfo<v8::Integer> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Integer> result;
#if OS(WINDOWS)
	v8::String::Value const name(property);
	::GetEnvironmentVariableW(reinterpret_cast<wchar_t const*>(*name), nullptr, 0);
	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
	{
		result = v8::Integer::New(isolate, 0);
	}
#else
	v8::String::Utf8Value const name(property);
	if (getenv(*name))
	{
		result = v8::Integer::New(isolate, 0);
	}
#endif
	info.GetReturnValue().Set(scope.Escape(result));
}

static void env_del(v8::Local<v8::String> property, v8::PropertyCallbackInfo<v8::Boolean> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::HandleScope scope(isolate);

	bool result = false;
#if OS(WINDOWS)
	v8::String::Value const name(property);
	if (::SetEnvironmentVariableW(reinterpret_cast<wchar_t const*>(*name), nullptr))
	{
		result = true;
	}
	else
	{
		::GetEnvironmentVariableW(reinterpret_cast<wchar_t const*>(*name), nullptr, 0);
		result = (GetLastError() == ERROR_ENVVAR_NOT_FOUND);
	}
#else
	v8::String::Utf8Value const name(property);
	if (getenv(*name))
	{
		result = true;
		unsetenv(*name);
	}
#endif
	info.GetReturnValue().Set(result);
}

static void env_enum(v8::PropertyCallbackInfo<v8::Array> const& info)
{
	v8::Isolate* isolate = info.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Array> result = v8::Array::New(isolate);
#if OS(WINDOWS)
	wchar_t* environment_strings = GetEnvironmentStringsW();
	wchar_t const* wenviron = environment_strings;
	for (uint32_t i = 0; wenviron && *wenviron; ++i, wenviron += wcslen(wenviron) + 1)
	{
		if (*wenviron == L'=')
		{
			//skip hidden variables
			continue;
		}
		wchar_t const* const pos = wcschr(wenviron, L'=');
		int const length = pos? int(pos - wenviron) : -1;
		result->Set(i, v8pp::to_v8(isolate, wenviron, length));
	}
	FreeEnvironmentStringsW(environment_strings);
#else
	for (uint32_t i = 0; environ && environ[i]; ++i)
	{
		char const* const pos = strchr(environ[i], '=');
		int const length = pos? int(pos - environ[i]) : -1;
		result->Set(i, v8pp::to_v8(isolate, environ[i], length));
	}
#endif
	info.GetReturnValue().Set(scope.Escape(result));
}

static char const* build_target()
{
#if TARGET(DEBUG)
	return "debug";
#else
	return "release";
#endif
}

static std::string boost_version()
{
	unsigned const BOOST_VERSION_MAJOR = BOOST_VERSION / 100000;
	unsigned const BOOST_VERSION_MINOR = BOOST_VERSION / 100 % 1000;
	unsigned const BOOST_VERSION_PATCH = BOOST_VERSION % 100;

	return (boost::format("%d.%d.%d")
		% BOOST_VERSION_MAJOR % BOOST_VERSION_MINOR % BOOST_VERSION_PATCH).str();
}

static std::string openssl_version()
{
	unsigned const OPENSSL_VERSION_MAJOR  = (OPENSSL_VERSION_NUMBER >> 28) & 0x0F;
	unsigned const OPENSSL_VERSION_MINOR  = (OPENSSL_VERSION_NUMBER >> 20) & 0xFF;
	unsigned const OPENSSL_VERSION_FIX    = (OPENSSL_VERSION_NUMBER >> 12) & 0xFF;
	unsigned const OPENSSL_VERSION_PATCH  = (OPENSSL_VERSION_NUMBER >> 4)  & 0xFF;
	unsigned const OPENSSL_VERSION_STATUS = (OPENSSL_VERSION_NUMBER & 0x0F);

	std::string version_str = (boost::format("%d.%d.%d")
		% OPENSSL_VERSION_MAJOR % OPENSSL_VERSION_MINOR % OPENSSL_VERSION_FIX).str();
	if (OPENSSL_VERSION_PATCH)
	{
		version_str += 'a' + char(OPENSSL_VERSION_PATCH- 1);
	}
	switch (OPENSSL_VERSION_STATUS)
	{
	case 0:
		version_str += "-dev";
		break;
	case 0xF:
		// release
		break;
	default:
		version_str += "-beta";
		version_str += '0' + char(OPENSSL_VERSION_STATUS);
		break;
	}
	return version_str;
}

v8::Handle<v8::Value> versions(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> versions = v8::Object::New(isolate);

	set_option(isolate, versions, "V8", v8::V8::GetVersion());
	set_option(isolate, versions, "Boost", boost_version());
	set_option(isolate, versions, "OpenSSL", openssl_version());
	set_option(isolate, versions, "zlib", ZLIB_VERSION);
	set_option(isolate, versions, "pion", PION_VERSION);

	return scope.Escape(versions);
}

static boost::filesystem::path get_current_path()
{
	return boost::filesystem::current_path();
}

static void set_current_path(boost::filesystem::path const& path)
{
	boost::filesystem::current_path(path);
}

static boost::filesystem::path get_temp_path()
{
	return boost::filesystem::temp_directory_path();
}

static void halt(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	int const exit_code = v8pp::from_v8<int>(args.GetIsolate(), args[0], 0);
#if OS(WINDOWS)
	ExitProcess(exit_code);
#else
	exit(exit_code);
#endif
}

void exit(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	int32_t const exit_code = v8pp::from_v8<int32_t>(isolate, args[0], 0);
	runtime::instance(isolate).terminate(exit_code);
}

static void run_GC(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	int const idle_time = v8pp::from_v8<int>(isolate, args[0], 10);
	isolate->IdleNotification(idle_time);
}

static void reset_rte_include_paths(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	runtime::instance(isolate).reset_rte_include_paths();
}

static void add_rte_include_path(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	boost::filesystem::path const path = v8pp::from_v8<boost::filesystem::path>(isolate, args[0]);
	runtime::instance(isolate).add_rte_include_path(path);
}

static void set_node_lib_path(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	boost::filesystem::path const path = v8pp::from_v8<boost::filesystem::path>(isolate, args[0]);
	runtime::instance(isolate).set_node_lib_path(path);
}

static void register_at_exit_handler(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (!args[0]->IsFunction())
	{
		throw std::invalid_argument("at_exit() expecting function as parameter");
	}
	runtime::instance(isolate).core().at_exit_handler().Reset(isolate, args[0].As<v8::Function>());
}

static void load_library_(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string const name = v8pp::from_v8<std::string>(isolate, args[0]);
	args.GetReturnValue().Set(runtime::instance(isolate).core().load_library(name));
}

static v8::Handle<v8::Object> create_runtime_object(runtime& rt)
{
	v8::Isolate* isolate = rt.isolate();

	v8::EscapableHandleScope scope(isolate);

	/**
	@module runtime Runtime
	Runtime properties and functions

	@property rt {Object}
	JSX runtime properties are accessible in a [global](globals) object `rt`
	**/
	v8pp::module rt_template(isolate);

	/**
	@property rt.local {Object}
	Local JSX runtime properties.
	**/
	v8pp::module local_template(isolate);

	std::string const uuid_system_string = utils::get_unique_local_system_uuid();
	/**
	@property rt.local.systemUuid {String}
	System UUID string, read-only.
	**/
	local_template.set_const("systemUuid", uuid_system_string);

	v8_core::uuid_generator* generator = new v8_core::uuid_generator(uuid_system_string);
	v8::Local<v8::Object> generator_object = v8pp::class_<v8_core::uuid_generator>::import_external(isolate, generator);
	/**
	@property rt.local.systemUuidGenerator {Object}
	System [UUID generator](uuid) instance.
	**/
	local_template.set("systemUuidGenerator", generator_object);

	// if uuid is present - it becomes local
	// else if module name is present, uuid is generated from it
	// else if nothing is available, a random uuid is generated
	using namespace boost::uuids;
	if (rt.uuid().empty() && !rt.script().empty() )
	{
		rt.set_uuid(generator->name(rt.script().string()));
	}
	else if (rt.script().empty())
	{
		random_generator random_gen;
		uuid uuid_local = random_gen();
		rt.set_uuid(to_string(uuid_local));
	}

	std::string const& uuid_local_string = rt.uuid();
	/**
	@property rt.local.uuid {String}
	Process UUID string, read-only.
	**/
	local_template.set_const("uuid", uuid_local_string);

	v8_core::uuid_generator* generator_local = new v8_core::uuid_generator(uuid_local_string);
	v8::Local<v8::Object> generator_object_local = v8pp::class_<v8_core::uuid_generator>::import_external(isolate, generator_local);
	/**
	@property rt.local.uuidGenerator {Object}
	Process [UUID generator](uuid) instance.
	**/
	local_template.set("uuidGenerator", generator_object_local);

	// set script filename & path
	local_template
		/**
		@property rt.local.script {String}
		Full file name of script that is currently running, read-only.
		**/
		.set_const("script", rt.script())

		/**
		@property rt.local.scriptName {String}
		File name of script that is currently running, read-only.
		**/
		.set_const("scriptName", rt.script().filename())

		/**
		@property rt.local.scriptPath {String}
		Path to script that is currently running, read-only.
		**/
		.set_const("scriptPath", rt.script().parent_path())

		/**
		@property rt.local.scriptArgs {Array}
		Array of script arguments passed to JSX process.
		**/
		.set_const("scriptArgs", rt.script_args())

		/**
		@property rt.local.cfgFile {String}
		Runtime configuration file name, read-only, default `rte.cfg`
		**/
		.set_const("cfgFile", rt.cfg_file())

		/**
		@property rt.local.execPath {String}
		Full path to JSX executable, read-only.
		**/
		.set_const("execPath", os::exe_path())

		/**
		@property rt.local.rootPath {String}
		Path to JSX root, read-only.
		**/
		.set_const("rootPath", rt.root_path())

		/**
		@property rt.local.rtePath {String}
		Path to RTE directory, read-only, default #rt.local.rootPath`/rte`
		**/
		.set_const("rtePath", rt.rte_path())

		/**
		@property rt.local.currentPath {String}
		Current working directory.
		Changing this property modifies JSX process working directory.
		**/
		.set("currentPath", v8pp::property(get_current_path, set_current_path))

		/**
		@property rt.local.tempPath {String}
		Path to temporary directory, read-only.
		**/
		.set("tempPath", v8pp::property(get_temp_path))
		;

	/**
	@property rt.module {Object}
	JSX module properties.
	**/
	v8pp::module module_template(isolate);
	module_template
		/**
		@property rt.module.alias {String}
		Module name, read-only.
		**/
		.set_const("alias", rt.script())
		/**
		@property rt.module.uuid {String}
		Module UUID string, read-only.
		**/
		.set_const("uuid", uuid_local_string)
		;

	v8::Local<v8::ObjectTemplate> env_template = v8::ObjectTemplate::New(isolate);
	env_template->SetNamedPropertyHandler(env_get, env_set, env_query,env_del, env_enum);

	rt_template
		/**
		@property rt.arch {String}
		Processor architecture JSX is running on, read-only. One of following values:
		  * `x86`
		  * `x64`
		  * `ARM`
		**/
		.set_const("arch", CURRENT_CPU_STRING)

		/**
		@property rt.platform {String}
		JSX runtime platform, read-only. One of following values:
		  * `windows`
		  * `linux`
		  * `osx`
		  * `ios`
		  * `android`
		  * `freebsd`
		**/
		.set_const("platform", _OS_NAME)

		/**
		@property rt.version {String}
		JSX runtime version string in a major.minor.release format, read-only.
		**/
		.set_const("version", HARMONY_RTE_VERSION)

		/**
		@property rt.versions {Object}
		Used library versions, an object of `libname: version` attributes, read-only.
		**/
		.set_const("versions", versions(isolate))

		/**
		@property rt.buildTarget {String}
		JSX runtime build target, either `debug` or `release`.
		**/
		.set_const("buildTarget", build_target())

		/**
		@property rt.args {Array}
		A read-only array of command line argument string JSX running with.
		**/
		.set_const("args", rt.args())

		/**
		@property rt.env {Object}
		An object of user environment variables. Each object property is an environment
		variable. Changing, deleting or adding a property will change appropriate
		environment variable.
		**/
		.set_const("env", env_template->NewInstance())

		.set("local", local_template)
		.set("module", module_template)

		/**
		@function rt.addRteIncludePath(path)
		@param path {String}
		Append additional RTE search `path`.
		**/
		.set("addRteIncludePath", add_rte_include_path)

		/**
		@function rt.resetRteIncludePaths()
		Remove all additional RTE search paths.
		**/
		.set("resetRteIncludePaths", reset_rte_include_paths)

		/**
		@function rt.setNodeLibPath(path)
		@param path {String}
		Set Node.js lib path and turn on partial API emulation to load Node modules.
		**/
		.set("setNodeLibPath", set_node_lib_path)

		/**
		@function rt.halt([exit_code = 0])
		@param [exit_code=0] {Number}
		Immediate program stop and exit. An optional exit code could be specified.
		**/
		.set("halt", halt)

		/**
		@function rt.exit([exit_code = 0])
		@param [exit_code=0] {Number}
		Graceful program stop and exit. An optional exit code could be specified.
		**/
		.set("exit", exit)

		/**
		@function rt.__atExit(function)
		@param function {Function}
		Register a function to call at #rt.exit
		**/
		.set("__atExit", register_at_exit_handler)
		;

	return scope.Escape(rt_template.new_instance());
}

void core::setup_bindings(v8pp::module& bindings)
{
	/**
	@module v8 V8
	V8 specific functions.
	**/
	v8pp::module v8_template(bindings.isolate());

	v8_template
		/**
		@property version {String}
		V8 version string read-only property.
		**/
		.set_const("version", v8::V8::GetVersion())

		/**
		@function executeInCurrentContext(source [, name [, args]])
		@param source    {String} Script source text.
		@param [name]    {String} Optional script name string.
		@param [args]*   {Object} Optional arguments object to setup in the new context.
		@returns         {Value}  Script execution result.
		Execute JavaScript in the current V8 context.
		**/
		.set("executeInCurrentContext", execute_in_current_context)

		/**
		@function executeInPrivateContext(source [, name [, args]])
		@param source    {String} Script source text.
		@param [name]    {String}  Optional script name string.
		@param [args]*   {Object} Optional arguments object to setup in the new context.
		@returns         {Value}  Script execution result.
		Execute JavaScript in a new V8 context.
		**/
		.set("executeInPrivateContext", execute_in_private_context)

		/**
		@function runGC([idle_time_ms = 10])
		@param [idle_time_ms = 10] {Number} Optional idle time in milliseconds
		Notify V8 about idle to run garbage collection.
		**/
		.set("runGC", run_GC)
		;

	bindings.set("v8", v8_template);
}

core::core(runtime& rt)
	: rt_(rt)
	, require_counter(0)
{
	v8::Isolate* isolate = rt_.isolate();
	v8::HandleScope scope(isolate);

	v8::Local<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate);
	global_template_.Reset(isolate, global_template);

	v8pp::module global(isolate, global_template);

	register_functions(global);
	timer::register_functions(global);

	v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global_template);
	context_.Reset(isolate, context);

	context->Enter();
}

void core::init()
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> global_object = context->Global();

	// get JSON.stringify and JSON.parse functions
	v8::Local<v8::Object> json = global_object->Get(v8pp::to_v8(isolate, "JSON")).As<v8::Object>();
	json_.Reset(isolate, json);
	json_stringify_.Reset(isolate, json->Get(v8pp::to_v8(isolate, "stringify")).As<v8::Function>());
	json_parse_.Reset(isolate, json->Get(v8pp::to_v8(isolate, "parse")).As<v8::Function>());

	/**
	@module globals Globals
	@property global {Object} - Global namespace object
	**/
	set_option(isolate, global_object, "global", global_object);

	/**
	@module bindings Bindings
	Global `bindings` object with native modules
	**/
	v8pp::module bindings(isolate);
	/**
	@function library(name)
	@param name {String} library name
	@return {Value} library instance
	Load native JSX library
	**/
	bindings.set("library", load_library_);
	/**
	@property crypto
	Native cryptogrpahic functions. See crypto#
	**/
	crypto::setup_bindings(bindings);
	/**
	@property console
	Console functions. See console#
	**/
	console::setup_bindings(bindings);
	/**
	@property events
	Native events. See events#
	**/
	event_emitter::setup_bindings(bindings);
	/**
	@property buffer
	Native byte buffer. See buffer#
	**/
	buffer::setup_bindings(bindings);
	/**
	@property logger
	Native logger. See logger#
	**/
	logger::setup_bindings(bindings);
	/**
	@property os
	Native OS functions. See os#
	**/
	os::setup_bindings(bindings);
	/**
	@property fs
	Native filesystem support. See fs#
	**/
	fs::setup_bindings(bindings);
	/**
	@property process
	Native process management. See process#
	**/
	process::setup_bindings(bindings);
	/**
	@property db
	Database support. See db#
	**/
	db::setup_bindings(bindings);
	/**
	@property tcp
	Native TCP support. See tcp#
	**/
	tcp::setup_bindings(bindings);
	/**
	@property http
	HTTP protocol support. See http#
	**/
	http::setup_bindings(bindings);
	/**
	@property timer
	Native timers. See timer#
	**/
	timer::setup_bindings(bindings);
	/**
	@property v8
	V8 related functions. See v8#
	**/
	core::setup_bindings(bindings);
	/**
	@property uuid
	Native UUID support. See uuid#
	**/
	uuid_generator::setup_bindings(bindings);
	/**
	@property xml
	Native XML support. See xml#
	**/
	xml::setup_bindings(bindings);

#if OS(WINDOWS)
	/**
	@property firewall
	Manage firewall rules, Windows only. See firewall#
	**/
	firewall::setup_bindings(bindings);

	/**
	@property netinfo
	Additional information about network adapters, Windows only. See netinfo#
	**/
	setup_netinfo_bindings(bindings);

	/**
	@property registry
	Registry support, Windows only. See registry#
	**/
	registry::setup_bindings(bindings);
#endif

	v8::Local<v8::Object> bindings_object = bindings.new_instance();

	/**
	@module globals
	@property rt
	Reference to the global runtime# instance
	**/
	v8::Local<v8::Object> runtime = create_runtime_object(rt_);
	runtime_.Reset(isolate, runtime);
	set_option(isolate, global_object, "rt", runtime);

	/**
	@module globals
	@property bindings
	Reference to the native bindings#
	**/
	set_option(isolate, runtime, "bindings", bindings_object);
	set_option(isolate, global_object, "bindings", bindings_object);

	require("util");
	require("events");

	/**
	@module globals
	@property console
	Global console# instance
	**/
	set_option(isolate, global_object, "console", require("console"));

	/**
	@module globals
	@property Buffer
	Global buffer# class declaration
	**/
	v8::Handle<v8::Value> buffer_class;
	if (get_option(isolate, bindings_object, "Buffer", buffer_class))
	{
		set_option(isolate, global_object, "Buffer", buffer_class);
	}
}

core::~core()
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if ( !at_exit_handler_.IsEmpty() )
	{
		v8::Local<v8::Function> at_exit_function = v8pp::to_local(isolate, at_exit_handler_);

		v8::TryCatch try_catch;
		at_exit_function->Call(context->Global(), 0, NULL);
		if ( try_catch.HasCaught() )
		{
			report_exception(try_catch);
		}
		at_exit_handler_.Reset();
	}

	json_.Reset();
	json_stringify_.Reset();
	json_parse_.Reset();

	runtime_.Reset();

	script_cache_.clear();
	unload_libraries();

	context->Exit();
	context_.Reset();

	global_template_.Reset();
}

v8::Handle<v8::Value> core::run_script_file(boost::filesystem::path const& filename) const
{
	v8::Isolate* isolate = rt_.isolate();

	boost::iostreams::mapped_file_source script(filename);
	if ( !script.is_open() )
	{
		return v8pp::throw_ex(isolate, "Unable to load script " + filename.string());
	}

	script_container container;
	container.decrypt(script.data(), script.size());

	return run_script(container.data(), container.size(), filename);
}

v8::Handle<v8::Value> core::run_script(char const* source, size_t length, boost::filesystem::path filename) const
{
	v8::Isolate* isolate = rt_.isolate();

	v8::EscapableHandleScope scope(isolate);

	filename.make_preferred();
	std::string const name = filename.empty()? "(undefined)" : filename.string();
	filename = absolute(filename, current_script_path_);

	// compile and run the script
	v8::Local<v8::Object> global = isolate->GetCurrentContext()->Global();
	set_const(isolate, global, "__filename", filename.string());
	set_const(isolate, global, "__dirname", filename.parent_path().string());

	v8::TryCatch try_catch;
	v8::Local<v8::Script> script = v8::Script::Compile(v8pp::to_v8(isolate, source, static_cast<int>(length)),
		v8pp::to_v8(isolate, name.c_str()));
	if ( try_catch.HasCaught() )
	{
		report_exception(try_catch, name.c_str());
	}
	else if ( script.IsEmpty() )
	{
		rt_.warning("Warning: script \'%s\' is empty\n", name.c_str());
	}

	v8::Local<v8::Value> result;
	if ( !script.IsEmpty() )
	{
		v8::TryCatch try_catch;
		result = script->Run();
		if ( try_catch.HasCaught() )
		{
			report_exception(try_catch, name.c_str());
		}
	}

	return scope.Escape(result);
}

v8::Handle<v8::Value> core::load_library(std::string const& name)
{
	if (name.empty())
	{
		throw std::runtime_error("Error: missing library name");
	}

	persistent_map::iterator it = v8_libraries_.find(name);
	if (it == v8_libraries_.end())
	{
		library lib(name);
		v8pp::persistent<v8::Value> v8_lib(rt_.isolate(), lib.startup(rt_.isolate()));

		libraries_.emplace_back(name);
		it = v8_libraries_.emplace_hint(it, name, std::move(v8_lib));
	}
	return v8pp::to_local(rt_.isolate(), it->second);
}

void core::unload_libraries()
{
	// shutdown libraries in reverese order
	std::for_each(libraries_.rbegin(), libraries_.rend(),
		[this](library& lib)
		{
			core::persistent_map::iterator it = v8_libraries_.find(lib.name());
			_aspect_assert(it != v8_libraries_.end());

			if (it != v8_libraries_.end())
			{
				v8::Handle<v8::Value> v8_lib = v8pp::to_local(rt_.isolate(), it->second);
				lib.shutdown(rt_.isolate(), v8_lib);
			}
			//lib.unload();
		});

	libraries_.clear();
	v8_libraries_.clear();
}

void core::set_node_lib_path(boost::filesystem::path const& path)
{
	node_lib_path_ = boost::filesystem::absolute(path, current_script_path_);
	node_support::init(rt_.isolate(), !node_lib_path_.empty());
}

void core::enable_debug(std::string const& module, uint16_t port, bool wait)
{
	(void)module;
	(void)port;
	(void)wait;
//TODO: new debugger support
//	v8::Debug::SetDebugMessageDispatchHandler(dispatch_debug_message, true);
//	v8::Debug::EnableAgent(module.c_str(), port, wait);
}

void core::report_exception(v8::TryCatch const& try_catch, char const* origin) const
{
	if ( origin )
	{
		rt_.error("Exception in %s\n", origin);
	}

	v8::String::Utf8Value const exception(try_catch.Exception());
	v8::Local<v8::Message> const message(try_catch.Message());
	if ( message.IsEmpty() )
	{
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		rt_.error("%s\n", *exception);
	}
	else
	{
		v8::String::Utf8Value const stack(try_catch.StackTrace());

		// Print (filename):(line number): (message).
		v8::String::Utf8Value const filename(message->GetScriptResourceName());
		int const linenum = message->GetLineNumber();

		rt_.error("%s - line %i:\n%s\n", *filename, linenum, (stack.length() > 0? *stack : *exception));

		// Print line of source code.
		v8::String::Utf8Value const sourceline(message->GetSourceLine());
		int start = message->GetStartColumn();
		int end = message->GetEndColumn();

		// replace tab characters to spaces, count tabs before start
		std::string line;
		line.reserve(sourceline.length());
		int tab_count = 0; // before start
		int const tab_size = 4;
		char const* const str = *sourceline;
		for (int i = 0; i < sourceline.length(); ++i)
		{
			if (str[i] == '\t')
			{
				if (i <= start) ++tab_count;
				line.append(tab_size, ' ');
			}
			else
			{
				line.push_back(str[i]);
			}
		}
		rt_.error("%s\n", line.c_str());

		// Print wavy underline (GetUnderline is deprecated).
		if ( start >= 0 )
		{
			// correct start and end to tab_count
			int const tabs_offset = tab_count * (tab_size - 1);
			start += tabs_offset;
			end += tabs_offset;
			std::string const line = std::string(start, ' ') + std::string(end - start, '^');
			rt_.error("%s\n",line.c_str());
		}
	}
}

std::string core::json_str(v8::Handle<v8::Value> value) const
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> json = v8pp::to_local(isolate, json_);
	v8::Local<v8::Function> json_stringify = v8pp::to_local(isolate, json_stringify_);

	v8::Local<v8::Value> result = json_stringify->Call(json, 1, &value);
	v8::String::Utf8Value const str(result);
	return *str;
}

v8::Handle<v8::Value> core::json_parse(std::string const& str) const
{
	v8::Isolate* isolate = rt_.isolate();

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> json = v8pp::to_local(isolate, json_);
	v8::Local<v8::Function> json_parse = v8pp::to_local(isolate, json_parse_);
	v8::Local<v8::Value> value = v8pp::to_v8(isolate, str);

	v8::TryCatch try_catch;
	v8::Local<v8::Value> result = json_parse->Call(json, 1, &value);
	if (try_catch.HasCaught())
	{
		result = value;
	}
	return scope.Escape(result);
}

}} // aspect::v8_core
