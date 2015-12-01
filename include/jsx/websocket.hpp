//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_WEBSOCKET_HPP_INCLUDED
#define JSX_WEBSOCKET_HPP_INCLUDED

#include "jsx/http.hpp"
#include "jsx/events.hpp"
#include "jsx/types.hpp"

namespace aspect
{

class url;

class CORE_API websocket : public v8_core::event_emitter
{
public:
	static void server_start(v8::Isolate* isolate, pion::http::request_ptr http_request, pion::tcp::connection_ptr tcp_conn,
		v8_core::event_emitter* external_emitter);

	static websocket* client_start(v8::Isolate* isolate, std::string const& url, bool use_old_hixie);

	/// Check is this a web socket updgrade request
	static bool is_websocket_upgrade(pion::http::request const& http_request);
	static bool is_websocket_upgrade(pion::http::response const& http_response);

	virtual ~websocket();

	/// Websocket state
	enum state_t { CONNECTING = 0, OPEN = 1, CLOSING = 2, CLOSED = 3 };
	state_t state() const { return state_; }

	/// Close the websocket
	void close();

	/// Write UTF-8 encoded string or v8_buffer
	void send(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Write data[size] into the websocket
	void write(void const* data, size_t size, bool is_binary, bool async);

	/// Maximal data size
	static size_t const MAX_DATA_SIZE = 1024 * 1024 * 1024;

public:

	/// Websocket service sink
	class sink
	{
	public:
		virtual ~sink() {}

		/// Notify about websocket connect
		virtual bool on_connect(websocket& ws, string const& resource) = 0;

		/// Notify about websocket close
		virtual bool on_close(websocket& ws) = 0;

		/// Notify about websocket error
		virtual bool on_error(websocket& ws, string const& error) = 0;

		/// Notify about websocket data
		virtual bool on_data(websocket& ws, buffer& data, bool is_binary) = 0;
	};

	/// Install websocket sink, transfer ownership of ptr to the websocket
	void set_sink(sink* ptr) { sink_.reset(ptr); }

	void on_connect(string const& resource);
	void on_close();
	void on_error(string const& error);
	void on_data(buffer& data, bool is_binary);

protected:
	websocket(runtime& rt, pion::tcp::connection_ptr tcp_conn, v8_core::event_emitter* emitter);

	bool is_closed(boost::system::error_code const& err, size_t bytes_transferred) const;

	bool check_err(boost::system::error_code const& err, string const& reason);
	bool check_err(bool cond, string const& msg);

	/// Abort connection
	void abort(string const& reason);

	/// Is client-side websocket
	bool is_client_side() const { return emitter_ == this; }

	/// Send websocket handshake message
	boost::system::error_code send_message(pion::http::message const& http_message);

	runtime& rt_;
	pion::tcp::connection_ptr tcp_conn_;
	state_t state_;

private:
	boost::scoped_ptr<sink> sink_;
	v8_core::event_emitter* emitter_;

	/// Start server-side websocket listening
	void start(pion::http::request_ptr http_request);

	/// Connect client-side websocket to URL
	void connect(url const& target);

	void handle_resolve(url target, boost::shared_ptr<boost::asio::ip::tcp::resolver> reslv,
		boost::system::error_code const& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void handle_connect(url target, const boost::system::error_code& err);
private:
	void handle_connect_v8(string resource);
	void handle_close_v8();
	void handle_error_v8(string error);
	void handle_data_v8(shared_buffer data, bool is_binary);

private:
	/// Check for server-side websocket handshake
	virtual bool handshake(pion::http::request_ptr http_request,
		pion::http::response_ptr http_response) = 0;

	/// Check for client-side websocket handshake
	virtual bool handshake(url const& target) = 0;

	/// Perform close
	virtual void do_close() = 0;

	/// Perform data write
	virtual void do_write(void const* data, size_t size, bool is_binary, bool is_async) = 0;

	/// Wait data from client
	virtual void wait_data() = 0;
};

} // aspect

namespace v8pp {

template<>
inline aspect::websocket* v8_args_factory::instance<aspect::websocket>::create(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (!(args.Length() > 0 && args[0]->IsString()))
	{
		throw std::runtime_error("websocket ctor requires URL string");
	}

	std::string const url = v8pp::from_v8<string>(isolate, args[0]);
	bool const use_old_hixie = v8pp::from_v8<bool>(isolate, args[1], false);

	return aspect::websocket::client_start(isolate, url, use_old_hixie);
}

template<>
inline void v8_args_factory::instance<aspect::websocket>::destroy(v8::Isolate*, aspect::websocket* ws)
{
	ws->close();
	delete ws;
}

} // v8pp

#endif // JSX_WEBSOCKET_HPP_INCLUDED
