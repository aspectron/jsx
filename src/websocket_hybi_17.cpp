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
#include "jsx/websocket_hybi_17.hpp"

#include "jsx/crypto.hpp"
#include "jsx/url.hpp"

#include <boost/detail/endian.hpp>

namespace aspect {

using boost::system::error_code;

namespace {

char rand_byte()
{
	return static_cast<char>(std::rand() % 0xFF);
}

#if !defined(ntohll) && !defined(htonll)
inline uint64_t swap_bytes(uint64_t val)
{
    return ((((val) & 0xff00000000000000ull) >> 56) |
            (((val) & 0x00ff000000000000ull) >> 40) |
            (((val) & 0x0000ff0000000000ull) >> 24) |
            (((val) & 0x000000ff00000000ull) >> 8 ) |
            (((val) & 0x00000000ff000000ull) << 8 ) |
            (((val) & 0x0000000000ff0000ull) << 24) |
            (((val) & 0x000000000000ff00ull) << 40) |
            (((val) & 0x00000000000000ffull) << 56));
}

inline uint64_t ntohll(uint64_t x)
{
#ifdef BOOST_LITTLE_ENDIAN
	return swap_bytes(x);
#else
	return x;
#endif
}

inline uint64_t htonll(uint64_t x)
{
	return ntohll(x);
}
#endif

struct opcodes
{
	static uint8_t const CONTINUATION = 0x00;

	static uint8_t const TEXT = 0x01;
	static uint8_t const BINARY = 0x02;
	static uint8_t const CLOSE = 0x08;

	static uint8_t const PING = 0x09;
	static uint8_t const PONG = 0x0A;
};

} // namespace

/////////////////////////////////////////////////////////////////////////////
//
// websocket_hybi_17
//
bool websocket_hybi_17::handshake(pion::http::request_ptr http_request, pion::http::response_ptr http_response)
{
	_aspect_assert(state() == CONNECTING);

	string key = http_request->get_header("Sec-WebSocket-Key");
	if ( key.empty() )
	{
		return false;
	}
	key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	key = crypto::sha1_digest(key.data(), key.size());

	string resp_key;
	if ( !pion::algorithm::base64_encode(key, resp_key) )
	{
		return false;
	}

	http_response->set_status_code(101);
	http_response->set_status_message("Switching Protocols");

	http_response->add_header("Upgrade", "WebSocket");
	http_response->add_header("Connection", "Upgrade");
	http_response->add_header("Sec-WebSocket-Accept", resp_key);

	return true;
}

bool websocket_hybi_17::handshake(url const& target)
{
	_aspect_assert(state() == CONNECTING);

	// create websocket handshake request
	pion::http::request req(target.path_for_request());

	req.add_header("Connection", "Upgrade");
	req.add_header("Upgrade", "WebSocket");

	req.add_header("Host", target.hostport());
	req.add_header("Origin", target.origin());

	string key(16, 0);
	std::generate(key.begin(), key.end(), rand_byte);
	string encoded_key;
	if ( !pion::algorithm::base64_encode(key, encoded_key) )
	{
		return false;
	}
	req.add_header("Sec-WebSocket-Key", encoded_key);
	req.add_header("Sec-WebSocket-Version", "13");

	// send request, recieve response
	error_code err = send_message(req);
	if ( !check_err(err, "send request") )
	{
		return false;
	}

	pion::http::response resp(req);
	resp.receive(*tcp_conn_, err);
	if ( !check_err(err, "recieve response") )
	{
		return false;
	}

	// check response
	if ( !is_websocket_upgrade(resp) )
	{
		// it is not a hybi connection upgrade, give a chance for another protocol
		return false;
	}

	// Check reply key
	encoded_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	encoded_key = crypto::sha1_digest(encoded_key.data(), encoded_key.size());
	string resp_key;
	if ( !pion::algorithm::base64_encode(encoded_key, resp_key) )
	{
		return false;
	}

	if ( resp_key != resp.get_header("Sec-WebSocket-Accept") )
	{
		return false;
	}

	state_ = OPEN;
	on_connect(target.to_string());
	wait_data();

	return true;
}

void websocket_hybi_17::do_close()
{
	uint8_t const CLOSE_FRAME[] = { 0x80 | opcodes::CLOSE, 0x00 };
	error_code err;
	tcp_conn_->write(boost::asio::buffer(CLOSE_FRAME, sizeof(CLOSE_FRAME)), err);
}

void websocket_hybi_17::do_write(void const* data, size_t size, bool is_binary, bool async)
{
	shared_buffer buf = boost::make_shared<buffer>();
	buf->reserve(size + FRAME_START_LEN + FRAME_PAYLOAD_LEN + FRAME_MASK_LEN);

	size_t frame_len = FRAME_START_LEN;

	buf->push_back(0x80 | (is_binary ? opcodes::BINARY : opcodes::TEXT));
	if ( size < 126 )
	{
		buf->push_back(static_cast<uint8_t>(size));
	}
	else if ( size <= std::numeric_limits<uint16_t>::max() )
	{
		buf->push_back(126);

		uint16_t const len = htons(static_cast<uint16_t>(size));
		buf->resize(buf->size() + sizeof(len));
		memcpy(&*(buf->begin() + frame_len), &len, sizeof(len));
		frame_len += sizeof(len);
	}
	else
	{
		_aspect_assert(size > std::numeric_limits<uint16_t>::max());
		buf->push_back(127);

		uint64_t const len = htonll(size);
		buf->resize(buf->size() + sizeof(len));
		memcpy(&*(buf->begin() + frame_len), &len, sizeof(len));
		frame_len += sizeof(len);
	}

	if ( async || is_client_side() )
	{
		uint8_t const* src = static_cast<uint8_t const*>(data);

		if ( is_client_side() )
		{
			// data from client are masked
			(*buf)[1] |= 0x80;

			uint8_t mask[FRAME_MASK_LEN];
			std::generate_n(mask, FRAME_MASK_LEN, rand_byte);

			buf->resize(buf->size() + FRAME_MASK_LEN);
			memcpy(&*(buf->begin() + frame_len), mask, FRAME_MASK_LEN);
			frame_len += FRAME_MASK_LEN;
			for (size_t i = 0; i != size; ++i)
			{
				buf->push_back(src[i] ^ mask[i % 4]);
			}
		}
		else
		{
			buf->insert(buf->end(), src, src + size);
		}
		tcp_conn_->async_write(boost::asio::buffer(*buf),
			boost::bind(&websocket_hybi_17::on_write_completed, this, buf,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		boost::array<boost::asio::const_buffer, 2> bufs;
		bufs[0] = boost::asio::buffer(*buf);
		bufs[1] = boost::asio::buffer(data, size);
		error_code err;
		tcp_conn_->write(bufs, err);
		check_err(err, "send");
	}
}

void websocket_hybi_17::on_write_completed(shared_buffer buf, error_code err, size_t bytes_transferred)
{
	check_err(!err && bytes_transferred >= buf->size(), "async send");
}

void websocket_hybi_17::wait_data()
{
	if ( state() != OPEN )
	{
		return;
	}

	tcp_conn_->async_read(boost::asio::buffer(frame_start_), boost::asio::transfer_all(),
		boost::bind(&websocket_hybi_17::frame_started, this,
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void websocket_hybi_17::frame_started(error_code err, size_t bytes_transferred)
{
	if ( is_closed(err, bytes_transferred) )
	{
		return;
	}

	if ( !check_err(err, "Frame start") ||
		 !check_err(bytes_transferred == sizeof(frame_start_), "Frame start") )
	{
		return;
	}

	payload_len_ = (frame_start_[1] & 0x7F);
	if ( payload_len_ == 126 )
	{
		uint16_t len;
		boost::asio::read(*tcp_conn_, boost::asio::buffer(&len, sizeof(len)), err);
		payload_len_ = ntohs(len);
	}
	else if ( payload_len_ == 127 )
	{
		uint64_t len = 0;
		boost::asio::read(*tcp_conn_, boost::asio::buffer(&len, sizeof(len)), err);
		payload_len_ = static_cast<size_t>(ntohll(len));
	}
	if ( !check_err(err, "Payload length") )
	{
		return;
	}
	if ( !check_err(payload_len_ < MAX_DATA_SIZE, "Payload length exceeded") )
	{
		return;
	}

	bool const is_masked = (frame_start_[1] & 0x80) != 0;
	//check_err(is_masked == !is_client_side(), "Frame from client should be masked");

	if ( is_masked )
	{
		boost::asio::read(*tcp_conn_, boost::asio::buffer(frame_mask_), err);
		if ( !check_err(err, "Payload mask") )
		{
			return;
		}
	}

	boost::asio::async_read(*tcp_conn_, read_buf_.prepare(payload_len_),
		boost::bind(&websocket_hybi_17::on_frame_payload, this,
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void websocket_hybi_17::on_frame_payload(boost::system::error_code err, size_t bytes_transferred)
{
	if ( is_closed(err, bytes_transferred) )
	{
		return;
	}

	if ( !check_err(err, "frame read") ||
		 !check_err(bytes_transferred == payload_len_, "frame read") )
	{
		return;
	}

	bool const is_final_fragment = (frame_start_[0] & 0x80) != 0;
	if ( is_final_fragment )
	{
		read_buf_.commit(bytes_transferred);
	}

	uint8_t const opcode = (frame_start_[0] & 0x0F);

	if ( !check_err(is_final_fragment || !opcode, "Invalid continuation frame") )
	{
		return;
	}

	if ( opcode & 0x8 )
	{
		// control opcode
		if ( !check_err(payload_len_ < 126 && is_final_fragment, "Invalid control frame") )
		{
			return;
		}
	}

	switch ( opcode )
	{
	case opcodes::CONTINUATION:
		check_err(!is_final_fragment, "Invalid continuation frame");
		break;
	case opcodes::TEXT:
		handle_data(false);
		break;
	case opcodes::BINARY:
		handle_data(true);
		break;
	case opcodes::CLOSE:
		handle_close();
		break;
	case opcodes::PING:
		break;
	case opcodes::PONG:
		// do nothing
		break;
	default:
		check_err(false, "Unknown frame type");
		break;
	}

	wait_data();
}

void websocket_hybi_17::handle_data(bool is_binary)
{
	if ( state() != OPEN )
	{
		return;
	}

	std::istreambuf_iterator<char> beg(&read_buf_), end;
	buffer data(beg, end);

	if ( !is_client_side() )
	{
		bool const is_masked = (frame_start_[1] & 0x80) != 0;
		_aspect_assert(is_masked);
		for (size_t i = 0, count = data.size(); i != count; ++i)
		{
			data[i] ^= frame_mask_[i % 4];
		}
	}
	on_data(data, is_binary);
}

void websocket_hybi_17::handle_close()
{
	if ( state() != OPEN )
	{
		return;
	}

	//TODO: close status and reason
	/*
	{
	std::istreambuf_iterator<char> beg(&read_buf_), end;
	if ( beg != end )
	{
		uint16_t const status = (*beg++) << 8 | (*beg++);
		if ( beg != end )
		{
			std::string const reason(beg, end);
		}
	}
	*/
	close();
}

void websocket_hybi_17::handle_ping()
{
	if ( state() != OPEN )
	{
		return;
	}

	boost::array<boost::asio::const_buffer, 2> bufs;

	uint8_t frame[2];
	frame[0] = 0x80 | opcodes::PONG;
	frame[1] = static_cast<uint8_t>(std::min<size_t>(125, payload_len_));

	bufs[0] = boost::asio::buffer(frame);
	bufs[1] = read_buf_.data();

	error_code err;
	tcp_conn_->write(bufs, err);
	check_err(err, "Pong failed");
}

} // namespace aspect
