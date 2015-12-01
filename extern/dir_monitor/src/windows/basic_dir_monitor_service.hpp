//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_DIR_MONITOR_SERVICE_HPP
#define BOOST_ASIO_BASIC_DIR_MONITOR_SERVICE_HPP

#include "dir_monitor_impl.hpp"
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <stdexcept>
#include <windows.h>

namespace boost {
namespace asio {

template <typename DirMonitorImplementation = dir_monitor_impl>
class basic_dir_monitor_service
	: public boost::asio::io_service::service
{
public:
	struct completion_key
	{
		completion_key(HANDLE h, boost::filesystem::path const& d, bool w, boost::shared_ptr<DirMonitorImplementation> &i)
			: handle(h),
			dirname(d),
			watch_subdirs(w),
			impl(i)
		{
			ZeroMemory(&overlapped, sizeof(overlapped));
		}

		HANDLE handle;
		boost::filesystem::path dirname;
		bool watch_subdirs;
		boost::weak_ptr<DirMonitorImplementation> impl;
		char buffer[1024];
		OVERLAPPED overlapped;
	};

	static boost::asio::io_service::id id;

	explicit basic_dir_monitor_service(boost::asio::io_service &io_service)
		: boost::asio::io_service::service(io_service),
		iocp_(init_iocp()),
		run_(true),
		async_monitor_work_(new boost::asio::io_service::work(async_monitor_io_service_)),
		async_monitor_thread_(boost::bind(&boost::asio::io_service::run, &async_monitor_io_service_))
	{
		work_thread_ = boost::thread(&boost::asio::basic_dir_monitor_service<DirMonitorImplementation>::work_thread, this);
	}

	~basic_dir_monitor_service()
	{
		// The async_monitor thread will finish when async_monitor_work_ is reset as all asynchronous
		// operations have been aborted and were discarded before (in destroy).
		async_monitor_work_.reset();

		// Event processing is stopped to discard queued operations.
		async_monitor_io_service_.stop();

		// The async_monitor thread is joined to make sure the directory monitor service is
		// destroyed _after_ the thread is finished (not that the thread tries to access
		// instance properties which don't exist anymore).
		async_monitor_thread_.join();

		// The work thread is stopped and joined, too.
		stop_work_thread();
		work_thread_.join();

		CloseHandle(iocp_);
	}

	typedef boost::shared_ptr<DirMonitorImplementation> implementation_type;

	void construct(implementation_type &impl)
	{
		impl = boost::make_shared<DirMonitorImplementation>();
	}

	void destroy(implementation_type &impl)
	{
		// If an asynchronous call is currently waiting for an event
		// we must interrupt the blocked call to make sure it returns.
		impl->destroy();

		impl.reset();
	}

	void add_directory(implementation_type &impl, boost::filesystem::path const& dirname, bool watch_subdirs)
	{
		if (!boost::filesystem::is_directory(dirname))
			throw std::invalid_argument("boost::asio::basic_dir_monitor_service::add_directory: " + dirname.string() + " is not a valid directory entry");

		HANDLE handle = CreateFileW(dirname.wstring().c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
		if (handle == INVALID_HANDLE_VALUE)
		{
			DWORD last_error = GetLastError();
			boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::add_directory: CreateFile failed");
			boost::throw_exception(e);
		}

		// No smart pointer can be used as the pointer must travel as a completion key
		// through the I/O completion port module.
		completion_key *ck = new completion_key(handle, dirname, watch_subdirs, impl);
		iocp_ = CreateIoCompletionPort(ck->handle, iocp_, reinterpret_cast<ULONG_PTR>(ck), 0);
		if (iocp_ == NULL)
		{
			delete ck;
			DWORD last_error = GetLastError();
			boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::add_directory: CreateIoCompletionPort failed");
			boost::throw_exception(e);
		}

		DWORD bytes_transferred; // ignored
		BOOL res = ReadDirectoryChangesW(ck->handle, ck->buffer, sizeof(ck->buffer), ck->watch_subdirs, 0x1FF, &bytes_transferred, &ck->overlapped, NULL);
		if (!res)
		{
			delete ck;
			DWORD last_error = GetLastError();
			boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::add_directory: ReadDirectoryChangesW failed");
			boost::throw_exception(e);
		}

		impl->add_directory(dirname, ck->handle);
	}

	void remove_directory(implementation_type &impl, boost::filesystem::path const& dirname)
	{
		// Removing the directory from the implementation will automatically close the associated file handle.
		// Closing the file handle again will make GetQueuedCompletionStatus() return where the completion key
		// is then deleted.
		impl->remove_directory(dirname);
	}

	path_list directories(implementation_type const& impl) const
	{
		return impl->directories();
	}

	dir_monitor_event monitor(implementation_type &impl, boost::system::error_code &ec)
	{
		return impl->popfront_event(ec);
	}

	template <typename Handler>
	class monitor_operation
	{
	public:
		monitor_operation(implementation_type &impl, boost::asio::io_service &io_service, Handler handler)
			: impl_(impl),
			io_service_(io_service),
			work_(io_service),
			handler_(handler)
		{
		}

		void operator()() const
		{
			implementation_type impl = impl_.lock();
			if (impl)
			{
				boost::system::error_code ec;
				dir_monitor_event ev = impl->popfront_event(ec);
				this->io_service_.post(boost::asio::detail::bind_handler(handler_, ec, ev));
			}
			else
			{
				this->io_service_.post(boost::asio::detail::bind_handler(handler_, boost::asio::error::operation_aborted, dir_monitor_event()));
			}
		}

	private:
		boost::weak_ptr<DirMonitorImplementation> impl_;
		boost::asio::io_service &io_service_;
		boost::asio::io_service::work work_;
		Handler handler_;
	};

	template <typename Handler>
	void async_monitor(implementation_type &impl, Handler handler)
	{
		this->async_monitor_io_service_.post(monitor_operation<Handler>(impl, this->get_io_service(), handler));
	}

private:
	void shutdown_service()
	{
		stop_work_thread();
	}

	HANDLE init_iocp()
	{
		HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (iocp == NULL)
		{
			DWORD last_error = GetLastError();
			boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::init_iocp: CreateIoCompletionPort failed");
			boost::throw_exception(e);
		}
		return iocp;
	}

	void work_thread()
	{
		while (run_)
		{
			DWORD bytes_transferred;
			completion_key *ck;
			OVERLAPPED *overlapped;
			BOOL res = GetQueuedCompletionStatus(iocp_, &bytes_transferred, reinterpret_cast<PULONG_PTR>(&ck), &overlapped, INFINITE);
			if (!res)
			{
				DWORD last_error = GetLastError();
				boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::work_thread: GetQueuedCompletionStatus failed");
				boost::throw_exception(e);
			}

			if (ck)
			{
				// If a file handle is closed GetQueuedCompletionStatus() returns and bytes_transferred will be set to 0.
				// The completion key must be deleted then as it won't be used anymore.
				if (!bytes_transferred)
					delete ck;
				else
				{
					// We must check if the implementation still exists. If the I/O object is destroyed while a directory event
					// is detected we have a race condition. Using a weak_ptr and a lock we make sure that we either grab a
					// shared_ptr first or - if the implementation has already been destroyed - don't do anything at all.
					implementation_type impl = ck->impl.lock();

					// If the implementation doesn't exist anymore we must delete the completion key as it won't be used anymore.
					if (!impl)
						delete ck;
					else
					{
						DWORD offset = 0;
						PFILE_NOTIFY_INFORMATION fni;
						do
						{
							fni = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(ck->buffer + offset);
							dir_monitor_event::event_type type = dir_monitor_event::null;
							switch (fni->Action)
							{
							case FILE_ACTION_ADDED: type = dir_monitor_event::added; break;
							case FILE_ACTION_REMOVED: type = dir_monitor_event::removed; break;
							case FILE_ACTION_MODIFIED: type = dir_monitor_event::modified; break;
							case FILE_ACTION_RENAMED_OLD_NAME: type = dir_monitor_event::renamed_old_name; break;
							case FILE_ACTION_RENAMED_NEW_NAME: type = dir_monitor_event::renamed_new_name; break;
							}
							std::wstring const filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
							impl->pushback_event(dir_monitor_event(ck->dirname / filename, type));
							offset += fni->NextEntryOffset;
						}
						while (fni->NextEntryOffset);

						ZeroMemory(&ck->overlapped, sizeof(ck->overlapped));
						BOOL res = ReadDirectoryChangesW(ck->handle, ck->buffer, sizeof(ck->buffer), ck->watch_subdirs, 0x1FF, &bytes_transferred, &ck->overlapped, NULL);
						if (!res)
						{
							delete ck;
							DWORD last_error = GetLastError();
							boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::work_thread: ReadDirectoryChangesW failed");
							boost::throw_exception(e);
						}
					}
				}
			}
		}
	}

	void stop_work_thread()
	{
		run_ = false;

		// By setting the third paramter to 0 GetQueuedCompletionStatus() will return with a null pointer as the completion key.
		// The work thread won't do anything except checking if it should continue to run. As run_ is set to false it will stop.
		BOOL res = PostQueuedCompletionStatus(iocp_, 0, 0, NULL);
		if (!res)
		{
			DWORD last_error = GetLastError();
			boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), "boost::asio::basic_dir_monitor_service::stop_work_thread: PostQueuedCompletionStatus failed");
			boost::throw_exception(e);
		}
	}

	HANDLE iocp_;
	boost::atomic<bool> run_;
	boost::thread work_thread_;
	boost::asio::io_service async_monitor_io_service_;
	boost::scoped_ptr<boost::asio::io_service::work> async_monitor_work_;
	boost::thread async_monitor_thread_;
};

template <typename DirMonitorImplementation>
boost::asio::io_service::id basic_dir_monitor_service<DirMonitorImplementation>::id;

}
}

#endif
