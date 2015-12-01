//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_DB_HPP_INCLUDED
#define JSX_DB_HPP_INCLUDED

#include <boost/noncopyable.hpp>

#include "jsx/types.hpp"
#include "jsx/v8_core.hpp"
#include "jsx/v8_callback.hpp"

#include "cppdb/frontend.h"

namespace aspect { namespace db {

void setup_bindings(v8pp::module& target);

class session;
class statement;
class result;

typedef std::vector<v8::Handle<v8::Value>> v8_values;

/// Database session
class session : boost::noncopyable
{
public:
	/// Create and open a database session
	explicit session(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (args.Length() > 0)
		{
			open(args);
		}
	}

	/// Close the session on destroy
	~session()
	{
		try
		{
			impl_.close();
		}
		catch (...)
		{
		}
	}

	/// Open database session
	void open(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Close database session
	void close(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Is session open?
	bool is_open() { return impl_.is_open(); }

	/// Prepare a statement with query
	void prepare(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Execute a statement
	void exec(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Begin a transaction
	void begin() { impl_.begin(); }

	/// Commit a transaction
	void commit() { impl_.commit(); }

	/// Rollback a transaction
	void rollback() { impl_.rollback(); }

	/// Escape string (sanitize) (may throw)
	std::string escape(std::string const& s) { return impl_.escape(s); }

private:
	friend struct async_operation; // to access impl_
	friend struct async_prepare; // to access impl_
	cppdb::session impl_;
};

/// SQLite statement
class statement : boost::noncopyable
{
public:
	statement(cppdb::session& sess, std::string const& query);

	/// Fetch a result of the statement query
	result* query();

	/// Fetch a single row of the statement query
	result* row();

	/// Execute the statement
	void exec();

	/// Reset the statement (remove all bindings and returs it into initial state)
	void reset();

	/// Number of affected rows
	uint64_t affected() { return impl_.affected(); }

	uint64_t last_insert_id() { return impl_.last_insert_id(); }

	/// Bind args to the placeholder number (starting from 1) marked with '?' marker in the query.
	void bind_nth(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Bind args to the next placeholder (starting from the first) marked with '?' marker in the query.
	void bind(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	int bind_values(int col, v8::Isolate* isolate, v8_values const& values);
	int bind_values(int col, int offset, v8::FunctionCallbackInfo<v8::Value> const& args);

	void bind_value(int col, v8::Isolate* isolate, v8::Handle<v8::Value> value);

	friend struct async_operation;
	friend struct async_prepare;
	cppdb::statement impl_;
	int bind_col_;
};

/// Statement query result
class result : boost::noncopyable
{
public:
	explicit result(cppdb::result const& r);

	/// Number of columns in the result
	int cols() { return impl_.cols(); }

	/// Convert column name to its index, throws invalid_column if the name is not valid.
	int index(std::string const& name) { return impl_.index(name); }

	/// Convert column name to its index, returns -1 if the name is not valid.
	int find_column(std::string const& name) { return impl_.find_column(name); }

	/// Convert column index to column name, throws invalid_column if col is not in range 0<= col < cols()
	std::string name(int col) { return impl_.name(col); }

	/// Move forward to next row in the result, return false if no more rows.
	bool next() { return impl_.next(); }

	/// Clears the result.
	void clear() { impl_.clear(); }

	/// Check if the current row is empty
	bool empty() { return impl_.empty(); }

	/// Get a column value
	void value(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Return current row value as an array.
	void as_array(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Return current row as an object (with column names)
	void as_object(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Return an array of objects
	void as_array_of_objects(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Return an array of arrays
	void as_array_of_arrays(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	v8::Handle<v8::Value> value_n(v8::Isolate* isolate, int col);
	v8::Handle<v8::Value> as_array_impl(v8::Isolate* isolate);
	v8::Handle<v8::Value> as_object_impl(v8::Isolate* isolate);

	typedef v8::Handle<v8::Value> (result::*row_handler)(v8::Isolate*);

	v8::Handle<v8::Value> as_array_of_smth(v8::Isolate* isolate, row_handler handler, uint32_t limit);

	struct callback;

	static void on_array_of_smth(callback* cb);

	cppdb::result impl_;

	friend struct callback;
	friend struct async_exec;
};

}} // namespace aspect::db

#endif // JSX_DB_HPP_INCLUDED
