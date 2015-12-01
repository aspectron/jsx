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
#include "jsx/websocket.hpp"

#include "jsx/url.hpp"
#include "jsx/runtime.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_main_loop.hpp"

#include "jsx/websocket_hixie_76.hpp"
#include "jsx/websocket_hybi_17.hpp"

namespace aspect
{

using boost::system::error_code;
using namespace boost::asio::ip;

bool websocket::is_websocket_upgrade(pion::http::request const& http_request)
{
	pion::iequal_to iequals;

	return http_request.get_method() == pion::http::types::REQUEST_METHOD_GET
		&& http_request.get_version_major() >= 1
		&& http_request.get_version_minor() >= 1
		&& iequals(http_request.get_header("Upgrade"), "WebSocket")
		&& utils::to_lower(http_request.get_header("Connection")).find("upgrade") != string::npos;
}

bool websocket::is_websocket_upgrade(pion::http::response const& http_response)
{
	pion::iequal_to iequals;

	return http_response.get_status_code() == 101
		&& http_response.get_version_major() >= 1
		&& http_response.get_version_minor() >= 1
		&& iequals(http_response.get_header("Upgrade"), "WebSocket")
		&& iequals(http_response.get_header("Connection"), "Upgrade");
}

void websocket::server_start(v8::Isolate* isolate, pion::http::request_ptr http_request, pion::tcp::connection_ptr tcp_conn, v8_core::event_emitter* emitter)
{
	_aspect_assert(is_websocket_upgrade(*http_request));

	std::unique_ptr<websocket> ws;

	if ( http_request->has_header("Sec-WebSocket-Key")
		&& http_request->has_header("Sec-WebSocket-Version") )
	{
		ws.reset(new websocket_hybi_17(runtime::instance(isolate), tcp_conn, emitter));
	}
	else if ( http_request->has_header("Sec-WebSocket-Key1")
		&& http_request->has_header("Sec-WebSocket-Key2") )
	{
		ws.reset(new websocket_hixie_76(runtime::instance(isolate), tcp_conn, emitter));
	}
	else
	{
		throw std::runtime_error("Requested unsupported websocket protocol");;
	}

	ws->start(http_request);
	v8pp::class_<websocket>::import_external(isolate, ws.release());
}

websocket* websocket::client_start(v8::Isolate* isolate, string const& url, bool use_old_hixie)
{
	std::auto_ptr<websocket> ws;

	pion::tcp::connection_ptr conn;
	v8_core::event_emitter* emitter = nullptr;
	runtime& rt = runtime::instance(isolate);

	if ( use_old_hixie )
	{
		ws.reset(new websocket_hixie_76(rt, conn, emitter));
	}
	else
	{
		ws.reset(new websocket_hybi_17(rt, conn, emitter));
	}

	ws->connect(aspect::url(url));
	return ws.release();
}

websocket::websocket(runtime& rt, pion::tcp::connection_ptr tcp_conn, v8_core::event_emitter* emitter)
	: rt_(rt)
	, tcp_conn_(tcp_conn)
	, state_(CLOSED)
{
	if ( tcp_conn_ )
	{
		tcp_conn_->get_socket().set_option(boost::asio::ip::tcp::no_delay(true)); // disable ngale
	}

	// Use either external event handler for server-side websocket
	// or this instance as an event hander for client-side websocket.
	emitter_ = (emitter? emitter : this);
}

websocket::~websocket()
{
	if ( tcp_conn_ )
	{
		tcp_conn_->set_lifecycle(pion::tcp::connection::LIFECYCLE_CLOSE);
		tcp_conn_->finish();
	}
}

void websocket::abort(string const& reason)
{
	state_ = CLOSED;
	tcp_conn_.reset();
	on_error(reason);
}

bool websocket::is_closed(boost::system::error_code const& err, size_t bytes_transferred) const
{
	return state_ != OPEN
		|| (err == boost::asio::error::connection_reset && bytes_transferred == 0);

}
bool  websocket::check_err(error_code const& err, string const& reason)
{
	if ( err )
	{
		return check_err(false, reason + ": " + err.message());
	}
	return true;
}

bool websocket::check_err(bool cond, string const& msg)
{
	if ( !cond )
	{
		on_error(msg);
		//TODO: log error
		close();
	}
	return cond;
}

error_code websocket::send_message(pion::http::message const& http_message)
{
	// HTTPMessage::send() isn't suitable because it changes some headers
	// (such as "Connection", "Content-Length")
	// reimplement most of the HTTPRequestponse::send() function

	pion::http::message::write_buffers_t write_buffers;

	// add first message line
	write_buffers.push_back(boost::asio::buffer(http_message.get_first_line()));
	write_buffers.push_back(boost::asio::buffer(pion::http::types::STRING_CRLF));

	// add HTTP headers
	pion::ihash_multimap const& headers = const_cast<pion::http::message&>(http_message).get_headers();
	for (pion::ihash_multimap::const_iterator it = headers.begin(), end = headers.end(); it != end; ++it)
	{
		write_buffers.push_back(boost::asio::buffer(it->first));
		write_buffers.push_back(boost::asio::buffer(pion::http::types::HEADER_NAME_VALUE_DELIMITER));
		write_buffers.push_back(boost::asio::buffer(it->second));
		write_buffers.push_back(boost::asio::buffer(pion::http::types::STRING_CRLF));
	}
	// add an extra CRLF to end HTTP headers
	write_buffers.push_back(boost::asio::buffer(pion::http::types::STRING_CRLF));

	// append content
	size_t const content_len = http_message.get_content_length();
	char const* const content = http_message.get_content();
	if ( content_len > 0 && content )
	{
		write_buffers.push_back(boost::asio::buffer(content, content_len));
	}

	error_code err;
	tcp_conn_->write(write_buffers, err);
	return err;
}

void websocket::start(pion::http::request_ptr http_request)
{
	state_ = CONNECTING;

	// check handshake and start listening
	pion::http::response_ptr http_response(new pion::http::response(*http_request));

	if ( !handshake(http_request, http_response) )
	{
		http_response->set_status_code(400);
		http_response->set_status_message("Bad request");
		send_message(*http_response);
		abort("Handshake error");
	}
	else if ( check_err(send_message(*http_response), "handshake") )
	{
		state_ = OPEN;
		on_connect(http_request->get_resource());
		wait_data();
	}
}

void websocket::connect(url const& target)
{
	state_ = CONNECTING;

	if ( target.scheme() != "ws" && target.scheme() != "wss" )
	{
		throw std::invalid_argument("Unsupported URL " + target.to_string());
	}

	tcp_conn_.reset(new pion::tcp::connection(rt_.io_service(), target.is_scheme_secured()));
//	tcp_conn_->getSocket().set_option(boost::asio::ip::tcp::no_delay(true)); // disable ngale

	// create resolver and start async resolve
	boost::shared_ptr<tcp::resolver> resolver = boost::make_shared<tcp::resolver>(rt_.io_service());
	tcp::resolver::query query(target.host(), std::to_string(static_cast<unsigned long long>(target.effective_port())),
		tcp::resolver::query::numeric_service);

	resolver->async_resolve(query, boost::bind(&websocket::handle_resolve, this,
		target, resolver, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
}

void websocket::handle_resolve(url target, boost::shared_ptr<tcp::resolver> reslv,
		error_code const& err, tcp::resolver::iterator endpoint_iterator)
{
	if ( check_err(err, "resolve") )
	{
		tcp_conn_->async_connect(endpoint_iterator->endpoint(), boost::bind(&websocket::handle_connect, this,
			target, boost::asio::placeholders::error));
	}
}

void websocket::handle_connect(url target, error_code const& err)
{
	if ( check_err(err, "connect") )
	{
		if ( tcp_conn_->get_ssl_flag() )
		{
			if ( !check_err(tcp_conn_->handshake_client(), "SSL Handshake") )
			{
				return;
			}
		}
		if ( !handshake(target) )
		{
			abort("Handshake");
		}
	}
}

void websocket::close()
{
	if ( state_ == OPEN )
	{
		state_ = CLOSING;
		do_close();
		state_ = CLOSED;
		tcp_conn_->close();
		on_close();
	}
}

void websocket::send(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if ( state_ == OPEN )
	{
		_aspect_assert(tcp_conn_->is_open());

		v8::Local<v8::Value> data = args[0];
		if ( data->IsString() )
		{
			v8::String::Utf8Value str(data);
			do_write(*str, str.length(), false, false);
		}
		else if ( v8_core::buffer const* buf = v8pp::from_v8<v8_core::buffer*>(args.GetIsolate(), data) )
		{
			do_write(buf->data(), buf->size(), true, false);
		}
		else
		{
			throw std::invalid_argument("websocket_s::write() - unsupported argument (neither string nor buffer)");
		}
	}
}

void websocket::write(void const* data, size_t size, bool is_binary, bool async)
{
	if ( state_ == OPEN )
	{
		if ( size > MAX_DATA_SIZE )
		{
			throw std::runtime_error("Send: data size exceeds 1Gb");
		}
		do_write(data, size, is_binary, async);
	}
}

void websocket::on_connect(string const& resource)
{
	if ( sink_ && sink_->on_connect(*this, resource) )
	{
		return;
	}
	rt_.main_loop().schedule(boost::bind(&websocket::handle_connect_v8, this, resource));
}

void websocket::on_close()
{
	if ( sink_ && sink_->on_close(*this) )
	{
		return;
	}
	rt_.main_loop().schedule(boost::bind(&websocket::handle_close_v8, this));
}

void websocket::on_error(string const& error)
{
	if ( sink_ && sink_->on_error(*this, error) )
	{
		return;
	}
	rt_.main_loop().schedule(boost::bind(&websocket::handle_error_v8, this, error));
}

void websocket::on_data(buffer& data, bool is_binary)
{
	if ( sink_ && sink_->on_data(*this, data, is_binary) )
	{
		return;
	}
	shared_buffer shared_buf = boost::make_shared<buffer>();
	shared_buf->swap(data);
	rt_.main_loop().schedule(boost::bind(&websocket::handle_data_v8, this, shared_buf, is_binary));
}

void websocket::handle_connect_v8(string resource)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> self = v8pp::to_v8(isolate, this);
	if (!self.IsEmpty())
	{
		v8::Handle<v8::Value> args[1] = { v8pp::to_v8(isolate, resource) };
		emitter_->emit(isolate, "connect", self->ToObject(), 1, args);
	}
}

void websocket::handle_close_v8()
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> self = v8pp::to_v8(isolate, this);
	if (!self.IsEmpty())
	{
		emitter_->emit(isolate, "close", self->ToObject(), 0, nullptr);
	}
}

void websocket::handle_error_v8(string error)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> self = v8pp::to_v8(isolate, this);
	if (!self.IsEmpty())
	{
		v8::Handle<v8::Value> args[1] = { v8pp::to_v8(isolate, error) };
		emitter_->emit(isolate, "error", self->ToObject(), 1, args);
	}
}

void websocket::handle_data_v8(shared_buffer data, bool is_binary)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> self = v8pp::to_v8(isolate, this);
	if (!self.IsEmpty())
	{
		v8::Handle<v8::Value> args[2];
		args[1] = v8pp::to_v8(isolate, is_binary);
		if ( is_binary )
		{
			v8_core::buffer* buf = new v8_core::buffer;
			buf->swap(*data);
			args[0] = v8pp::class_<v8_core::buffer>::import_external(isolate, buf);
		}
		else
		{
			char const* const chars = reinterpret_cast<char const*>(data->empty()? nullptr : &*(data->begin()));
			int const length = static_cast<int>(data->size());
			args[0] = v8pp::to_v8(isolate, chars, length);
		}
		emitter_->emit(isolate, "data", self->ToObject(), 2, args);
	}
}

} // aspect
