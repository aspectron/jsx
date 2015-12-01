//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_LOG_HPP_INCLUDED
#define JSX_LOG_HPP_INCLUDED

#include "jsx/threads.hpp"
#include "jsx/v8_core.hpp"

#include <vector>

#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

// TODO - create runtime logger and create multiplexer delegate

namespace aspect {

class runtime;

namespace log
{
	enum
	{
		LEVEL_TRACE,
		LEVEL_DEBUG,
		LEVEL_INFO,
		LEVEL_ALERT,
		LEVEL_WARNING,
		LEVEL_ERROR,
		LEVEL_PANIC,

		FLAG_USER		= 0x00010000,	// message is intended for user
		FLAG_LOCAL		= 0x00020000,	// message is intended for application console only (no propagation)
		FLAG_REMOTE		= 0x00040000,	// message is intended for remote (web) console only
		FLAG_OBJECT		= 0x01000000,	// json object

		LOG_LEVEL_MASK	= 0x0000ffff,	// levels
		LOG_FLAG_MASK	= 0xffff0000	// flags
	};
}

class logger_delegate;
struct log_message;

typedef boost::shared_ptr<aspect::logger_delegate> logger_delegate_ptr;
typedef boost::shared_ptr<aspect::log_message> message_ptr;

struct CORE_API log_message
{
	uint32_t    type;
	std::string origin;
	std::string msg;

	log_message(uint32_t type, std::string const& origin, std::string const& msg)
		: type(type)
		, origin(origin)
		, msg(msg)
	{
	}
};

class CORE_API logger_delegate : public boost::enable_shared_from_this<logger_delegate>
{
public:
	virtual ~logger_delegate() {}

	void output(uint32_t type, const char *origin, const char *fmt, ...);

	virtual void output_impl(uint32_t type, const char *origin, const char *msg) = 0;
};

class CORE_API logger_delegate_console : public logger_delegate
{
public:
	explicit logger_delegate_console(runtime const& rt)
		: rt_(rt)
	{
	}

private:
	virtual void output_impl(uint32_t type, const char *origin, const char *msg);
	static boost::mutex mutex_;
	runtime const& rt_;
};

class CORE_API logger_delegate_multiplexor : public logger_delegate
{
public:
	void attach(logger_delegate_ptr target);

	virtual void output_impl(uint32_t type, const char *origin, const char *msg);

private:
	typedef std::vector<logger_delegate_ptr> logger_list;

	boost::mutex mutex_;
	logger_list targets_;
};


class CORE_API logger_delegate_async : public logger_delegate
{
public:
	logger_delegate_async()
#pragma warning(suppress: 4355)
		: thread_(&logger_delegate_async::thread_fn, this)
	{
	}

	~logger_delegate_async()
	{
		stop();
	}

	void stop()
	{
		queue_.push(message_ptr());
		thread_.join();
	}

protected:
	virtual void output_impl(uint32_t type, const char *origin, const char *msg)
	{
		queue_.push(boost::make_shared<log_message>(type, origin, msg));
	}

	virtual void digest_async(message_ptr& msg) = 0;

private:
	void thread_fn();

	boost::thread thread_;
	aspect::threads::concurrent_queue<message_ptr> queue_;
};

/* ZZZ:temporary disabled
class CORE_API logger_delegate_redirector : public logger_delegate_async
{
public:

	static const uint32_t MAX_RECORDS_PENDING = 2048;

	logger_delegate_redirector();

	~logger_delegate_redirector();

	void connect(std::string const& address);

	void flush();

private:
	virtual void output_impl(uint32_t type, const char *origin, const char *msg)
	{
		if (type & log::FLAG_LOCAL)
			return;
		logger_delegate_async::output_impl(type, origin, msg);
	}


	virtual void digest_async(message_ptr& msg);
private:
	boost::mutex            mutex_;
	std::queue<message_ptr>	queue_;
	zmq::socket_t socket_;
	bool established_;
};
*/

class CORE_API logger
{
public:
	static void setup_bindings(v8pp::module& target);

	logger();
	explicit logger(runtime const& rt);

	virtual ~logger();

public:

	void set_origin(std::string const& origin) { origin_ = origin; }

	void connect_to_remote_sink(std::string const& address);

	void output(uint32_t type, std::string const& str);

	boost::shared_ptr<logger_delegate_multiplexor> get_multiplexor() { return multiplexor_; }

	std::string origin_;

private:
	boost::shared_ptr<logger_delegate_multiplexor> multiplexor_;
	logger_delegate_ptr redirector_;
	logger_delegate_ptr console_;
};

} // aspect

#endif // JSX_LOG_HPP_INCLUDED
