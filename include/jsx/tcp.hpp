//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_TCP_HPP_INCLUDED
#define JSX_TCP_HPP_INCLUDED

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/scoped_ptr.hpp>

#include "jsx/events.hpp"
#include "jsx/types.hpp"

namespace aspect { namespace tcp {

void setup_bindings(v8pp::module& target);

using boost::system::error_code;
using boost::asio::ip::tcp;

/// TCP socket
class CORE_API socket : public v8_core::event_emitter
{
public:
	/// Create a socket
	explicit socket(v8::FunctionCallbackInfo<v8::Value> const& args);
	explicit socket(runtime& rt);

	/// Close socket on destroy
	~socket();

	/// Close socket
	void close();

	/// Connected remote endpoint, returns string in form of ip-address:port
	std::string remote_endpoint();

	/// Connected local endpoint, returns string in form of ip-address:port
	std::string local_endpoint();

public:
	// Synchronous interface

	/// Open and connect socket to host:port, return 'this'
	socket& connect(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Send data, return number of bytes sent
	size_t send(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Send data
	void send_data(void const* data, size_t size);

	/// Read data into buffer, return number of bytes
	size_t receive(v8::FunctionCallbackInfo<v8::Value> const& args);

public:
	// Asynchronous interface

	/// Open and connect socket to host:port asynchronously, return 'this'
	socket& async_connect(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Send data asynchronously, return 'this'
	socket& async_send(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Receive data asynchronously, return 'this'
	socket& async_receive(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Cancel pending asynchronous operations
	void cancel();

private:
	friend class server; // for access to impl_
	tcp::socket impl_;

	runtime& rt_;

	bool check_error(error_code const& err);
	void start_receive(size_t buf_size);

	void handle_resolve(error_code const& err, tcp::resolver::iterator iterator);
	void handle_connect(error_code const& err, tcp::resolver::iterator iterator);
	void handle_write(v8::String::Utf8Value* buf, error_code const& err, size_t bytes_transferred);
	void handle_read(shared_buffer buf, error_code const& err, size_t bytes_transferred);

	void handle_connect_v8();
	void handle_close_v8();
	void handle_error_v8(error_code err);
	void handle_write_v8(size_t bytes_transferred);
	void handle_read_v8(shared_buffer buf, size_t buf_size);
};

/// TCP server
class CORE_API server : public v8_core::event_emitter
{
public:
	/// Create a server
	explicit server(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Stop server on destroy
	~server();

	/// Run server on port with specified handler
	void run(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Stop server
	void stop();

private:
	bool check_error(error_code const& err);

	void start_accept();
	void handle_accept(socket* s, error_code const& err);

	void handle_accept_v8(socket* s);
	void handle_error_v8(error_code err);

	tcp::acceptor acceptor_;

	runtime& rt_;
};

}} // aspect::tcp


#endif // JSX_TCP_HPP_INCLUDED
