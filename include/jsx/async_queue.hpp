//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_ASYNC_QUEUE_HPP_INCLUDED
#define JSX_ASYNC_QUEUE_HPP_INCLUDED

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>

#include "jsx/api.hpp"

namespace aspect {

namespace v8_core { class main_loop; }

/// Thread pool for asio::io_service
class CORE_API io_thread_pool : boost::noncopyable
{
public:
	/// Create the pool with num_threads (if num_threads == 0 use number of cores)
	io_thread_pool(char const* name, unsigned num_threads);

	~io_thread_pool();

	/// boost::asio::io_service
	boost::asio::io_service& io_service() { return *io_service_; }

	/// Number of threads in thread pool
	size_t num_threads() const { return thread_pool_.size(); }

private:
	void thread_fun(unsigned num);

	std::string name_;

	boost::scoped_ptr<boost::asio::io_service> io_service_;
	boost::scoped_ptr<boost::asio::io_service::work> io_service_work_;

	boost::thread_group thread_pool_;
};

/// Asyncronous event queue in I/O thread pool.
class CORE_API async_queue : public io_thread_pool
{
public:
	/// Create the queue with num_threads in pool (if num_threads == 0 use number of cores)
	explicit async_queue(char const* name, unsigned num_threads);

	~async_queue();

	/// Worker function type
	typedef boost::function<void ()> worker;

	/// Completion callback type, non empty err_msg for error
	typedef boost::function<void (std::string err_msg)> callback;

	/// Schedule async call of worker w with optional completion callback cb.
	/// In strand means to schedule worker in the same thread of pool.
	void schedule(worker w, callback cb = callback(), bool in_strand = false);

	/// Schedule async call of worker w with completion callback cb in V8 thread.
	void schedule(v8_core::main_loop& v8_loop, worker w, callback cb);

private:
	boost::scoped_ptr<boost::asio::io_service::strand> strand_;

	/// Function object wrapper. It's called in some thread of pool
	struct call_wrapper;
};

} // namespace aspect

#endif // JSX_ASYNC_QUEUE_HPP_INCLUDED
