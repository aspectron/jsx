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
#include "jsx/tcp.hpp"

#include "jsx/runtime.hpp"
#include "jsx/v8_buffer.hpp"
#include "jsx/v8_main_loop.hpp"

#include <boost/asio.hpp>

namespace aspect { namespace tcp {

using namespace v8;

void setup_bindings(v8pp::module& target)
{
	/**
	@module tcp - TCP
	TCP socket and server bindings.
	**/

	/**
	@class Socket - TCP socket class

	Socket class derives from events#EventEmitter

	@event connect                  fired from #Socket.asyncConnect
	@event close                    fired from #Socket.close
	@event send(bytesSent)          fired from #Socket.asyncSend
	@param bytesSent {Number}       number of bytes sent
	@event data(bufferReceived)     fired from #Socket.asyncReceive
	@param bufferReceived {Buffer}  data buffer received
	@event error(message)           fired on socket error
	@param message {String}
	**/
	v8pp::class_<socket> socket_class(target.isolate(), v8pp::v8_args_ctor);
	socket_class
		.inherit<v8_core::event_emitter>()
		/**
		@function connect(address [, port])
		@param address {String}
		@param [port] {Number}
		@return {Socket} `this` to chain calls
		Connect socket to `address`.
		TCP port can be specified as a part of address in
		form `host:port` or as an optional `port` argument.
		**/
		.set("connect", &socket::connect)

		/**
		@function close()
		@emit close
		Close socket
		**/
		.set("close", &socket::close)

		/**
		@function cancel()
		Cancel all asynchronous operations on the socket.
		**/
		.set("cancel", &socket::cancel)

		/**
		@property remoteEndpoint {String}
		Remote endpoint in `address:port` format, read-only.
		**/
		.set("remoteEndpoint", v8pp::property(&socket::remote_endpoint))

		/**
		@property localEndpoint {String}
		Local endpoint in `address:port` format, read-only.
		**/
		.set("localEndpoint", v8pp::property(&socket::local_endpoint))

		/**
		@function send(data)
		@param data {String|Buffer}
		@return {Number} number of bytes sent
		Send string or buffer `data` synchronously.
		**/
		.set("send", &socket::send)

		/**
		@function receive(buf [, size])
		@param buf {Buffer}
		@param [size] {Number}
		@return {Number} number of bytes received
		Receive data to the buffer `buf`.
		An optional `size` argument may be used to receive
		specified number of bytes.
		**/
		.set("receive", &socket::receive)

		/**
		@function asyncConnect(address [, port])
		@param address {String}
		@param [port] {Number}
		@return {Socket} `this` to chain calls
		@emit connect
		@emit error
		Asynchronously connect socket to `address`.
		TCP port can be specified as a part of address
		in form `host:port` or as an optional argument.
		**/
		.set("asyncConnect", &socket::async_connect)

		/**
		@function asyncSend(data [, callback])
		@param data {String|Buffer}
		@param [callback] {Function}
		@return {Socket} `this` to chain calls
		@emit send
		@emit error
		Send string or buffer `data` asynchronously.
		An optional `callback` Function may be specified as
		a `send` event handler.
		**/
		.set("asyncSend", &socket::async_send)

		/**
		@function asyncReceive([size])
		@param [size] {Number}
		@return {Socket} `this` to chain calls
		@emit data
		@emit error
		Receive data asynchronously.
		An optional `size` argument may be used to
		specified number of bytes to receive.
		**/
		.set("asyncReceive", &socket::async_receive)
		;

	/**
	@class Server TCP server class
	Server class derives from events#EventEmitter.

	@event accept(socket)  on accept new client connection in #Server.run
	@param socket {Socket} accepted socket instance
	@event error(message)  on socket error
	@param message {String}
	**/
	v8pp::class_<server> server_class(target.isolate(), v8pp::v8_args_ctor);
	server_class
		.inherit<v8_core::event_emitter>()
		/**
		@function run(port, handler)
		@param port {Number}
		@param handler {Function}
		@emit accept
		@emit error
		Start TCP server listening on specified `port`.
		Set `handler` function as `accept` event handler.
		**/
		.set("run", &server::run)

		/**
		@function stop()
		Stop the server.
		**/
		.set("stop", &server::stop)
		;

	v8pp::module tcp(target.isolate());
	tcp
		.set("Socket", socket_class)
		.set("Server", server_class)
		;

	target.set("tcp", tcp);
}

using namespace boost::asio;

static string endpoint_str(tcp::endpoint const& endpoint)
{
	return endpoint.address().to_string() + ":" + std::to_string(static_cast<unsigned long long>(endpoint.port()));
}

/////////////////////////////////////////////////////////////////////////////
//
// socket
//
socket::socket(v8::FunctionCallbackInfo<v8::Value> const& args)
	: impl_(runtime::instance(args.GetIsolate()).io_service())
	, rt_(runtime::instance(args.GetIsolate()))
{
}

socket::socket(runtime& rt)
	: impl_(rt.io_service())
	, rt_(rt)
{
}

socket::~socket()
{
	close();
}

void socket::close()
{
	if (impl_.is_open())
	{
		impl_.close();
		rt_.main_loop().schedule(boost::bind(&socket::handle_close_v8, this));
	}
}

void socket::cancel()
{
	impl_.cancel();
}

string socket::remote_endpoint()
{
	return endpoint_str(impl_.remote_endpoint());
}

string socket::local_endpoint()
{
	return endpoint_str(impl_.local_endpoint());
}

bool socket::check_error(error_code const& err)
{
	if ( err )
	{
		rt_.main_loop().schedule(boost::bind(&socket::handle_error_v8, this, err));
		return false;
	}
	return true;
}

static tcp::resolver::query connect_query(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	string host, port;

	switch ( args.Length() )
	{
	case 1:
		{
			std::string const address = v8pp::from_v8<std::string>(isolate, args[0]);
			std::vector<string> parts = utils::split(address, ':');
			if ( parts.size() != 2 )
			{
				throw std::invalid_argument("invalid address specified, must be in host:port format");
			}
			host = parts[0];
			port = parts[1];
		}
		break;
	case 2:
		{
			host = v8pp::from_v8<std::string>(isolate, args[0]);
			port = std::to_string((unsigned long long)v8pp::from_v8<uint32_t>(isolate, args[1]));
		}
		break;
	default:
		throw std::invalid_argument("argument must be a single string in host:port or two separate arguments host, port");
	}

	return tcp::resolver::query(tcp::v4(), host, port);
}

socket& socket::connect(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	error_code err;
	for (tcp::resolver::iterator it = rt_.tcp_resolver().resolve(connect_query(args), err), end; it != end; ++it)
	{
		if ( !err )
		{
			impl_.connect(*it);
			break;
		}
	}
	if ( err )
	{
		throw std::runtime_error(err.message());
	}

	return *this;
}

socket& socket::async_connect(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	rt_.tcp_resolver().async_resolve(connect_query(args), boost::bind(&socket::handle_resolve, this,
		placeholders::error, placeholders::iterator));

	return *this;
}

void socket::handle_resolve(error_code const& err, tcp::resolver::iterator iterator)
{
	if ( check_error(err) )
	{
		tcp::endpoint endpoint = *iterator;
		// cancel disabled. see boost::asio::basic_socket::cancel notes, use socket::close() instead
		// impl_->cancel();
		impl_.async_connect(endpoint, boost::bind(&socket::handle_connect, this,
			placeholders::error, ++iterator));
	}
}

void socket::handle_connect(error_code const& err, tcp::resolver::iterator iterator)
{
	if ( err )
	{
		if ( iterator == tcp::resolver::iterator() )
		{
			check_error(err);
		}
		else
		{
			// connection failed, try the next endpoint in the list
			impl_.close();
			tcp::endpoint endpoint = *iterator;
			impl_.async_connect(endpoint, boost::bind(&socket::handle_connect, this,
				placeholders::error, ++iterator));
		}
	}
	else
	{
		rt_.main_loop().schedule(boost::bind(&socket::handle_connect_v8, this));
	}
}

size_t socket::send(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if ( args.Length() != 1 )
	{
		throw std::invalid_argument("string or buffer argument required");
	}

	size_t sent = 0;
	if ( args[0]->IsString() )
	{
		String::Utf8Value const value(args[0]);
		sent = boost::asio::write(impl_, boost::asio::buffer(*value, value.length()));
	}
	else if ( v8_core::buffer* buf = v8pp::from_v8<v8_core::buffer*>(args.GetIsolate(), args[0]) )
	{
		sent = boost::asio::write(impl_, boost::asio::buffer(buf->data(), buf->size()));
	}
	else
	{
		throw std::invalid_argument("string or buffer argument required");
	}

	return sent;
}

void socket::send_data(void const* data, size_t size)
{
	error_code err;
	size_t const sent = boost::asio::write(impl_, boost::asio::buffer(data, size), err);
	if ( sent != size || !check_error(err) )
	{
		throw std::runtime_error("tcp::socket::send() - unable to send requested amount of data");
	}
}

socket& socket::async_send(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (args.Length() < 1)
	{
		throw std::invalid_argument("data argument required");
	}
	if (args[1]->IsFunction())
	{
		on(args.GetIsolate(), "send", args[1].As<v8::Function>());
	}

	String::Utf8Value* buf = new String::Utf8Value(args[0]);
	boost::asio::async_write(impl_, boost::asio::buffer(**buf, buf->length()),
		boost::bind(&socket::handle_write, this, buf,
			placeholders::error, placeholders::bytes_transferred));

	return *this;
}

void socket::handle_write(String::Utf8Value* buf, error_code const& err, size_t bytes_transferred)
{
	delete buf;
	if ( check_error(err) )
	{
		rt_.main_loop().schedule(boost::bind(&socket::handle_write_v8, this, bytes_transferred));
	}
}

size_t socket::receive(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if ( args.Length() != 1 && args.Length() != 2 )
	{
		throw std::invalid_argument("buffer parameter is required");
	}

	v8_core::buffer* res_buf = v8pp::from_v8<v8_core::buffer*>(args.GetIsolate(), args[0]);
	if ( !res_buf )
	{
		throw std::invalid_argument("buffer parameter is required");
	}

	size_t const buf_size = (args.Length() == 2? v8pp::from_v8<uint32_t>(args.GetIsolate(), args[1]) : impl_.available());
	res_buf->resize(buf_size);

	error_code err;
	if ( !res_buf->empty() )
	{
		size_t const received = boost::asio::read(impl_,
			boost::asio::buffer(res_buf->data(), res_buf->size()), transfer_all(), err);
		if ( check_error(err) )
		{
			res_buf->resize(received);
		}
	}

	return res_buf->size();
}

socket& socket::async_receive(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	size_t read_size = 4096;
	if ( args.Length() == 1 && args[0]->IsNumber() )
	{
		read_size = max<size_t>(read_size, v8pp::from_v8<uint32_t>(args.GetIsolate(), args[0]));
	}
	start_receive(read_size);
	return *this;
}

void socket::start_receive(size_t buf_size)
{
	shared_buffer buf = boost::make_shared< ::buffer >(buf_size);
	boost::asio::async_read(impl_, boost::asio::buffer(*buf), boost::asio::transfer_at_least(1),
		boost::bind(&socket::handle_read, this,
			buf, placeholders::error, placeholders::bytes_transferred));
}

void socket::handle_read(shared_buffer buf, error_code const& err, size_t bytes_transferred)
{
	size_t const buf_size = buf->size();
	buf->resize(bytes_transferred);
	if ( check_error(err) )
	{
		rt_.main_loop().schedule(boost::bind(&socket::handle_read_v8, this,
			buf, buf_size));
	}
}

void socket::handle_connect_v8()
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	emit(isolate, "connect", 0, nullptr);
}

void socket::handle_close_v8()
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	emit(isolate, "close", 0, nullptr);
}

void socket::handle_error_v8(error_code err)
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> args[1] = { v8pp::to_v8(isolate, err.message()) };
	if (!emit(isolate, "error", 1, args))
	{
		rt_.error("%tcp socket: s\n", err.message().c_str());
	}
}

void socket::handle_write_v8(size_t bytes_transferred)
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> args[1] = { v8pp::to_v8(isolate, bytes_transferred) };
	emit(isolate, "send", 1, args);
}

void socket::handle_read_v8(shared_buffer buf, size_t buf_size)
{
	if (!buf->empty())
	{
		v8_core::buffer* res_buf = new v8_core::buffer;
		res_buf->swap(*buf);

		v8::Isolate* isolate = rt_.isolate();

		v8::HandleScope scope(isolate);

		v8::Local<v8::Value> args[1] = { v8pp::class_<v8_core::buffer>::import_external(isolate, res_buf) };
		emit(isolate, "data", 1, args);
	}

	if (impl_.is_open())
	{
		start_receive(buf_size);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// server
//
server::server(v8::FunctionCallbackInfo<v8::Value> const& args)
	: acceptor_(runtime::instance(args.GetIsolate()).io_service())
	, rt_(runtime::instance(args.GetIsolate()))
{
}

server::~server()
{
	stop();
}

void server::stop()
{
	acceptor_.cancel();
	acceptor_.close();
}

void server::run(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<Uint32> port = args[0]->ToUint32();
	v8::Local<v8::Function> acceptor = args[1].As<v8::Function>();

	if (port.IsEmpty() || acceptor.IsEmpty())
	{
		throw std::invalid_argument("required port and handler function arguments");
	}

	on(isolate, "accept", acceptor);

	tcp::endpoint const endpoint(tcp::v4(), static_cast<uint16_t>(port->Value()));

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	start_accept();
}

bool server::check_error(error_code const& err)
{
	if ( err )
	{
		rt_.main_loop().schedule(boost::bind(&server::handle_error_v8, this, err));
		return false;
	}
	return true;
}

void server::start_accept()
{
	socket* s = new socket(rt_);
	v8pp::class_<socket>::import_external(rt_.isolate(), s);
	acceptor_.async_accept(s->impl_, boost::bind(&server::handle_accept, this, s, placeholders::error));
}

void server::handle_accept(socket* s, error_code const& err)
{
	if ( check_error(err) )
	{
		rt_.main_loop().schedule(boost::bind(&server::handle_accept_v8, this, s));
	}
}

void server::handle_accept_v8(socket* s)
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> args[1] = { v8pp::to_v8(isolate, s) };
	emit(isolate, "accept", 1, args);
	start_accept();
}

void server::handle_error_v8(error_code err)
{
	v8::Isolate* isolate = rt_.isolate();

	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> args[1] = { v8pp::to_v8(isolate, err.message()) };
	if (!emit(isolate, "error", 1, args))
	{
		rt_.error("tcp server: %s\n", err.message().c_str());
	}
}

}} // aspect::tcp
