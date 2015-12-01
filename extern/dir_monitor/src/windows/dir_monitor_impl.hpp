//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DIR_MONITOR_IMPL_HPP
#define BOOST_ASIO_DIR_MONITOR_IMPL_HPP

#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <deque>
#include <windows.h>

namespace boost {
namespace asio {

class dir_monitor_impl
{
public:
	dir_monitor_impl()
		: run_(true)
	{
	}

	void add_directory(boost::filesystem::path const& dirname, HANDLE handle)
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		dirs_.emplace(dirname, handle);
	}

	void remove_directory(boost::filesystem::path const& dirname)
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		dirs::iterator it = dirs_.find(dirname);
		if (it != dirs_.end())
		{
			CloseHandle(it->second);
			dirs_.erase(it);
		}
	}

	path_list directories() const
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		path_list result;
		result.reserve(dirs_.size());
		for (dirs::const_iterator it = dirs_.begin(), end = dirs_.end(); it != end; ++it)
		{
			result.push_back(it->first);
		}
		return result;
	}

	void destroy()
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		for (dirs::iterator it = dirs_.begin(), end = dirs_.end(); it != end; ++it)
		{
			CloseHandle(it->second);
		}
		dirs_.clear();
		run_ = false;
		events_cond_.notify_all();
	}

	dir_monitor_event popfront_event(boost::system::error_code &ec)
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		while (run_ && events_.empty())
			events_cond_.wait(lock);
		dir_monitor_event ev;
		if (!events_.empty())
		{
			ec = boost::system::error_code();
			ev = events_.front();
			events_.pop_front();
		}
		else
			ec = boost::asio::error::operation_aborted;
		return ev;
	}

	void pushback_event(dir_monitor_event const& ev)
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		if (run_)
		{
			events_.push_back(ev);
			events_cond_.notify_all();
		}
	}

private:
	typedef boost::unordered_map<boost::filesystem::path, HANDLE> dirs;
	dirs dirs_;
	mutable boost::mutex events_mutex_;
	boost::condition_variable events_cond_;
	bool run_;
	std::deque<dir_monitor_event> events_;
};

}
}

#endif
