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
#include "jsx/db.hpp"

#include "jsx/v8_buffer.hpp"
#include "jsx/async_queue.hpp"
#include "jsx/runtime.hpp"
#include "jsx/v8_main_loop.hpp"

namespace aspect { namespace db {

void setup_bindings(v8pp::module& target)
{
	v8pp::module db_module(target.isolate());

	/**
	@module db

	@class Session

	Database session class

	@function Session([config])
	@param [config] {String}
	Create a new SQLite session. If `config` string is provided open the
	database session. If no database backend is specified in the `config`,
	an SQLite database would be created in #storageLocation`/config.db`

	Configuration string could have following parts:
	  1. Driver Name separated from rest of the string with ":"
	  2. Set of `key = value` pairs separated with ";" symbol. Values can be
	  put in single quotation marks ' and double quotation mark represents a
	  single one (like in SQL syntax).

	Allowed driver names are:
	  * `sqlite3`
	  * `mysql`
	  * `postgres`
	  * `odbc`

	See reference to the database engine for connection string properties.

	SQLite connection properties are:
	  * `db` - Path to sqlite3 database file, special name ":memory:"
	  can be used as well for in-memory database.
	  * `mode` - File open mode: `create`, `readonly` or `readwrite`,
	  default value is `create`.
	  * `busy_timeout` - Minimal number of milliseconds to wait before
	  returning a error if the database is locked by another process.

	MySQL connection properties are:
	  * `host` - Database host to connect
	  * `port` - Port to connect
	  * `database` - Database name to use
	  * `user` - The user name to login
	  * `password` - The password to login

	The most used properties for PostgreSQL are:
	  * `host` - Database host to connect
	  * `port` - Port to connect
	  * `dbname` - Database name to use
	  * `user` - The user name to login
	  * `password` - The password to login

	**/
	v8pp::class_<session> session_class(target.isolate(), v8pp::v8_args_ctor);
	session_class
		/**
		@function open(config [, callback])
		@param config {String}
		@param [callback] {Function}
		Open database session with `config` connection string.
		If `callback` function is specified, open the session asynchronously.

		@event callback(err)
		@param err {Error} undefined on success
		**/
		.set("open", &session::open)

		/**
		@function close([callback])
		@param [callback] {Function}
		Close the session.
		If `callback` function is specified, close the session asynchronously.

		@event callback(err)
		@param err {Error} undefined on success
		**/
		.set("close", &session::close)

		/**
		@function isOpen()
		@return {Boolean}
		Check if the database session is open
		**/
		.set("isOpen", &session::is_open)

		/**
		@function prepare(query [, values...] [, callback])
		@param query {String}
		@param [values]* {Value}
		@param callback {Function}
		@return {Statement|Undefined}
		Prepare a statement for SQL `query`.
		Optional `values` arguments will be bound into the `query`
		If `callback` function is specified, prepare the statement asynchronously,
		otherwise prepare and return the statement.

		@event callback(err, statement)
		@param err {Error} undefined on success
		@param statement {Statement} prepared statement
		**/
		.set("prepare", &session::prepare)

		/**
		@function exec(query [, values...] [, callback])
		@param query {String}
		@param [values]* {Value}
		@param callback {Function}
		Execute a statement with SQL `query`.
		Optional `values` arguments will be bound into the `query`
		If `callback` function is specified, execute the statement asynchronously.

		@event callback(err, info, result)
		@param err {Error} undefined on success
		@param info {Object} exec information
		@param result {Object} select result

		Exec information object has attributes:
		  * `lastInsertId` last used ID for inserted row
		  * `rowsAffected` number of rows updated or deleted
		**/
		.set("exec", &session::exec)

		/**
		@function begin()
		Begin a new transaction
		**/
		.set("begin", &session::begin)

		/**
		@function commit()
		Commit the current transaction
		**/
		.set("commit", &session::commit)

		/**
		@function rollback()
		Rollback the current transaction
		**/
		.set("rollback", &session::rollback)

		/**
		@function escape(str)
		@param str {String}
		@return {String}
		Escape `str` for inclusion in SQL statement.
		**/
		.set("escape", &session::escape)
		;

	/**
	@class Statement

	Database statement

	@function Statement(session, query)
	@param session {Session}
	@param query {String}
	@return {Statement}
	Create a new statement in the database `session` for the SQL `query`
	**/
	v8pp::class_<statement> statement_class(target.isolate(), v8pp::ctor<cppdb::session&, std::string const&>());
	statement_class
		/**
		@function query()
		@return {Result}
		Fetch a result of the statement query
		**/
		.set("query", &statement::query)

		/**
		@function row()
		@return {Result}
		Fetch a single row from the statement query
		**/
		.set("row", &statement::row)

		/**
		@function exec()
		Execute the statement
		**/
		.set("exec", &statement::exec)

		/**
		@function reset()
		Reset the statement into initial state
		**/
		.set("reset", &statement::reset)

		/**
		@function lastInsertId()
		@return {Number}
		Get last insert id from the last executed statement
		**/
		.set("lastInsertId", &statement::last_insert_id)

		/**
		@function affected()
		@return {Number}
		Get number of affected rows by the last executed statement
		**/
		.set("affected", &statement::affected)

		/**
		@function bind(values)
		@param values* {Value}
		@return {Statement} `this` to chain calls
		Bind `values` to the statement query.
		Bind each of the `values` to the next placeholder number (starting from 1).
		Placeholders are marked with '?' in the query string.
		**/
		.set("bind", &statement::bind)

		/**
		@function bindNth(column, values)
		@param column {Number}
		@param values* {Value}
		@return {Statement} `this` to chain calls
		Bind `values` to the statement query starting from Nth `column`.
		Bind each of the `values` to the next placeholder number starting from `column`.
		Placeholders are marked with '?' in the query string.
		**/
		.set("bindNth", &statement::bind_nth)
		;

	/**
	@class Result
	Statement result
	**/
	v8pp::class_<result> result_class(target.isolate(), v8pp::no_ctor);
	result_class
		/**
		@function cols()
		@return {Number}
		Get number of columns in the result
		**/
		.set("cols", &result::cols)

		/**
		@function index(name)
		@param name {String}
		@return {Number}
		Get column index by its `name`. Throws exception if no such name in the result
		**/
		.set("index", &result::index)

		/**
		@function findColumn(name)
		@param name {String}
		@return {Number}
		Find column index by its `name`. Returns `-1` if no such name in the result
		**/
		.set("findColumn", &result::find_column)

		/**
		@function name(index)
		@param index {Number}
		@return {String}
		Get column name by its `index`. Throws exception if `index` is not
		in range [0 .. cols()]
		**/
		.set("name", &result::name)

		/**
		@function next()
		@return {Boolean}
		Move forward to next row in the result. Return false if there are no more rows.
		**/
		.set("next", &result::next)

		/**
		@function clear()
		Clear the result. No further use of the result should be done with the result.
		**/
		.set("clear", &result::clear)

		/**
		@function empty()
		@return {Boolean}
		Check if the current row is empty. Return `true` in these cases:
		  1. Empty result
		  2. next() wasn't called first time
		  3. next() returned false
		**/
		.set("empty", &result::empty)

		/**
		@function value(column)
		@param column {Number|String}
		@return {Value}
		Get `column` value in the result current row.
		Column may be an index or name.
		**/
		.set("value", &result::value)

		// deprecated
		.set("values", &result::as_array)
		.set("data", &result::as_object)

		/**
		@function asArray()
		@return {Array}
		Return current row as array of values
		**/
		.set("asArray", &result::as_array)

		/**
		@function asObject()
		@return {Object}
		Return current row as object of name: value pairs
		**/
		.set("asObject", &result::as_object)

		/**
		@function asArrayOfArrays([limit=Infinity] [, callback])
		@param [limit=Infinity] {Number}
		@param [callback] {Function}
		@return {Array|Undefined}
		Return rest of the result as array of arrays.
		Optional number of rows could be set to limit the return array length.
		If `callback` function is specified, get the result asynchronously.

		@event callback(err, result)
		@param err {Error} undefined on success
		@param result {Array}
		**/
		.set("asArrayOfArrays", &result::as_array_of_arrays)

		/**
		@function asArrayOfObjects([limit=Infinity] [, callback])
		@param [limit=Infinity] {Number}
		@param [callback] {Function}
		@return {Array|Undefined}
		Return rest of the result as array of objects with name: value pairs.
		Optional number of rows could be set to limit the return array length.
		If `callback` function is specified, get the result asynchronously.

		@event callback(err, result)
		@param err {Error} undefined on success
		@param result {Array}
		**/
		.set("asArrayOfObjects", &result::as_array_of_objects)
		;

	db_module
		.set("_session", session_class) // wrapped by Session in db.js RTE library
		.set("Statement", statement_class)
		.set("Result", result_class)
		;

	target.set("db", db_module.new_instance());
}

/////////////////////////////////////////////////////////////////////////////
//
// session
//

struct async_operation : boost::noncopyable
{
	session* sess;
	std::string query;
	boost::scoped_ptr<v8_core::callback> cb;
	v8_values cb_args;

	async_operation(session* sess, v8::FunctionCallbackInfo<v8::Value> const& args)
		: sess(sess)
	{
		v8::Isolate* isolate = args.GetIsolate();

		query = v8pp::from_v8<std::string>(isolate, args[0], "");

		int argc = args.Length();
		if (args[argc - 1]->IsFunction())
		{
			--argc;
			cb.reset(new v8_core::callback(args, argc));
		}

		int const bind_count = (argc > 1? argc - 1 : 0);
		if (bind_count > 0)
		{
			cb_args.resize(bind_count);
			for (int i = 0; i < bind_count; ++i)
			{
				cb_args[i] = args[i+1];
			}
		}
	}

	virtual ~async_operation()
	{
	}

	virtual void done()
	{
	}

	void completed(std::string err_msg)
	{
		if (cb)
		{
			v8::HandleScope scope(cb->isolate);

			_aspect_assert(sess);
			if (err_msg.empty())
			{
				done();
			}
			else
			{
				cb_args.clear();
				cb_args.push_back(v8pp::throw_ex(cb->isolate, err_msg));
			}

			cb->call(static_cast<int>(cb_args.size()), cb_args.data());
		}
		delete this;
	}
};

struct async_prepare : async_operation
{
	std::unique_ptr<statement> st;

	async_prepare(session* sess, v8::FunctionCallbackInfo<v8::Value> const& args)
		: async_operation(sess, args)
		, st(new statement(sess->impl_, query))
	{
		if (!cb_args.empty())
		{
			st->bind_col_ += st->bind_values(st->bind_col_, args.GetIsolate(), cb_args);
			cb_args.clear();
		}
	}

	void run()
	{
		cb_args.resize(2);
		cb_args[0] = v8::Undefined(cb->isolate);
		cb_args[1] = v8pp::class_<statement>::import_external(cb->isolate, st.release());
		completed("");
	}
};

struct async_exec : async_prepare
{
	std::unique_ptr<result> res;

	async_exec(session* sess, v8::FunctionCallbackInfo<v8::Value> const& args)
		: async_prepare(sess, args)
	{
	}

	void select()
	{
		res.reset(st->query());
	}

	void exec()
	{
		st->exec();
		res.reset();
	}

	void done()
	{
		_aspect_assert(cb);

		cb_args.resize(3);
		cb_args[0] = v8::Undefined(cb->isolate);
		if (res)
		{
			cb_args[1] = v8::Undefined(cb->isolate);
			cb_args[2] = res->as_array_of_smth(cb->isolate, &result::as_array_impl, 0);
		}
		else
		{
			v8::Local<v8::Object> info = v8::Object::New(cb->isolate);
			set_option(cb->isolate, info, "lastInsertId", st->last_insert_id());
			set_option(cb->isolate, info, "affected", st->affected());
			cb_args[1] = info;
			cb_args[2] = v8::Undefined(cb->isolate);
		}
	}
};

void session::open(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::unique_ptr<async_operation> op(new async_operation(this, args));
	std::string const& config = op->query;

	if (config.empty())
	{
		throw std::invalid_argument("expected session config string");
	}

	if (!op->cb)
	{
		impl_.open(config);
	}
	else
	{
		runtime& rt = runtime::instance(isolate);
		void (cppdb::session::*impl_open)(std::string const&) = &cppdb::session::open;

		rt.event_queue().schedule(rt.main_loop(),
			boost::bind(impl_open, &impl_, config),
			boost::bind(&async_operation::completed, op.release(), _1));
	}
}

void session::close(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::unique_ptr<async_operation> op(new async_operation(this, args));

	if (!op->cb)
	{
		impl_.close();
	}
	else
	{
		runtime& rt = runtime::instance(isolate);

		rt.event_queue().schedule(rt.main_loop(),
			boost::bind(&cppdb::session::close, &impl_),
			boost::bind(&async_operation::completed, op.release(), _1));
	}
}

void session::prepare(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::auto_ptr<async_prepare> op(new async_prepare(this, args));
	if (op->query.empty())
	{
		throw std::invalid_argument("expected query string");
	}

	if (!op->cb)
	{
		v8::Handle<v8::Object> st = v8pp::class_<statement>::import_external(isolate, op->st.release());
		args.GetReturnValue().Set(st);
	}
	else
	{
		runtime::instance(isolate).main_loop().schedule(boost::bind(&async_prepare::run, op.release()));
	}
}

void session::exec(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::auto_ptr<async_exec> op(new async_exec(this, args));

	if (op->query.empty())
	{
		throw std::invalid_argument("expected query string");
	}

	if (!op->cb)
	{
		op->exec();
	}
	else
	{
		runtime& rt = runtime::instance(isolate);

		bool const is_select = (utils::to_lower(op->query.substr(0, 6)) == "select");
		async_exec* ctx = op.release();
		
		rt.event_queue().schedule(rt.main_loop(),
			boost::bind(is_select? &async_exec::select : &async_exec::exec, ctx),
			boost::bind(&async_exec::completed, ctx, _1));
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// statement
//
statement::statement(cppdb::session& sess, std::string const& query)
	: impl_(sess.prepare(query))
	, bind_col_(1)
{
}

void statement::bind_value(int col, v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
	if ( value->IsNull() )         impl_.bind_null(col);
	else if ( value->IsBoolean() ) impl_.bind(col, value->BooleanValue());
	else if ( value->IsInt32() )   impl_.bind(col, value->Int32Value());
	else if ( value->IsUint32() )  impl_.bind(col, value->Uint32Value());
	else if ( value->IsNumber() )  impl_.bind(col, value->NumberValue());
	else if ( value->IsString() )  impl_.bind(col, v8pp::from_v8<std::string>(isolate, value));
	else throw std::runtime_error("unsupported value to bind: " + v8pp::from_v8<std::string>(isolate, value->ToString()));
}

int statement::bind_values(int col, v8::Isolate* isolate, v8_values const& values)
{
	int const count = static_cast<int>(values.size());
	for (int i = 0; i < count; ++i)
	{
		bind_value(col + i, isolate, values[i]);
	}
	return count;
}

int statement::bind_values(int col, int offset, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	int const count = max(0, args.Length() - offset);
	for (int i = 0; i < count; ++i)
	{
		bind_value(col + i, isolate, args[i + offset]);
	}
	return count;
}

void statement::bind_nth(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	int const start = v8pp::from_v8<int>(isolate, args[0], 0);
	if (start < 1)
	{
		throw std::invalid_argument("required a start index >= 1");
	}

	if (args[1]->IsArray())
	{
		bind_values(start, isolate, v8pp::from_v8<v8_values>(isolate, args[1]));
	}
	else
	{
		bind_values(start, 1, args);
	}

	args.GetReturnValue().Set(v8pp::to_v8(isolate, *this));
}

void statement::bind(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	if (args[0]->IsArray())
	{
		bind_col_ += bind_values(bind_col_, 0, v8pp::from_v8<v8_values>(isolate, args[0]));
	}
	else
	{
		bind_col_ += bind_values(bind_col_, 0, args);
	}

	args.GetReturnValue().Set(v8pp::to_v8(isolate, *this));
}

result* statement::query()
{
	result* r = new result(impl_.query());

	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	if (isolate)
	{
		v8pp::class_<result>::import_external(isolate, r);
	}
	return r;
}

result* statement::row()
{
	result* r = query();
	r->next();
	return r;
}

void statement::exec()
{
	impl_.exec();
	reset();
}

void statement::reset()
{
	impl_.reset();
	bind_col_ = 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// result
//
result::result(cppdb::result const& r)
	: impl_(r)
{
}

v8::Handle<v8::Value> result::value_n(v8::Isolate* isolate, int col)
{
	switch ( impl_.type(col) )
	{
	case cppdb::null_type:
		return v8::Null(isolate);
	case cppdb::int_type:
		return v8pp::to_v8(isolate, impl_.get<int64_t>(col));
	case cppdb::real_type:
		return v8pp::to_v8(isolate, impl_.get<double>(col));
	case cppdb::string_type:
		return v8pp::to_v8(isolate, impl_.get<std::string>(col));
	case cppdb::blob_type:
		{
			std::stringstream ss;
			impl_.fetch(col, ss);

			std::string const str = ss.str();
			return v8pp::class_<v8_core::buffer>::import_external(isolate, new v8_core::buffer(str.data(), str.size()));
		}
	}

	return v8::Undefined(isolate);
}

v8::Handle<v8::Value> result::as_array_impl(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	int const size = cols();

	v8::Local<v8::Array> obj = v8::Array::New(isolate, size);
	for (int i = 0; i < size; ++i)
	{
		obj->Set(i, value_n(isolate, i));
	}

	return scope.Escape(obj);
}

v8::Handle<v8::Value> result::as_object_impl(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	int const size = cols();

	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	for (int i = 0; i != size; ++i)
	{
		obj->Set(v8pp::to_v8(isolate, name(i)), value_n(isolate, i));
	}
	return scope.Escape(obj);
}

v8::Handle<v8::Value> result::as_array_of_smth(v8::Isolate* isolate, row_handler handler, uint32_t limit)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Array> arr = v8::Array::New(isolate, limit);
	if (!limit)
	{
		limit = std::numeric_limits<uint32_t>::max();
	}

	for (uint32_t i = 0; next() && i < limit; ++i)
	{
		arr->Set(i, (this->*handler)(isolate));
	}

	return scope.Escape(arr);
}

struct result::callback : public v8_core::callback
{
	result::row_handler handler;
	uint32_t limit;

	callback(v8::FunctionCallbackInfo<v8::Value> const& args, result::row_handler handler, uint32_t limit)
		: v8_core::callback(args, 1)
		, handler(handler)
		, limit(limit)
	{
	}
};

void result::on_array_of_smth(callback* cb)
{
	v8::HandleScope scope(cb->isolate);

	int argc = 0;
	v8::Handle<v8::Value> argv[2];

	try
	{
		v8::Local<v8::Object> self = v8pp::to_local(cb->isolate, cb->recv);
		result* res = v8pp::from_v8<result*>(cb->isolate, self);

		argc = 2;
		argv[0] = v8::Undefined(cb->isolate);
		argv[1] = res->as_array_of_smth(cb->isolate, cb->handler, cb->limit);
	}
	catch (std::exception const& ex)
	{
		argc = 1;
		argv[0] = v8pp::throw_ex(cb->isolate, ex.what());
	}

	cb->call(argc, argv);
	delete cb;
}

void result::value(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (impl_.empty())
	{
		return;
	}

	v8::Isolate* isolate = args.GetIsolate();

	int col = -1;
	if (args[0]->IsNumber())
	{
		col = args[0]->Int32Value();
	}
	else if (args[0]->IsString())
	{
		std::string const name = v8pp::from_v8<std::string>(isolate, args[0]);
		col = impl_.find_column(name);
	}
	else
	{
		throw std::runtime_error("expected column index or name");
	}

	args.GetReturnValue().Set(value_n(isolate, col));
}

void result::as_object(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (!impl_.empty())
	{
		args.GetReturnValue().Set(as_object_impl(args.GetIsolate()));
	}
}

void result::as_array(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (!impl_.empty())
	{
		args.GetReturnValue().Set(as_array_impl(args.GetIsolate()));
	}
}

void result::as_array_of_arrays(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	uint32_t const limit = v8pp::from_v8<uint32_t>(isolate, args[0], 0);

	if (args[1]->IsFunction())
	{
		runtime::instance(isolate).main_loop().schedule(boost::bind(&on_array_of_smth,
			new callback(args, &result::as_array_impl, limit)));
	}
	else
	{
		v8::Local<v8::Value> result = as_array_of_smth(isolate, &result::as_array_impl, limit);
		args.GetReturnValue().Set(scope.Escape(result));
	}
}

void result::as_array_of_objects(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	uint32_t const limit = v8pp::from_v8<uint32_t>(isolate, args[0], 0);

	if (args[1]->IsFunction())
	{
		runtime::instance(isolate).main_loop().schedule(boost::bind(&on_array_of_smth,
			new callback(args, &result::as_object_impl, limit)));
	}
	else
	{
		v8::Local<v8::Value> result = as_array_of_smth(isolate, &result::as_object_impl, limit);
		args.GetReturnValue().Set(scope.Escape(result));
	}
}

}} // namespace aspect::db
