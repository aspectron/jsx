//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_PROCESS_HPP_INCLUDED
#define JSX_PROCESS_HPP_INCLUDED

#include "jsx/v8_core.hpp"

namespace aspect {

/// Process management subsystem
namespace process {

/// Process priority
enum priority_t
{
	PRIORITY_IDLE,
	PRIORITY_BELOW_NORMAL,
	PRIORITY_NORMAL,
	PRIORITY_ABOVE_NORMAL,
	PRIORITY_HIGH,
	PRIORITY_REALTIME,
	PRIORITY_CLASS_COUNT
};

#if OS(WINDOWS)
typedef uint32_t pid_t;
#endif

/// Start a process, return process id
pid_t start(std::string const& command_line, int priority = PRIORITY_NORMAL);

/// Kill a process
void kill(pid_t pid);

/// Current process id
pid_t current();

/// Is process running
bool is_running(pid_t pid);

/// Process identifiers list
typedef std::vector<pid_t> pid_list;

/// List of running processes
pid_list list();

/// Get process information as JS object
void get_info(v8::FunctionCallbackInfo<v8::Value> const& args);

/// Set process priority class
void set_priority(pid_t pid, int priority);

/// Obtain object containing CPU usage information
/// If PID is supplied, process information is obtained
/// If PID is not supplied, system cpu usage is obtained
void get_cpu_usage(v8::FunctionCallbackInfo<v8::Value> const& args);

// TODO - make this platform independent using named_conditions
// http://www.boost.org/doc/libs/1_36_0/doc/html/boost/interprocess/named_condition.html
// REFER TO runtime.cpp line 94
/// returns true if successful, false if unsuccessful
bool signal_named_condition(std::string const& condition_name);

void setup_bindings(v8pp::module& target);

}} // aspect::process

#endif // JSX_PROCESS_HPP_INCLUDED
