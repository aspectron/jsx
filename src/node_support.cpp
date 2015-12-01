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
#include "jsx/node_support.hpp"

#include <boost/iostreams/device/mapped_file.hpp>

#include <v8pp/call_v8.hpp>

#include "jsx/runtime.hpp"
#include "jsx/v8_main_loop.hpp"
#include "jsx/console.hpp"

namespace aspect { namespace v8_core {

void trace(v8::FunctionCallbackInfo<v8::Value> const& args);
void exit(v8::FunctionCallbackInfo<v8::Value> const& args);
v8::Handle<v8::Value> versions(v8::Isolate* isolate);

}}

namespace aspect { namespace node_support {

static v8::Handle<v8::Value> define_constants(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	v8pp::module constants(isolate);

	return scope.Escape(constants.new_instance());
}

static v8::Handle<v8::Value> define_natives(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> natives = v8::Object::New(isolate);

	using namespace boost::filesystem;
	for (directory_iterator it(runtime::instance(isolate).core().node_lib_path()), end; it != end; ++it)
	{
		if (it->status().type() == regular_file)
		{
			path const& p = it->path();
			boost::iostreams::mapped_file_source script(p);
			set_option(isolate, natives, p.filename().string().c_str(),
				v8pp::to_v8(isolate, script.data(), static_cast<int>(script.size())));
		}
	}

	return scope.Escape(natives);
}

static v8::Handle<v8::Value> define_evals(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	v8pp::module script(isolate);
	script.set("runInThisContext", v8_core::execute_in_current_context);
	script.set("runInNewContext", v8_core::execute_in_private_context);

	v8pp::module evals(isolate);
	evals.set("NodeScript", script);

	return scope.Escape(evals.new_instance());
}

static void binding(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	std::string const name = v8pp::from_v8<std::string>(isolate, args[0], "");

	static v8::Persistent<v8::Object> bindings;

	if (bindings.IsEmpty())
	{
		v8pp::module bindings_template(isolate);
		bindings_template
			.set("constants", define_constants(isolate))
			.set("natives", define_natives(isolate))
			.set("evals", define_evals(isolate))
			;
		bindings.Reset(isolate, bindings_template.new_instance());
	}

	v8::Local<v8::Value> binding = v8pp::to_local(isolate, bindings)->Get(args[0]);
	if (binding.IsEmpty() || binding == Undefined(isolate))
	{
		binding = runtime::instance(isolate).core().require(name);
	}

	args.GetReturnValue().Set(scope.Escape(binding));
}

static v8::Persistent<v8::Object> process; //TODO: make per runtime

static void chdir(std::string const& new_dir)
{
	boost::system::error_code ec;
	boost::filesystem::current_path(new_dir, ec);
	if (ec)
	{
		throw boost::filesystem::filesystem_error("chdir", new_dir, ec);
	}
}

static boost::filesystem::path::string_type cwd()
{
	boost::system::error_code ec;
	boost::filesystem::path const curr = boost::filesystem::current_path(ec);
	if (ec)
	{
		throw boost::filesystem::filesystem_error("cwd", ec);
	}
	return curr.native();
}

static void memory_usage(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8::HeapStatistics hs;
	isolate->GetHeapStatistics(&hs);

	v8::Local<v8::Object> heap_info = v8::Object::New(isolate);
	set_option(isolate, heap_info, "heapTotal", hs.total_heap_size());
	set_option(isolate, heap_info, "heapExecutable", hs.total_heap_size_executable());
	set_option(isolate, heap_info, "heapPhysical", hs.total_physical_size());
	set_option(isolate, heap_info, "heapUsed", hs.used_heap_size());
	set_option(isolate, heap_info, "heapLimit", hs.heap_size_limit());
	
	args.GetReturnValue().Set(scope.Escape(heap_info));
}

static void next_tick(v8::Isolate* isolate, v8::Handle<v8::Function> callback)
{
	struct tick
	{
		v8::Isolate* isolate;

		// Using shared_ptr<Persistent<Function>> for reference counting
		//TODO: review this code after updating to recent V8 version
		// because Persistent sematics might be changed
		boost::shared_ptr<v8::Persistent<v8::Function>> cb;

		explicit tick(v8::Isolate* isolate, v8::Handle<v8::Function> cb)
			: isolate(isolate)
			, cb(boost::make_shared<v8::Persistent<v8::Function>>(isolate, cb))
		{
		}

		void operator()()
		{
			v8pp::call_v8(isolate, v8pp::to_local(isolate, *cb));
		}
	};

	runtime::instance(isolate).main_loop().schedule(tick(isolate, callback));
}

static unsigned max_tick_depth(v8::Isolate* isolate)
{
	return runtime::instance(isolate).main_loop().max_callbacks();
}

static void set_max_tick_depth(v8::Isolate* isolate, unsigned max_depth)
{
	runtime::instance(isolate).main_loop().set_max_callbacks(max_depth);
}

using namespace boost::chrono;
typedef high_resolution_clock clock;

static clock::time_point const start_time = clock::now();

static double uptime()
{
	return duration<double>(clock::now() - start_time).count();
}

static void hrtime(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	clock::duration duration = clock::now().time_since_epoch();

	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	int64_t secs, nanos;

	if (args[0]->IsArray())
	{
		// diff from previous hrtime() call
		v8::Local<v8::Array> arr = args[0].As<v8::Array>();
		secs = v8pp::from_v8<int64_t>(isolate, arr->Get(0), 0);
		nanos = v8pp::from_v8<int64_t>(isolate, arr->Get(1), 0);
		duration -= seconds(secs);
		duration -= nanoseconds(nanos);
	}

	v8::Local<v8::Array> arr = v8::Array::New(isolate, 2);
	secs = static_cast<int64_t>(duration_cast<seconds>(duration).count());
	nanos = static_cast<int64_t>(duration_cast<nanoseconds>(duration - seconds(secs)).count());

	arr->Set(0, v8pp::to_v8(isolate, secs));
	arr->Set(1, v8pp::to_v8(isolate, nanos));

	args.GetReturnValue().Set(scope.Escape(arr));
}

// wrap standard function to hide additional signature attributes
void abort()
{
	::abort();
}

void init(v8::Isolate* isolate, bool enable)
{
	if (enable && process.IsEmpty())
	{
		v8::HandleScope scope(isolate);

		v8::Local<v8::Object> global = isolate->GetCurrentContext()->Global();

		v8::Local<v8::Object> runtime;
		get_option(isolate, global, "rt", runtime);

		v8::Local<v8::Value> env;
		get_option(isolate, runtime, "env", env);

		v8::Local<v8::Object> jsx_bindings;
		v8::Local<v8::Object> jsx_console;
		get_option(isolate, global, "bindings", jsx_bindings);
		get_option(isolate, jsx_bindings, "console", jsx_console);

		v8::Local<v8::Value> jsx_stdin, jsx_stdout, jsx_stderr;
		get_option(isolate, jsx_console, "stdin", jsx_stdin);
		get_option(isolate, jsx_console, "stdout", jsx_stdout);
		get_option(isolate, jsx_console, "stderr", jsx_stderr);

		v8pp::module process_template(isolate);
		process_template
			.set("binding", binding)
			.set("moduleLoadList", v8::Array::New(isolate))
			.set_const("stdin",  jsx_stdin)
			.set_const("stdout", jsx_stdout)
			.set_const("stderr", jsx_stderr)
			.set_const("argv", runtime::instance(isolate).args())
			.set_const("execPath", os::exe_path().string())
			.set_const("env", env)
			.set_const("version", HARMONY_RTE_VERSION)
			.set_const("versions", v8_core::versions(isolate))
			.set_const("arch", CURRENT_CPU_STRING)
			.set_const("platform", _OS_NAME)
			.set("title", v8pp::property(console::get_title, console::set_title))
			.set("abort", &abort)
			.set("exit", v8_core::exit)
			.set("chdir", chdir)
			.set("cwd", cwd)
			.set("memoryUsage", memory_usage)
			.set("nextTick", next_tick)
			.set("maxTickDepth", v8pp::property(max_tick_depth, set_max_tick_depth))
			.set("uptime", uptime)
			.set("hrtime", hrtime)
			;

		v8::Local<v8::Object> proc = process_template.new_instance();

		set_option(isolate, global, "process", proc);
		set_option(isolate, global, "GLOBAL", global);
		set_option(isolate, global, "root", global);

		process.Reset(isolate, proc);
	}
}

}} //aspect::node_support
