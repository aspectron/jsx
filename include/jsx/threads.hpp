//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_THREADS_HPP_INCLUDED
#define JSX_THREADS_HPP_INCLUDED

#include <queue>

#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace aspect { namespace threads {

/// Concurrent queue for data type T
template<typename T>
class concurrent_queue
{
public:

	/// Clear the queue
	void clear()
	{
		boost::mutex::scoped_lock lock(mtx_);
		while ( !data_.empty() )
		{
			data_.pop();
		}
	}

	/// Is the queue is empty?
	bool empty() const
	{
		boost::mutex::scoped_lock lock(mtx_);

		return data_.empty();
	}

	/// What is the size of the queue?
	size_t size() const
	{
		boost::mutex::scoped_lock lock(mtx_);

		return data_.size();
	}

	/// Push item to the queue
	void push(T const& value)
	{
		boost::mutex::scoped_lock lock(mtx_);

		data_.push(value);
		lock.unlock();

		cv_.notify_one();
	}

	/// Try to pop item from the queue, return true on success
	bool try_pop(T& popped_value)
	{
		boost::mutex::scoped_lock lock(mtx_);

		if ( data_.empty() )
		{
			return false;
		}

		popped_value = data_.front();
		data_.pop();
		return true;
	}

	/// Wait for an item in the queue and pop it.
	void wait_and_pop(T& popped_value)
	{
		boost::mutex::scoped_lock lock(mtx_);
		while ( data_.empty() )
		{
			cv_.wait(lock);
		}

		popped_value = data_.front();
		data_.pop();
	}

private:
	std::queue<T> data_;
	mutable boost::mutex mtx_;
	boost::condition_variable cv_;
};

}} // aspect::threads

#endif // JSX_THREADS_HPP_INCLUDED
