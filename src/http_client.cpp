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
#include "jsx/http_client.hpp"

#include "jsx/async_queue.hpp"
#include "jsx/runtime.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_main_loop.hpp"
#include "jsx/url.hpp"

#include <pion/tcp/connection.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/stream.hpp>

namespace aspect { namespace http {

client::client(v8::FunctionCallbackInfo<v8::Value> const& args)
	: keep_alive_(false)
	, flood_pace_(0.0)
	, ts_last_(0.0)
	, requests_(0)
{
	reset(args.GetIsolate());
}

client::~client()
{
	close();
}

void client::close()
{
	boost::recursive_mutex::scoped_lock lock(conn_mutex_);

	if ( conn_ )
	{
		conn_->close();
	}
}

void client::reset(v8::Isolate* isolate)
{
	boost::recursive_mutex::scoped_lock lock(conn_mutex_);

	if ( conn_ )
	{
		conn_->close();
	}
	conn_.reset(new pion::tcp::connection(runtime::instance(isolate).io_service()));

	set_keep_alive(true);
}

void client::set_keep_alive(bool flag)
{
	keep_alive_ = flag;
	set_keep_alive_impl(flag);
}

void client::set_keep_alive_impl(bool flag)
{

//	boost::mutex::scoped_lock lock(conn_mutex_);
	conn_->set_lifecycle(flag ? pion::tcp::connection::LIFECYCLE_KEEPALIVE : pion::tcp::connection::LIFECYCLE_CLOSE);
}

void client::connect(std::string const& url, unsigned num_tries)
{
	url_ = url;
	reconnect(num_tries);
}

bool client::reconnect(unsigned num_tries)
{
	url const http_url(url_);
	if (http_url.scheme() != "http" && http_url.scheme() != "https")
	{
		throw std::invalid_argument("Unsupported URL " + url_);
	}
	if (http_url.host().empty())
	{
		throw std::runtime_error("empty host");
	}

	requests_ = 0;

	num_tries = max(1U, num_tries);

	boost::system::error_code err;
	for (unsigned try_num = 0; try_num < num_tries; ++try_num)
	{

		boost::recursive_mutex::scoped_lock lock(conn_mutex_);

		conn_->close();
		err = conn_->connect(http_url.host(), http_url.effective_port());
		if (!err && http_url.is_scheme_secured())
		{
			err = conn_->handshake_client();
		}

		if (!err)
		{
			set_keep_alive_impl(keep_alive_);
			return true;
		}
	}

	return false;
}

void client::ajax(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> object = args[0]->ToObject();
	if (object.IsEmpty() || !object->IsObject())
	{
		throw std::runtime_error("require confgiuration object as a first argument");
	}

	std::unique_ptr<ajax_transaction> tx(new ajax_transaction(isolate, object));

	bool async = true;
	get_option(isolate, object, "async", async);

	get_option(isolate, object, "url", tx->url);
	tx->req.set_resource(tx->url);

	get_option(isolate, object, "retries", tx->retries);

	std::string method = pion::http::types::REQUEST_METHOD_GET;
	get_option(isolate, object, "type", method);
	tx->req.set_method(method);

	std::string resource;
	if (get_option(isolate, object, "resource", resource))
	{
		tx->req.set_resource(resource);
	}

	v8::Local<v8::Value> headers;
	if (get_option(isolate, object, "headers", headers))
	{
		tx->req.get_headers() = object_to_dict(isolate, headers->ToObject());
	}

	std::string data;
	if (get_option(isolate, object, "data", data))
	{
		if ( tx->req.get_method() == pion::http::types::REQUEST_METHOD_GET )
		{
			tx->req.set_query_string(data);
		}
		else if ( tx->req.get_method() == pion::http::types::REQUEST_METHOD_POST )
		{
			tx->req.set_content(data);
			tx->req.set_content_type(pion::http::types::CONTENT_TYPE_URLENCODED);
		}
	}

	ajax_transaction* txn = tx.release();
	runtime& rt = runtime::instance(isolate);
	if (async)
	{
		rt.event_queue().schedule(rt.main_loop(),
			boost::bind(&client::perform, this, isolate, txn),
			boost::bind(&client::completed, this, isolate, txn, _1));
	}
	else
	{
		perform(isolate, txn);
		rt.main_loop().schedule(boost::bind(&client::completed, this, isolate, txn, ""));
	}

	args.GetReturnValue().Set(args.This());
}

void client::perform(v8::Isolate* isolate, ajax_transaction* txn)
{
	_aspect_assert(txn);

	uint32_t num_tries = max(1U, txn->retries);

	url const http_url(url_);
	url const txn_url(txn->url);
	if ( http_url.empty() || (!txn_url.host().empty() && (http_url.origin() != txn_url.origin())) )
	{
		connect(txn->url, num_tries);
	}

	txn->req.add_header("Host", http_url.host()); // Mandatory in HTTP 1.1

	double ts_delta = utils::get_ts() - ts_last_;
	if ( flood_pace_ > 0.0 && ts_last_ > 0.0 && ts_delta < flood_pace_ )
	{
		ts_delta = std::min(ts_delta, 1000.0);
//		boost::this_thread::sleep(boost::posix_time::milliseconds(ts_delta));
	}

	boost::system::error_code err;
	for (unsigned try_num = 0; try_num < num_tries; ++try_num)
	{
		boost::recursive_mutex::scoped_lock lock(conn_mutex_);

		txn->req.send(*conn_, err);
		if ( err )
		{
#if HAVE(PION_LOGGER_DELEGATE)
			if (pion::logger::generic().level() <= pion::LOG_LEVEL_DEBUG )
			{
				runtime const& rt = runtime::instance(isolate);
				rt.trace("socket error on HTTP SEND: %s (requests: %d)\n", err.message().c_str(), requests_);
			}
#endif
			txn->error = "Request to " + http_url.host() + " failed: " + err.message();
			if ( reconnect(txn->retries) )
			{
				txn->error.clear();
				continue;
			}
		}

		txn->resp.update_request_info(txn->req);
		txn->resp.receive(*conn_, err);
		if ( err )
		{
#if HAVE(PION_LOGGER_DELEGATE)
			if ( pion::logger::generic().level() <= pion::LOG_LEVEL_DEBUG )
			{
				runtime const& rt = runtime::instance(isolate);
				rt.trace("socket error on HTTP RECV: %s (requests: %d)\n", err.message().c_str(), requests_);
			}
#endif
			txn->error = "Response from " + http_url.host() + " failed: " + err.message();
			if ( reconnect(txn->retries) )
			{
				txn->error.clear();
				continue;
			}
		}
		else
		{
			++requests_;
			break;
		}
	}

	if ( !err )
	{
		if ( txn->resp.get_header(pion::http::types::HEADER_CONTENT_ENCODING) == "gzip" )
		{
			deflate_gzip(txn->resp);
		}
	}
	ts_last_ = utils::get_ts();
}

void client::deflate_gzip(pion::http::response& resp)
{
	typedef boost::iostreams::array_source input_device;
	typedef boost::iostreams::back_insert_device<std::string> output_device;

	std::string content;

	boost::iostreams::stream<input_device> stream_in(resp.get_content(), resp.get_content_length());
	boost::iostreams::stream<output_device> stream_out(content);

	boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
	in.push(boost::iostreams::gzip_decompressor());
	in.push(stream_in);

	boost::iostreams::copy(in, stream_out);
	resp.set_content(content);
}

void client::completed(v8::Isolate* isolate, ajax_transaction* txn, std::string err_msg)
{
	std::unique_ptr<ajax_transaction> tx(txn);
	runtime& rt = runtime::instance(isolate);
	
	if ( !err_msg.empty() )
	{
		rt.trace("HTTP client failed: %s", err_msg.c_str());
		return;
	}

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> self = v8pp::to_v8(isolate, this)->ToObject();
	v8::Local<v8::Object> object = v8pp::to_local(isolate, tx->object);

	v8::Local<v8::Object> response_headers = dict_to_object(isolate, tx->resp.get_headers());

	v8::Local<v8::Value> text_status = v8pp::to_v8(isolate, tx->error.empty()? tx->resp.get_status_message() : tx->error);

	v8::Handle<v8::Value> args[4];
	char const* fn_name;
	if ( tx->resp.get_status_code() == 200 && tx->error.empty() )
	{
		v8_core::buffer* buf = new v8_core::buffer;
		if ( tx->resp.get_content_length() )
		{
			buf->replace(tx->resp.get_content(), tx->resp.get_content_length());
		}
		v8::Local<v8::Object> data = v8pp::class_<v8_core::buffer>::import_external(isolate, buf);

		args[0] = data;
		args[1] = text_status;
		args[2] = self;
		args[3] = response_headers;
		fn_name = "success";
	}
	else
	{
		v8::Local<v8::Value> response_code = v8pp::to_v8(isolate, tx->resp.get_status_code());

		v8::Handle<v8::Value> args[] = { text_status, self, response_headers, response_code };
		args[0] = text_status;
		args[1] = self;
		args[2] = response_headers;
		args[3] = response_code;
		fn_name = "failure";
	}

	v8::Local<v8::Function> fn = object->Get(v8pp::to_v8(isolate, fn_name)).As<v8::Function>();
	if(!fn.IsEmpty() && fn->IsFunction())
	{
		v8::TryCatch try_catch;
		fn->Call(object, sizeof(args) / sizeof(args[0]), args);
		if (try_catch.HasCaught())
		{
			rt.core().report_exception(try_catch);
		}
	}
}

void make_query_string(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if ( !args.Length() || !args[0]->IsObject() )
	{
		throw std::invalid_argument("make_query_string() requires an object as parameter");
	}

	pion::ihash_multimap const qp = object_to_dict(isolate, args[0]->ToObject());
	std::string const qstr = pion::http::types::make_query_string(qp);

	args.GetReturnValue().Set(v8pp::to_v8(isolate, qstr));
}

}} // namespace aspect::http
