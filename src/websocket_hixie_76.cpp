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
#include "jsx/websocket_hixie_76.hpp"

#include "jsx/url.hpp"
#include "jsx/crypto.hpp"

namespace aspect {

using boost::system::error_code;

namespace {

template<typename T>
T rand(T max)
{
	return std::rand() % max;
}

char rand_byte()
{
	return static_cast<char>(rand<unsigned char>(0xFF));
}

/// Protocol key data
struct key_data
{
	int64_t number;
	int64_t spaces;
};

/// Generate sec-key in protocol 76 at client side
string generate_sec_key(uint32_t& number)
{
	uint32_t const spaces = rand(12U) + 1;
	uint32_t const max = (rand(std::numeric_limits<uint32_t>::max()) / spaces) + 1;
	number = rand(max) + 1;
	int64_t const product = number * spaces;

	string key = std::to_string(product);

	// insert random characters into the key
	uint32_t const num_chars = rand(12U) + 1;
	for (size_t i = 0; i != num_chars; ++i)
	{
		string::size_type const pos = rand(key.length()) + 1;
		char ch = rand<char>(14 + 68);
		ch = (ch <= 14? ch + 0x21 : ch - 14 + 0x3a);
		key.insert(pos, 1, ch);
	}

	// insert spaces into the key
	for (size_t i = 0; i != spaces; ++i)
	{
		string::size_type const pos = rand(key.length() - 1) + 1;
		key.insert(pos, 1, ' ');
	}
	assert(key.front() != ' ' && key.back() != ' ');

	return key;
}

/// Parse key number and space count for a sec-key in protocol 76
key_data parse_key(string const& key)
{
	key_data result = {0};

	for (string::const_iterator it = key.begin(), end = key.end(); it != end; ++it)
	{
		if ( isdigit(*it) )
		{
			result.number = result.number * 10 + (*it - '0');
		}
		else if ( *it == ' ' )
		{
			++result.spaces;
		}
	}
	return result;
}

/// Generate handshake response for web socket protocol
string response_str(pion::http::request const& http_request, pion::tcp::connection& tcp_conn)
{
	// Get sec. keys 1 and 2
	key_data const key1 = parse_key(http_request.get_header("Sec-WebSocket-Key1"));
	key_data const key2 = parse_key(http_request.get_header("Sec-WebSocket-Key2"));

	if ( key1.spaces == 0 || key2.spaces == 0
		|| (key1.number % key1.spaces) != 0
		|| (key2.number % key2.spaces) != 0 )
	{
		// attack?
		return "";
	}

	// Get sec. key 3
	char const* key3_beg;
	char const* key3_end;
	tcp_conn.load_read_pos(key3_beg, key3_end);
	if ( key3_end - key3_beg != 8 )
	{
		// invalid key3
		return "";
	}
	tcp_conn.save_read_pos(key3_end, key3_end);

	// calculate response
	int32_t const part1 = htonl(static_cast<int32_t>(key1.number / key1.spaces));
	int32_t const part2 = htonl(static_cast<int32_t>(key2.number / key2.spaces));

	char challenge[16];
	memcpy(challenge, &part1, sizeof(part1));
	memcpy(challenge + sizeof(part1), &part2, sizeof(part2));
	memcpy(challenge + sizeof(part1) + sizeof(part2), key3_beg, key3_end - key3_beg);

	return crypto::md5_digest(challenge, sizeof(challenge));
}

uint8_t const FRAME_BEGIN = 0x00;
uint8_t const FRAME_END = 0xFF;

error_code close_socket(pion::tcp::connection_ptr tcp_conn)
{
	uint8_t const CLOSE_FRAME[] = { FRAME_END, FRAME_BEGIN };

	error_code err;
	tcp_conn->write(boost::asio::buffer(CLOSE_FRAME, sizeof(CLOSE_FRAME)), err);
	return err;
}

} // namespace

/////////////////////////////////////////////////////////////////////////////
//
// websocket_hixie_76
//
bool websocket_hixie_76::handshake(pion::http::request_ptr http_request, pion::http::response_ptr http_response)
{
	_aspect_assert(state() == CONNECTING);

	string const handshake_response = response_str(*http_request, *tcp_conn_);
	if ( handshake_response.empty() )
	{
		return false;
	}

	http_response->set_status_code(101);
	http_response->set_status_message("WebSocket Protocol Handshake");

	http_response->add_header("Upgrade", "WebSocket");
	http_response->add_header("Connection", "Upgrade");

	string const& origin = http_request->get_header("Origin");
	if ( !origin.empty() )
	{
		http_response->add_header("Sec-WebSocket-Origin", origin);
	}

	bool const is_secured = url(origin).is_scheme_secured();

	string const& host = http_request->get_header("Host");
	string const location = (is_secured? "wss://" : "ws://") + host + http_request->get_resource();
	if ( !host.empty() )
	{
		http_response->add_header("Sec-WebSocket-Location", location);
	}


	string const& subprotocol = http_request->get_header("Sec-WebSocket-Protocol");
	if ( !subprotocol.empty() )
	{
		http_response->add_header("Sec-WebSocket-Protocol", subprotocol);
	}

	http_response->set_content(handshake_response);
	return true;
}

bool websocket_hixie_76::handshake(url const& target)
{
	_aspect_assert(state() == CONNECTING);

	// create websocket handshake request
	pion::http::request req(target.path_for_request());

	req.add_header("Connection", "Upgrade");
	req.add_header("Upgrade", "WebSocket");

	req.add_header("Host", target.hostport());
	req.add_header("Origin", target.origin());

	//srand(time(NULL));
	uint32_t number1, number2;
	req.add_header("Sec-WebSocket-Key1", generate_sec_key(number1));
	req.add_header("Sec-WebSocket-Key2", generate_sec_key(number2));

	string sec_key3(8, ' ');
	std::generate(sec_key3.begin(), sec_key3.end(), rand_byte);
	req.set_content(sec_key3);

	// save expected server response
	number1 = htonl(number1);
	number2 = htonl(number2);

	char challenge[16];
	memcpy(challenge, &number1, sizeof(number1));
	memcpy(challenge + sizeof(number1), &number2, sizeof(number2));
	memcpy(challenge + sizeof(number1) + sizeof(number1), sec_key3.data(), sec_key3.size());

	string const expected = crypto::md5_digest(challenge, sizeof(challenge));

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
	if ( !is_websocket_upgrade(resp)
		|| resp.get_header("Sec-Websocket-Origin") != req.get_header("Origin") )
	{
		// is not websocket connection upgrade, give a chance for another protocol
		return false;
	}

	// Get reply
	string reply;

	char const* reply_beg;
	char const* reply_end;
	tcp_conn_->load_read_pos(reply_beg, reply_end);

	if ( reply_beg == reply_end )
	{
		char buf[16];
		size_t r = tcp_conn_->read(boost::asio::buffer(buf), boost::asio::transfer_all(), err);
		reply.assign(buf, r);
		check_err(err, "read reply");
	}
	else
	{
		reply.assign(reply_beg, reply_end);
		tcp_conn_->save_read_pos(reply_end, reply_end);
	}

	if ( reply != expected )
	{
		throw std::runtime_error("Invalid server reply on websocket handshake");
	}

	state_ = OPEN;
	on_connect(target.to_string());

	wait_data();
	return true;
}

void websocket_hixie_76::do_close()
{
	check_err(close_socket(tcp_conn_), "close");
}

void websocket_hixie_76::do_write(void const* data, size_t size, bool is_binary, bool async)
{
	if ( is_binary )
	{
		write_binary(data, size, async);
	}
	else
	{
		write_text(data, size, async);
	}
}

void websocket_hixie_76::write_binary(void const* data, size_t size, bool async)
{
	shared_buffer buf = boost::make_shared<buffer>();
	buf->reserve(size + 8);

	buf->push_back(0x80);
	for (size_t length = size; length != 0; length >>= 7)
	{
		buf->push_back(static_cast<uint8_t>(length & 0x7F));
	}

	if ( async )
	{
		uint8_t const* src = static_cast<uint8_t const*>(data);
		buf->insert(buf->end(), src, src + size);
		tcp_conn_->async_write(boost::asio::buffer(*buf),
			boost::bind(&websocket_hixie_76::on_write_completed, this, buf,
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

void websocket_hixie_76::write_text(void const* data, size_t size, bool async)
{
	boost::array<boost::asio::const_buffer, 3> bufs;

	bufs[0] = boost::asio::buffer(&FRAME_BEGIN, sizeof(FRAME_BEGIN));
	if ( async )
	{
		uint8_t const* src = static_cast<uint8_t const*>(data);
		shared_buffer buf = boost::make_shared<buffer>(src, src + size);
		bufs[1] = boost::asio::buffer(*buf);
		bufs[2] = boost::asio::buffer(&FRAME_END, sizeof(FRAME_END));
		tcp_conn_->async_write(bufs, boost::bind(&websocket_hixie_76::on_write_completed, this,
			buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		bufs[1] = boost::asio::buffer(data, size);
		bufs[2] = boost::asio::buffer(&FRAME_END, sizeof(FRAME_END));
		error_code err;
		tcp_conn_->write(bufs, err);
		check_err(err, "send");
	}
}

void websocket_hixie_76::on_write_completed(shared_buffer buf, error_code err, size_t bytes_transferred)
{
	check_err(!err && bytes_transferred >= buf->size(), "async send");
}

void websocket_hixie_76::wait_data()
{
	if ( state() != OPEN )
	{
		return;
	}

	tcp_conn_->async_read(boost::asio::buffer(&frame_type_, sizeof(frame_type_)), boost::asio::transfer_all(),
		boost::bind(&websocket_hixie_76::frame_started, this,
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void websocket_hixie_76::frame_started(error_code err, size_t bytes_transferred)
{
	if ( is_closed(err, bytes_transferred) )
	{
		return;
	}

	if ( !check_err(err, "frame started") ||
		 !check_err(bytes_transferred == sizeof(frame_type_), "frame started") )
	{
		return;
	}

	if ( frame_type_ & 0x80 )
	{
		// binary frame
		size_t length = 0;
		uint8_t b;
		do
		{
			boost::asio::read(*tcp_conn_, boost::asio::buffer(&b, sizeof(b)), err);
			if ( !check_err(err, "frame length") )
			{
				return;
			}
			if ( !b )
			{
				break;
			}
			length = length * 128 + (b & 0x7F);
		}
		while ( b & 0x80 );

		boost::asio::async_read(*tcp_conn_, read_buf_.prepare(length),
			boost::bind(&websocket_hixie_76::on_frame_data, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, true));
	}
	else
	{
		// text frame

#if 1 // TESTING - WORKING !

		/*

		PAVEL - This works!

		Thus I beleive the problem in the original method is either with async_read_until() on in buffer handling...
		last byte 0xff is either somehow getting left/stuck in the buffer queue..

		or next message gets swallowed...

		I saw multiple mentions on the net that async_read_until() WILL READ DATA PAST DELIMITER; which means that it can
		really screw up our framing!

		*/

		// This is just a regular old-fashion synchronous read which works just fine!
		buffer data;
		size_t length = 0;
		uint8_t b;
		do
		{
			boost::asio::read(*tcp_conn_, boost::asio::buffer(&b, sizeof(b)), err);
			if ( !check_err(err, "reading frame (test)") )
				return;
			if ( b == 0xff )	// break on delimiter
				break;
			data.push_back(b);
		}
		while ( true );

		// printf("GOT MESSAGE: %s\n",data->c_str());

		// give to v8
		on_data(data, false);

		// schedule another frame
		wait_data();

#else	// OLD - NON WORKING!

		read_buf_.prepare(512);
		boost::asio::async_read_until(*tcp_conn_, read_buf_, char(FRAME_END),
			boost::bind(&websocket_hixie_76::on_frame_data, shared_from_this(),
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, false));
#endif
	}
}

void websocket_hixie_76::on_frame_data(error_code err, size_t bytes_transferred, bool is_binary)
{
	if ( is_closed(err, bytes_transferred) )
	{
		return;
	}

	if ( !check_err(err, "frame read") )
	{
		return;
	}

	read_buf_.commit(bytes_transferred);
	std::istreambuf_iterator<char> in(&read_buf_);

	buffer data;
	data.reserve(bytes_transferred);
	std::copy_n(in, bytes_transferred, std::back_inserter(data));
	if ( !is_binary )
	{
		assert((frame_type_ & 0x80) == 0);
		// erase last 0xFF in text frame data
		if ( !data.empty() )
		{
			data.pop_back();
		}
	}

	on_data(data, is_binary);

	wait_data();
}

} // namespace aspect
