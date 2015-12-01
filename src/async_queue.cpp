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
#include "jsx/async_queue.hpp"

#include "jsx/os.hpp"
#include "jsx/v8_main_loop.hpp"

namespace aspect {

io_thread_pool::io_thread_pool(char const* name, unsigned num_threads)
	: name_(name? name : "")
{
	if ( num_threads == 0 )
	{
		num_threads = std::max(1U, boost::thread::hardware_concurrency());
	}

	// Create asio::io_service and hint it with the number of threads
	io_service_.reset(new boost::asio::io_service(num_threads));

	// Create io_service worker to prevent the io_service stop on empty queue
	io_service_work_.reset(new boost::asio::io_service::work(*io_service_));

	// Create threads in pool
	for (unsigned thread_num = 0; thread_num != num_threads; ++thread_num)
	{
		thread_pool_.create_thread(boost::bind(&io_thread_pool::thread_fun, this, thread_num));
	}
}

io_thread_pool::~io_thread_pool()
{
	// Destroy io_service worker
	io_service_work_.reset();
	io_service_->stop();
	/// Wait for threads in the pool
	///TODO: join threads with timeout?
	thread_pool_.join_all();
}

void io_thread_pool::thread_fun(unsigned num)
{
#if OS(WINDOWS)
	os::set_thread_exception_handlers();
#endif

	if ( !name_.empty() )
	{
		std::string const thread_name = name_ + " thread#" + std::to_string(static_cast<uint64_t>(num));
		os::set_thread_name(thread_name.c_str());
	}
	io_service_->run();
}

async_queue::async_queue(char const* name, unsigned num_threads)
	: io_thread_pool(name, num_threads)
{
	strand_.reset(new boost::asio::io_service::strand(io_service()));
}

async_queue::~async_queue()
{
	strand_.reset();
}

/// Function object wrapper. It's called in some thread of pool
struct async_queue::call_wrapper
{
	async_queue::worker w;
	async_queue::callback cb;

	call_wrapper(async_queue::worker w, async_queue::callback cb)
		: w(w)
		, cb(cb)
	{
	}

	void operator()()
	{
		std::string err_msg;
		try
		{
			w();
		}
		catch (std::exception const& ex)
		{
			// log error
			err_msg = ex.what()? ex.what() : "";
			if ( err_msg.empty() )
			{
				err_msg = typeid(ex).name();
			}
		}
		catch (...)
		{
			// log error
			err_msg = "unknown exception";
		}

		// Try call the worker completion callback
		if ( cb )
		try
		{
			cb(err_msg);
		}
		catch (std::exception const&)
		{
			//?? log error
		}
		catch (...)
		{
			//?? log error
		}
	}
};

void async_queue::schedule(worker w, callback cb, bool in_strand)
{
	if ( !w )
	{
		throw std::invalid_argument("empty worker");
	}

	/// Wrap worker and callback into function object
	call_wrapper const f(w, cb);

	// post wrapped function into the io_service
	if ( in_strand )
	{
		strand_->post(f);
	}
	else
	{
		io_service().post(f);
	}
}

struct v8_callback
{
	v8_core::main_loop& v8_loop;
	async_queue::callback cb;

	v8_callback(v8_core::main_loop& v8_loop, async_queue::callback cb)
		: v8_loop(v8_loop)
		, cb(cb)
	{
	}

	void operator()(std::string err_msg)
	{
		v8_loop.schedule(boost::bind(cb, err_msg));
	}
};

void async_queue::schedule(v8_core::main_loop& v8_loop, worker w, callback cb)
{
	schedule(w, v8_callback(v8_loop, cb));
}

} // namespace aspect
