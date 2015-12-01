//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_WEBSOCKET_HIXIE_76_HPP_INCLUDED
#define JSX_WEBSOCKET_HIXIE_76_HPP_INCLUDED

#include "jsx/websocket.hpp"

namespace aspect {

/// hixie-76 websocket protocol implementation
/// http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-76

class websocket_hixie_76 : public websocket
{
public:
	websocket_hixie_76(runtime& rt, pion::tcp::connection_ptr tcp_conn, v8_core::event_emitter* emitter)
		: websocket(rt, tcp_conn, emitter)
	{
	}

private:

	bool handshake(pion::http::request_ptr http_request, pion::http::response_ptr http_response);
	bool handshake(url const& target);

	void do_close();

	void do_write(void const* data, size_t size, bool is_binary, bool async);

	void write_binary(void const* data, size_t size, bool async);
	void write_text(void const* data, size_t size, bool async);

	void wait_data();

	void frame_started(boost::system::error_code err, size_t bytes_transferred);
	void on_frame_data(boost::system::error_code err, size_t bytes_transferred, bool is_binary);
	void on_write_completed(shared_buffer buf, boost::system::error_code err, size_t bytes_transferred);

	void fetch_frame_data_temp(boost::system::error_code err, size_t bytes_transferred, bool is_binary);

	uint8_t frame_type_;
	boost::asio::streambuf read_buf_;
};

} // namespace aspect

#endif // JSX_WEBSOCKET_HIXIE_76_HPP_INCLUDED
