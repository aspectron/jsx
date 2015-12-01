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
#include "jsx/log.hpp"

#include "jsx/runtime.hpp"
#include "jsx/console.hpp"

#include <stdarg.h>
#include <boost/format.hpp>

namespace aspect {

void logger::setup_bindings(v8pp::module& target)
{
	/**
	@module logger - Logger
	@class logger
	**/
	v8pp::class_<logger> logger_class(target.isolate());
	logger_class
		/**
		@function logger.output(type, string)
		@param type {Number}
		@param string {String}
		Add event `string` to log
		**/
		.set("output", &logger::output)
		/**
		@function connectToRemoteSink(address)
		@param address {String} remote sink address
		Connect logger to a remote sink at address `address`
		**/
		.set("connectToRemoteSink", &logger::connect_to_remote_sink)
		/**
		@property origin {String} - Logger origin
		**/
		.set("origin", &logger::origin_)
		;
	target.set("logger", logger_class);
}

logger::logger()
{
	multiplexor_.reset(new logger_delegate_multiplexor);
	//ZZZ: multiplexor_->attach(redirector_);
	//ZZZ: origin_ = runtime::get_module(); //"RTE";
}

logger::logger(runtime const& rt)
	: console_(boost::make_shared<logger_delegate_console>(rt))
{
	multiplexor_.reset(new logger_delegate_multiplexor);
	multiplexor_->attach(console_);
}


logger::~logger()
{
	multiplexor_.reset();
}

void logger::output(uint32_t type, std::string const& str)
{
	std::vector<std::string> x = utils::split(str.c_str(), '\n');

	for (std::vector<std::string>::iterator iter = x.begin(), end = x.end(); iter != end; ++iter)
	{
		multiplexor_->output_impl(type, origin_.c_str(), iter->c_str());
	}
}

void logger::connect_to_remote_sink(std::string const& address)
{
	(void)address;
//ZZZ:	((logger_delegate_redirector*)(redirector_.get()))->connect(address);
}

void logger_delegate::output(uint32_t type, const char *origin, const char *fmt, ...)
{
	char szBuff[8192]; // Strings longer than this get truncated
	va_list args;
	va_start(args, fmt);
	if ( vsnprintf(szBuff, sizeof(szBuff)-1, fmt, args) < 0) szBuff[sizeof(szBuff)-1] = '\0';
	va_end(args);

	output_impl(type,origin,szBuff);
}

	static boost::mutex console_mutex_;

boost::mutex logger_delegate_console::mutex_;

void logger_delegate_console::output_impl(uint32_t level, const char *origin, const char *msg)
{
	(void)origin;

	if ((level & log::FLAG_REMOTE) || (rt_.execution_options() & runtime::SILENT))
	{
		return;
	}

	boost::mutex::scoped_lock lock(mutex_);

	static const console::color fg_color[] =
	{
		console::CYAN,
		console::GREEN,
		console::WHITE,
		console::YELLOW,
		console::MAGENTA,
		console::RED,
		console::WHITE
	};

#if OS(WINDOWS) && ENABLE(MSVC_OUTPUT)
	::OutputDebugStringA(msg);
	::OutputDebugStringA("\n");
#endif

	size_t const log_level = (level & log::LOG_LEVEL_MASK);
	console::println(msg, fg_color[log_level],
		log_level == log::LEVEL_PANIC? console::RED : console::DEFAULT_COLOR);
}

void logger_delegate_multiplexor::attach(logger_delegate_ptr target)
{
	_aspect_assert(target);

	boost::mutex::scoped_lock lock(mutex_);
	targets_.push_back(target);
}

void logger_delegate_multiplexor::output_impl(uint32_t type, const char *origin, const char *msg)
{
	boost::mutex::scoped_lock lock(mutex_);
	for (logger_list::iterator it = targets_.begin(), end = targets_.end(); it != end; ++it)
	{
		(*it)->output_impl(type, origin, msg);
	}
}

void logger_delegate_async::thread_fn()
{
	message_ptr msg;

	while ( true )
	{
		queue_.wait_and_pop(msg);  // TODO-CRASH
		if ( !msg )
		{
			break;
		}
		digest_async(msg);
	}
}

/*
void logger_delegate_redirector::connect(std::string const& address)
{
	socket_.connect(address.c_str());
	established_ = true;

	flush();
}

void logger_delegate_redirector::digest_async(message_ptr& msg)
{
	{
		boost::mutex::scoped_lock lock(mutex_);
		while ( queue_.size() > MAX_RECORDS_PENDING )
		{
			queue_.pop();
		}
		queue_.push(msg);
	}

	if( established_ )
	{
		flush();
	}
}

void logger_delegate_redirector::flush()
{
//	static boost::format fmt("{\"uuid\":\"%s\",\"level\":%d,\"origin\":\"%s\",\"message\":\"%s\"}");
	static boost::format fmt("{\"huuid\":\"%s\",\"uuid\":\"%s\",\"pid\":%d,\"l\":%d,\"msg\":\"%s\"}");

	//std::string uuid = runtime::get_uuid()

	static const std::string local_system_uuid = utils::get_unique_local_system_uuid();

	static const uint32_t pid = getpid();

	boost::mutex::scoped_lock lock(mutex_);

	while( !queue_.empty() )
	{
		message_ptr msg = queue_.front();

		// send to socket

		//////////////////////////////////////////////////////////////////////////


		fmt % local_system_uuid % runtime::get_uuid() % pid % msg->type % utils::json_escape_string(msg->msg);
//		fmt % msg->type % utils::json_escape_string(msg->origin) % utils::json_escape_string(msg->msg);
		std::string const msgbuf = fmt.str();

//
// 		char szBuff[8192];
// 		if(message.length() > 8190)
// 		{
// 			std::string cut;
// 			cut = message;
// 			cut[4096] = 0;
// 			sprintf_s(szBuff,sizeof(szBuff),"{\"level\":%d,\"origin\":\"%s\",\"message\":\"WARINGING - MESSAGE TOO LONG, CROPPING - %s\"}",
// 				msg->type_, msg->origin_.c_str(), cut.c_str());
// 		}
// 		else
// 		{
// 			sprintf_s(szBuff,sizeof(szBuff),"{\"level\":%d,\"origin\":\"%s\",\"message\":\"%s\"}",
// 				msg->type_, msg->origin_.c_str(), message.c_str());
// 		}
		//////////////////////////////////////////////////////////////////////////

		bool success = true;
		try
		{
			size_t const size = msgbuf.size();
			zmq::message_t zmq_msg(size);
			memcpy(zmq_msg.data(), msgbuf.data(), size);
			socket_.send(zmq_msg, 0);
		}
		catch (zmq::error_t const& ex)
		{
			success = false;
		}

		if ( success )
		{
			queue_.pop();
		}
	}
}

logger_delegate_redirector::logger_delegate_redirector()
	: socket_(*runtime::zmq_context(), ZMQ_PUSH)
	, established_(false)
{
}

logger_delegate_redirector::~logger_delegate_redirector()
{
	stop();
	socket_.close();
}
*/

} // ::aspect
