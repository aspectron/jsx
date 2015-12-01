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
#include "jsx/v8_main_loop.hpp"

#include "jsx/runtime.hpp"

namespace aspect { namespace v8_core {

void main_loop::check_global_termination()
{
// TODO - PAVEL - turn this into named conditions
#if 0 // OS(WINDOWS)
		HANDLE termination_event = runtime::get_termination_event();
		if(termination_event)
		{
			DWORD dw = WaitForSingleObject(termination_event,0);
			if(dw == WAIT_OBJECT_0 && !runtime::is_terminating())
				runtime::terminate(0);
		}
#endif
}

void main_loop::idle_notification()
{
	if (rt_.execution_options() & runtime::GC_NOTIFY)
	{
#if ENABLE(MEASURE_V8_IDLE_DURATION)
			double const start = utils::get_ts();
#endif
			rt_.isolate()->IdleNotification(1);
#if ENABLE(MEASURE_V8_IDLE_DURATION)
			double const duration = utils::get_ts() - start;
			if ( duration > V8_IDLE_ALERT_THRESHOLD )
			{
				rt_.trace("-- idle '%s'  duration: %1.2f\n", rt_.script().c_str(), duration);
			}
#endif
	}
}

void main_loop::heap_statistics()
{
#if ENABLE(V8_HEAP_STATISTICS)
	double const ts_heap = utils::get_ts();
	if ( ts_heap - last_ts_heap_ > (V8_HEAP_STATISTICS_INTERVAL*1000.0) )
	{
		last_ts_heap_ = ts_heap;

		v8::HeapStatistics hs;
		rt_.isolate()->GetHeapStatistics(&hs);
		rt_.trace("-- heap '%s'  total: %d  total executable: %d  used: %d  limit: %d\n",
			rt_.script().string().c_str(),
			hs.total_heap_size(),
			hs.total_heap_size_executable(),
			hs.used_heap_size(),
			hs.heap_size_limit());
	}
#endif
}

main_loop::main_loop(runtime const& rt)
	: rt_(rt)
	, max_callbacks_(MIN_CALLBACKS)
	, last_ts_heap_(0)
{
}

void main_loop::execute_callbacks()
{
	try
	{
		size_t handled = io_service_.run_one();
		for (size_t num_cb = handled, max_cb = max_callbacks_; handled && num_cb < max_cb; num_cb += handled)
		{
			handled = io_service_.poll_one();
		}
	}
	catch (std::exception const& ex)
	{
		rt_.trace("exception in V8 callback: %s\n", ex.what());
	}
}

void main_loop::run()
{
	os::set_thread_name("v8 main");

	if (rt_.execution_options() & runtime::EVENT_QUEUE)
	{
		io_service_work_.reset(new boost::asio::io_service::work(io_service_));
	}

	while (!io_service_.stopped())
	{
		execute_callbacks();

		check_global_termination();

		idle_notification();
		heap_statistics();
	}
}

}} // aspect::v8_core
