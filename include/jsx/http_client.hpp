//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_HTTP_CLIENT_HPP_INCLUDED
#define JSX_HTTP_CLIENT_HPP_INCLUDED

#include "jsx/http.hpp"

namespace aspect { namespace http {

/// HTTP client
class  client : boost::noncopyable
{
public:
	/// Number of tries for HTTP requests by default
	static unsigned const NUM_TRIES = 3;

	/// Create an HTTP client
	explicit client(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Close connection on destroy
	~client();

	/// Close connection and clear all settings
	void close();

	/// Resets connectivity, keeps all settings
	void reset(v8::Isolate* isolate);

	/// Set connection keep alive flag
	void set_keep_alive(bool flag);

	/// Connect to the server, url is a string http[s]://[user:pass@]host[:port]
	void connect(std::string const& url, unsigned num_tries = NUM_TRIES);

	/// Response content length in bytes
	size_t content_length() const;

	/// Response content or NULL if conten_length() == 0
	char const* content() const;

	/// Response via v8 buffer
	v8::Handle<v8::Value> content_as_buffer();

	/// User ajax execute function
	void ajax(v8::FunctionCallbackInfo<v8::Value> const& args);

private:
	/// St keep alive flag for connection
	void set_keep_alive_impl(bool flag);

	/// Reconnect to url_
	bool reconnect(unsigned num_tries);

	// Data for ajax transaction
	struct ajax_transaction
	{
		ajax_transaction(v8::Isolate* isolate, v8::Handle<v8::Object> object)
			: object(isolate, object)
			, retries(NUM_TRIES)
			, url("/")
		{
		}

		v8::UniquePersistent<v8::Object> object;
		unsigned retries;
		std::string url;
		pion::http::request req;
		pion::http::response resp;
		std::string error;
	};

	/// Uses req and resp of tx to send and receive http transaction
	void perform(v8::Isolate* isolate, ajax_transaction* txn);

	/// Process ajax transaction completion
	void completed(v8::Isolate* isolate, ajax_transaction* txn, std::string err_msg);

	/// Deflate gzipped response content
	void deflate_gzip(pion::http::response& resp);

	std::string url_;
	bool keep_alive_;

	double flood_pace_;
	double ts_last_;

	boost::shared_ptr<pion::tcp::connection> conn_;
	boost::recursive_mutex conn_mutex_;
	uint32_t requests_;
};

void make_query_string(v8::FunctionCallbackInfo<v8::Value> const& args);

}} // namespace aspect::http

#endif // JSX_HTTP_CLIENT_HPP_INCLUDED
