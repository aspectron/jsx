//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef WEBSOCKET_HYBI_17_HPP_INCLUDED
#define WEBSOCKET_HYBI_17_HPP_INCLUDED

#include "jsx/websocket.hpp"

namespace aspect {

/// hybi-17 websocket protocol implementation
/// http://tools.ietf.org/wg/hybi/draft-ietf-hybi-thewebsocketprotocol/

class websocket_hybi_17 : public websocket
{
public:
	websocket_hybi_17(runtime& rt, pion::tcp::connection_ptr tcp_conn, v8_core::event_emitter* emitter)
		: websocket(rt, tcp_conn, emitter)
	{
	}

private:

	bool handshake(pion::http::request_ptr http_request, pion::http::response_ptr http_response);
	bool handshake(url const& target);

	void do_close();
	void do_write(void const* data, size_t size, bool is_binary, bool async);
	void wait_data();

	void frame_started(boost::system::error_code err, size_t bytes_transferred);
	void on_frame_payload(boost::system::error_code err, size_t bytes_transferred);
	void on_write_completed(shared_buffer buf, boost::system::error_code err, size_t bytes_transferred);

	void handle_data(bool is_binary);
	void handle_close();
	void handle_ping();

	static size_t const FRAME_START_LEN = 2;
	static size_t const FRAME_MASK_LEN = 4;
	static size_t const FRAME_PAYLOAD_LEN = sizeof(uint64_t);

	uint8_t frame_start_[2];
	uint8_t frame_mask_[4];
	size_t payload_len_;
	boost::asio::streambuf read_buf_;
};

} // namespace aspect

#endif // WEBSOCKET_HYBI_17_HPP_INCLUDED
