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
#include "jsx/fs.hpp"

#include "jsx/runtime.hpp"
#include "jsx/v8_buffer.hpp"

#if ENABLE(DIRECTORY_MONITOR)
#include "jsx/directory_monitor.hpp"
#endif

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

namespace aspect { namespace fs {

boost::filesystem::path v8_path::from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
	boost::filesystem::path result;
	if (value->IsString())
	{
#if OS(WINDOWS)
		v8::String::Value const str(value);
#else
		v8::String::Utf8Value const str(value);
#endif
		result = boost::filesystem::path(reinterpret_cast<boost::filesystem::path::value_type const*>(*str));
		result.make_preferred();
	}
	else if (v8_path const* src = v8pp::from_v8<v8_path*>(isolate, value))
	{
		result = static_cast<boost::filesystem::path const&>(*src);
	}
	return result;
}

v8::Handle<v8::Value> v8_path::to_v8(v8::Isolate* isolate, boost::filesystem::path const& p)
{
	return v8pp::to_v8(isolate, p.native());
}

void v8_path::string(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), path_));
}

void v8_path::parent_path(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), path_.parent_path()));
}

void v8_path::stem(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), path_.stem()));
}

void v8_path::filename(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), path_.filename()));
}

void v8_path::extension(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), path_.extension()));
}

static void current_directory(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set(v8_path::to_v8(isolate, boost::filesystem::current_path()));
}

static void absolute(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const p(isolate, args[0]);
	if (p.empty())
	{
		throw std::runtime_error("require path argument");
	}

	args.GetReturnValue().Set(v8_path::to_v8(isolate, boost::filesystem::absolute(p)));
}

static void resolve(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const p(isolate, args[0]);
	if (p.empty())
	{
		throw std::runtime_error("require path argument");
	}

	args.GetReturnValue().Set(v8_path::to_v8(isolate, os::resolve(p)));
}

static void enumerate_directory(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8_path const source_dir(isolate, args[0]);
	v8::Local<v8::Function> fn = args[1].As<v8::Function>();

	if (source_dir.empty() || !fn->IsFunction())
	{
		throw std::runtime_error("require directory and function arguments");
	}

	if (boost::filesystem::exists(source_dir))
	{
		using boost::filesystem::directory_iterator;
		for (directory_iterator iter(source_dir), end; iter != end ; ++iter)
		{
			v8_path* p = new v8_path(iter->path());
			v8::Handle<v8::Value> argv[1] = { v8pp::class_<v8_path>::import_external(isolate, p) };

			v8::TryCatch try_catch;
			fn->Call(args.This(), 1, argv);
			if (try_catch.HasCaught())
			{
				runtime::instance(isolate).core().report_exception(try_catch);
				// what to do here?  break???
			}
		}
	}
}

static void read_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8_path const filename(isolate, args[0]);
	if (filename.empty())
	{
		throw std::runtime_error("require file name argument");
	}

	boost::filesystem::ifstream file(filename, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		uint64_t const file_size = boost::filesystem::file_size(filename);
		std::unique_ptr<v8_core::buffer> file_buffer(new v8_core::buffer(nullptr, file_size));
		if (file_size)
		{
			file.read(file_buffer->data(), (std::streamsize)file_size);
		}
		v8::Local<v8::Value> result  = v8pp::class_<v8_core::buffer>::import_external(isolate, file_buffer.release());
		args.GetReturnValue().Set(scope.Escape(result));
	}
}

void write_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8_path const filename(isolate, args[0]);
	if (filename.empty() || args.Length() < 2 )
	{
		throw std::runtime_error("require file name and buffer or string as arguments");
	}

	v8_core::buffer const* buf = nullptr;
	std::string str;

	char const* data;
	size_t size;

	if (args[1]->IsString())
	{
		str = v8pp::from_v8<std::string>(isolate, args[1]);
		data = str.data();
		size = str.size();
	}
	else if (buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]))
	{
		data = buf->data();
		size = buf->size();
	}
	else
	{
		throw std::invalid_argument("second argument must be a string or buffer object");
	}

	bool const append = v8pp::from_v8<bool>(isolate, args[2], false);

	boost::filesystem::ofstream file(filename, std::ios::binary | (append? std::ios::app : std::ios::trunc));

	bool const result = file.is_open() && file.write(data, size).good();
	args.GetReturnValue().Set(result);
}

static void read_lines(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	v8_path const filename(isolate, args[0]);
	if (filename.empty())
	{
		throw std::runtime_error("require file name argument");
	}

	boost::filesystem::ifstream file(filename, std::ios::in);
	if (file.is_open())
	{
		v8::Local<v8::Array> lines = v8::Array::New(isolate);
		std::string line;
		for (uint32_t i = 0; std::getline(file, line); ++i)
		{
			lines->Set(i, v8pp::to_v8(isolate, line));
		}
		args.GetReturnValue().Set(scope.Escape(lines));
	}
}

inline static bool write_line(std::ostream& os, v8::Handle<v8::Value> str)
{
	char const endl = '\n';
	v8::String::Utf8Value const line(str);

	return os.write(*line, line.length()).write(&endl, 1).good();
}

void write_lines(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8_path const filename(isolate, args[0]);
	if (filename.empty() || args.Length() < 2 )
	{
		throw std::runtime_error("require file name and buffer or string as arguments");
	}

	bool const append = v8pp::from_v8<bool>(isolate, args[2], false);

	boost::filesystem::ofstream file(filename, (append? std::ios::app : std::ios::trunc));
	bool result = file.is_open();

	if (result && args[1]->IsArray())
	{
		v8::Local<v8::Array> lines = args[1].As<v8::Array>();
		for (uint32_t i = 0, count = lines->Length(); i < count; ++i)
		{
			if (!write_line(file, lines->Get(i)))
			{
				break;
			}
		}
		result = true;
	}
	else
	{
		result = write_line(file, args[1]);
	}

	args.GetReturnValue().Set(result);
}

static void is_directory(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	args.GetReturnValue().Set(boost::filesystem::is_directory(path));
}

static void is_regular_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	args.GetReturnValue().Set(boost::filesystem::is_regular_file(path));
}

static void is_symlink(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	args.GetReturnValue().Set(boost::filesystem::is_symlink(path));
}

static void is_other(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	args.GetReturnValue().Set(boost::filesystem::is_other(path));
}

static void exists(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	args.GetReturnValue().Set(boost::filesystem::exists(path));
}

static void create_directory(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	bool const recursive = v8pp::from_v8<bool>(isolate, args[1], false);

	recursive? boost::filesystem::create_directories(path) :
		boost::filesystem::create_directory(path);
}

static void rename(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const src(isolate, args[0]);
	v8_path const dst(isolate, args[1]);
	if (src.empty() ||dst.empty())
	{
		throw std::runtime_error("require string or path for source and destination");
	}

	boost::filesystem::rename(src, dst);
}

static void remove(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const path(isolate, args[0]);
	if (path.empty())
	{
		throw std::runtime_error("require string or path");
	}

	bool const recursive = v8pp::from_v8<bool>(isolate, args[1], false);

	recursive? boost::filesystem::remove_all(path) :
		boost::filesystem::remove(path);
}

static void copy(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8_path const src(isolate, args[0]);
	v8_path const dst(isolate, args[1]);
	if (src.empty() ||dst.empty())
	{
		throw std::runtime_error("require string or path for source and destination");
	}

	boost::filesystem::copy(src, dst);
}

#if ENABLE(ASYNC_FS_FUNCTIONS)

#if OS(WINDOWS)
static void on_file_write_complete_v8(
	boost::asio::windows::random_access_handle* file,
	aspect::v8_core::buffer* buf,
	v8::Isolate* isolate, v8::Persistent<v8::Object> self, v8::Persistent<v8::Function> callback,
	boost::system::error_code const& err, size_t bytes_transferred)
{
	// clean up resources
	file->close();
	delete file;

	v8::HandleScope scope(isolate);

	v8::Handle<v8::Value> argv[1];
	if ( err || bytes_transferred != buf->size() )
	{
		argv[0] = v8pp::throw_ex(isolate, err.message());
	}
	else
	{
		argv[0] = v8pp::to_v8(isolate, bytes_transferred);
	}

	v8::TryCatch try_catch;
	v8pp::to_local(isolate, callback)->Call(v8pp::to_local(isolate, self), 1, argv);
	if ( try_catch.HasCaught() )
	{
		runtime::instance(isolate).core().report_exception(try_catch);
	}
	callback.Reset();
	self.Reset();
}

static void on_file_write_complete(
	boost::asio::windows::random_access_handle* file,
	aspect::v8_core::buffer* buf,
	v8::Isolate* isolate, v8::Persistent<v8::Object> self, v8::Persistent<v8::Function> callback,
	boost::system::error_code const& err, size_t bytes_transferred)
{
	runtime::instance(isolate).main_loop().schedule(boost::bind(&on_file_write_complete_v8,
		file, buf, isolate, self, callback, err, bytes_transferred));
}
#endif

static void async_write_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

#if OS(WINDOWS)
	v8::HandleScope scope(isolate);

	if (args.Length() < 3)
	{
		throw std::invalid_argument("usage: async_write_file(filename, data, completion_callback)");
	}

	// Check and unpack arguments
	v8_path const filename(isolate, args[0]);
	if (filename.empty())
	{
		throw std::invalid_argument("first argument must be a file name");
	}

	v8_core::buffer* buf = nullptr;
	if (args[1]->IsString())
	{
		v8::String::Utf8Value const str(args[1]);
		buf = new v8_core::buffer(*str, str.length());
		v8pp::class_<v8_core::buffer>::import_external(isolate, buf);
	}
	else
	{
		buf = v8pp::from_v8<v8_core::buffer*>(isolate, args[1]);
		if (!buf)
		{
			throw std::invalid_argument("second argument must be a string or buffer object");
		}
	}

	if (!args[2]->IsFunction())
	{
		throw std::invalid_argument("3rd argument must be a completion function");
	}

	// may be reqired an exta v8 schedule call to convert callback into	Persistent<Function>
		//v8::Handle<v8::Value> callback = args[2];
	v8::Persistent<v8::Function> callback(isolate, args[2].As<v8::Function>());
	v8::Persistent<v8::Object> self(isolate, args.This()->ToObject());

	boost::filesystem::path const& fn = static_cast<boost::filesystem::path const&>(filename);

	// using Windows HANDLE for file
	HANDLE fh = ::CreateFileW(fn.c_str(), GENERIC_WRITE, 0, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	if (fh == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("can't open file" +  fn.string());
	}

	auto file = new boost::asio::windows::random_access_handle(runtime::instance(isolate).io_service(), fh);

	boost::asio::async_write_at(*file, 0, boost::asio::buffer(buf->data(), buf->size()),
		boost::bind(on_file_write_complete, file, buf, isolate, self, callback,
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#else
#warning Implemented in Windows only yet
#endif
}

#endif // ENABLE(ASYNC_FS_FUNCTIONS)

//////////////////////////////////////////////////////////////////////////

void setup_bindings(v8pp::module& target)
{
	/**
	@module fs
	**/
	v8pp::module fs_module(target.isolate());

	fs_module
		/**
		@function currentDirectory()
		@return {Path}

		Get current working directory. See also runtime#rt.local.currentPath
		**/
		.set("currentDirectory", current_directory)

		/**
		@function absolute(path)
		@param path {String|Path}
		@return {Path}

		Convert `path` to an absolute path. If `path` is relatve, #currentDirectory
		is used as a base.
		**/
		.set("absolute", absolute)

		/**
		@function resolve(path)
		@param path {String|Path}
		@return {Path}

		Resolve `path` to a path without `.` and `..`
		**/
		.set("resolve", resolve)

		/**
		@function enumerateDirectory(path, callback)
		@param path {String|Path}
		@param callback {Function}

		Enumerate contents of directory at `path` using `callabck` function
		for each directory entry.
		**/
		.set("enumerateDirectory", enumerate_directory)

		/**
		@function readFile(filename)
		@param filename {String|Path}
		@return {Buffer}

		Read file contents into a buffer.
		**/
		.set("readFile", read_file)

		/**
		@function writeFile(filename, data [, append = false])
		@param filename {String|Path}
		@param data {String|Buffer}
		@param [append=false] {Boolean]
		@return {Boolean} `true` on success

		Write `data` into the file. If optional argument `append` is true,
		append data to the end of file.
		**/
		.set("writeFile", write_file)

		/**
		@function readLines(filename)
		@param filename {String|Path}
		@return {Array}

		Read text file as an array of strings.
		**/
		.set("readLines", read_lines)

		/**
		@function writeLines(filename, lines [, append = false])
		@param filename {String|Path}
		@param lines {String|Array}
		@param [append=false] {Boolean]

		Write `lines` array into the file. If optional argument `append` is true,
		append data to the end of file.
		**/
		.set("writeLines", write_lines)

#if ENABLE(ASYNC_FS_FUNCTIONS)
		// undocumented yet
		.set("asyncWriteFile", async_write_file)
#endif

		/**
		@function isDirectory(path)
		@param path {String|Path}
		@return {Boolean}

		Check if the `path` is a directory
		**/
		.set("isDirectory", is_directory)

		/**
		@function isRegularFile(path)
		@param path {String|Path}
		@return {Boolean}

		Check if the `path` is a regular file
		**/
		.set("isRegularFile", is_regular_file)

		/**
		@function isSymlink(path)
		@param path {String|Path}
		@return {Boolean}

		Check if the `path` is a symbolic link
		**/
		.set("isSymlink", is_symlink)

		/**
		@function isOther(path)
		@param path {String|Path}
		@return {Boolean}

		Check if the `path` neither a directory, nor a regular file,
		nor a symbolic link
		**/
		.set("isOther", is_other)

		/**
		@function exists(path)
		@param path {String|Path}
		@return {Boolean}

		Check if the `path` is exist
		**/
		.set("exists", exists)

		/**
		@function createDirectory(path [, recursive = false])
		@param path {String|Path}
		@param [recursive=false] {Boolean}
		Create a directory `path`
		If optional parameter `recursive` is true, create full directory path.
		**/
		.set("createDirectory", create_directory)

		/**
		@function rename(old_path, new_path)
		@param old_path {String|Path}
		@param new_path {String|Path}

		Rename filesystem item with `old_path` to the `new_path`
		**/
		.set("rename", rename)

		/**
		@function remove(path [, recursive = false])
		@param path {String|Path}
		@param [recursive=false] {Boolean}

		Remove filesystem item at `path`
		If optional parameter `recursive` is true, remove recursively
		the contents of `path` then delete `path` itself.
		**/
		.set("remove", remove)

		/**
		@function copy(from, to)
		@param from {String|Path}
		@param to {String|Path}

		Copy filesystem contents from one place to another.
		**/
		.set("copy", copy)
		;

	/**
	@class Path
	Filesystem path

	@function Path([path])
	@param [path] {String|Path}
	@return {Path}
	Create a filesystem path instance, copy it from optional `path` parameter.
	**/
	v8pp::class_<v8_path> path_class(target.isolate(), v8pp::v8_args_ctor);
	path_class
		/**
		@function empty()
		@return {Boolean}
		Check if the path empty
		**/
		.set("empty", &v8_path::empty)

		/**
		@function toString()
		@return {String}
		Return filesystem path as a string
		**/
		.set("toString", &v8_path::string)

		/**
		@function parentPath()
		@return {String}
		Return parent path
		**/
		.set("parentPath", &v8_path::parent_path)

		/**
		@function filename()
		@return {String}
		Return filename
		**/
		.set("filename", &v8_path::filename)

		/**
		@function extension()
		@return {String}
		Return extension
		**/
		.set("extension", &v8_path::extension)

		/**
		@function stem()
		@return {String}
		Return filename without extension
		**/
		.set("stem", &v8_path::stem)
		;
	fs_module.set("Path", path_class);

#if ENABLE(DIRECTORY_MONITOR)
	/**
	@class DirectoryMonitor

	Monitor changes in specified directories.

	Directory monitor watches for changes in specfied directories
	and theirs subdirectories with optional path name filtering, based on
	regular expressions.

	Monitor event has following attributes:
	  * `type` Event type. One of the following values:
	     * `null`
	     * `added`
	     * `removed`
	     * `modified`
	     * `renamed_old_name`
	     * `renamed_new_name`
	  * `path`  Full event path, #Path
	**/
	v8pp::class_<directory_monitor> directory_monitor_class(target.isolate(), v8pp::v8_args_ctor);

	using boost::asio::dir_monitor_event;
	directory_monitor_class
		.set_const("null", dir_monitor_event::null)
		.set_const("added", dir_monitor_event::added)
		.set_const("removed", dir_monitor_event::removed)
		.set_const("modified", dir_monitor_event::modified)
		.set_const("renamed_old_name", dir_monitor_event::renamed_old_name)
		.set_const("renamed_new_name", dir_monitor_event::renamed_new_name)

		/**
		@function add(dir [, watch_subdirs = true [, filter_regexp]])
		@param dir {String|Path}
		@param [watch_subdirs=true] {Boolean}
		@param [filter_regexp] {String}
		Add directory `dir` for monitoring.
		Optional `watch_subdirs` and `filter_regexp` parameters could be specified.
		**/
		.set("add", &directory_monitor::add_dir)

		/**
		@function remove(dir)
		@param dir {String|Path}
		Remove directory `dir` from monitoring list.
		**/
		.set("remove", &directory_monitor::remove_dir)

		/**
		@function dirs()
		@return {Array} of Pathes
		Get list of directories that are being monitored.
		**/
		.set("dirs", &directory_monitor::dirs)

		/**
		@function monitor([callback])
		@param callback {Function}
		@return {Event|Undefined}

		Start directories monitoring.
		If no `callback` function is specfied, wait for any `Event` from the monitor
		and return it. Otherwise start asynchronous monitoring and return immediately.

		@event callback(err, event)
		@param err {Error} undefined on success
		@param event {Event}
		**/
		.set("monitor", &directory_monitor::monitor)

		/**
		@function stop()
		Stop  directories monitoring.
		**/
		.set("stop", &directory_monitor::stop)
		;
	fs_module.set("DirectoryMonitor", directory_monitor_class);
#endif

	target.set("fs", fs_module.new_instance());
}


}} // aspect::fs
