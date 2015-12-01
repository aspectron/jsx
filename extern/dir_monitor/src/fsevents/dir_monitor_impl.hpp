//
// FSEvents-based implementation for OSX
//
// Apple documentation about FSEvents interface:
// https://developer.apple.com/library/mac/documentation/Darwin/Reference/FSEvents_Ref/
//     Reference/reference.html#//apple_ref/doc/c_ref/kFSEventStreamCreateFlagFileEvents
//
// Copyright (c) 2014 Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/atomic.hpp>
#include <boost/filesystem.hpp>
#include <boost/unordered_set.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <string>
#include <deque>
#include <boost/thread.hpp>
#include <CoreServices/CoreServices.h>

namespace boost {
namespace asio {

class dir_monitor_impl
{
public:
	dir_monitor_impl()
		: run_(true)
		, work_thread_(&boost::asio::dir_monitor_impl::work_thread, this)
		, fsevents_()
	{
	}

	void add_directory(boost::filesystem::path const& dirname, bool watch_subdirs)
	{
		boost::unique_lock<boost::mutex> lock(dirs_mutex_);
		dirs_.insert(dirname);
		stop_fsevents();
		start_fsevents();
	}

	void remove_directory(boost::filesystem::path const& dirname)
	{
		boost::unique_lock<boost::mutex> lock(dirs_mutex_);
		dirs_.erase(dirname);
		stop_fsevents();
		start_fsevents();
	}

	path_list directories() const
	{
		boost::unique_lock<boost::mutex> lock(dirs_mutex_);
		path_list result;
		result.reserve(dirs_.size());
		for (dirs::const_iterator it = dirs_.begin(), end = dirs_.end(); it != end; ++it)
		{
			result.push_back(*it);
		}
		return result;
	}

	void destroy()
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		run_ = false;
		events_cond_.notify_all();

		stop_work_thread();
		work_thread_.join();
		stop_fsevents();
	}

	dir_monitor_event popfront_event(boost::system::error_code &ec)
	{
		boost::unique_lock<boost::mutex> lock(events_mutex_);
		while (run_ && events_.empty()) {
			events_cond_.wait(lock);
		}
		dir_monitor_event ev;
		if (!events_.empty())
		{
			ec = boost::system::error_code();
			ev = events_.front();
			events_.pop_front();
		}
		else {
			ec = boost::asio::error::operation_aborted;
		}
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
	static CFArrayRef make_array(boost::unordered_set<boost::filesystem::path> const& dirs)
	{
		CFMutableArrayRef arr = CFArrayCreateMutable(nullptr, dirs.size(), nullptr);
		for (auto const& dir : dirs)
		{
			CFStringRef cfstr = CFStringCreateWithCString(nullptr, dir.c_str(), kCFStringEncodingUTF8);
			CFArrayAppendValue(arr, cfstr);
		}
		return arr;
	}

	void start_fsevents()
	{
		FSEventStreamContext context = {};
		context.info = this;
		fsevents_ = FSEventStreamCreate(nullptr, &fsevents_callback, &context,
				make_array(dirs_),
				kFSEventStreamEventIdSinceNow, /* only new modifications */
				0, // no latency
				kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents);

		if (!fsevents_)
		{
			boost::system::system_error e(boost::system::error_code(errno, boost::system::get_system_category()), "boost::asio::dir_monitor_impl FSEventStreamCreate failed");
			boost::throw_exception(e);
		}

		while (!runloop_)
		{
			boost::this_thread::yield();
		}

		FSEventStreamScheduleWithRunLoop(fsevents_, runloop_, kCFRunLoopDefaultMode);
		FSEventStreamStart(fsevents_);
		runloop_cond_.notify_all();
	}

	void stop_fsevents()
	{
		if (fsevents_)
		{
			FSEventStreamStop(fsevents_);
			FSEventStreamInvalidate(fsevents_);
			FSEventStreamRelease(fsevents_);
		}
	}

	static void fsevents_callback(
			ConstFSEventStreamRef streamRef,
			void *clientCallBackInfo,
			size_t numEvents,
			void *eventPaths,
			const FSEventStreamEventFlags eventFlags[],
			const FSEventStreamEventId eventIds[])
	{
		dir_monitor_impl* impl = (dir_monitor_impl*)clientCallBackInfo;

		char **paths = (char**)eventPaths;
		bool rename_old = true;

		for (size_t i = 0; i < numEvents; ++i)
		{
			boost::filesystem::path const path(paths[i]);
			if (eventFlags[i] & kFSEventStreamEventFlagMustScanSubDirs)
			{
				impl->pushback_event(dir_monitor_event(path, dir_monitor_event::recursive_rescan));
			}
			if (eventFlags[i] & kFSEventStreamEventFlagItemCreated)
			{
				impl->pushback_event(dir_monitor_event(path, dir_monitor_event::added));
			}
			if (eventFlags[i] & kFSEventStreamEventFlagItemModified)
			{
				impl->pushback_event(dir_monitor_event(path, dir_monitor_event::modified));
			}
			// We assume renames always come in pairs inside a single callback, old name first.
			// This might be wrong in general, but I haven't seen evidence yet.
			if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed)
			{
				impl->pushback_event(dir_monitor_event(path, rename_old? dir_monitor_event::renamed_old_name : dir_monitor_event::renamed_new_name));
				rename_old = !rename_old;
			}
			if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved)
			{
				impl->pushback_event(dir_monitor_event(path, dir_monitor_event::removed));
			}
	   }
	}

	void work_thread()
	{
		runloop_ = CFRunLoopGetCurrent();

		while (run_)
		{
			boost::unique_lock<boost::mutex> lock(runloop_mutex_);
			runloop_cond_.wait(lock);
			CFRunLoopRun();
		}
	}

	void stop_work_thread()
	{
		run_ = false;
		CFRunLoopStop(runloop_); // exits the thread
		runloop_cond_.notify_all();
	}

	boost::atomic<bool> run_;
	CFRunLoopRef runloop_;
	boost::mutex runloop_mutex_;
	boost::condition_variable runloop_cond_;

	boost::thread work_thread_;

	FSEventStreamRef fsevents_;

	typedef boost::unordered_set<boost::filesystem::path> dirs;
	mutable boost::mutex dirs_mutex_;
	dirs dirs_;

	boost::mutex events_mutex_;
	boost::condition_variable events_cond_;
	std::deque<dir_monitor_event> events_;
};

} // asio namespace
} // boost namespace

