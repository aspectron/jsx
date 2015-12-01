//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CONFIG_HPP_INCLUDED
#define JSX_CONFIG_HPP_INCLUDED

#define HARMONY_RTE_VERSION					"0.9.0"
#define CONTACT_EMAIL_ADDRESS				"anton.yemelyanov@gmail.com"
#define ENABLE_STANDALONE_SCRIPT_EXECUTION	1

#if TARGET(DEBUG)
#define ENABLE_MSVC_OUTPUT					1
#define FORCE_V8_DEBUGGER					0
#define ENABLE_ISOLATE_CONSOLE_WINDOWS		0
#define ENABLE_MSVC_MEMORY_LEAK_DETECTION	0
#else
#define FORCE_V8_DEBUGGER 0
#define ENABLE_WIN32_TOP_LEVEL_EXCEPTION_HANDLER	1
#endif

#define ENABLE_MEASURE_V8_IDLE_DURATION	0
#define V8_IDLE_ALERT_THRESHOLD			0.0
#define ENABLE_V8_HEAP_STATISTICS		0
#define V8_HEAP_STATISTICS_INTERVAL		10.0
#define ENABLE_REDIRECT_OUTPUT_TO_LOGS	0
#define ENABLE_DIRECTORY_MONITOR 1

#define MAX_REQUIRE_RECURSIONS 64

// TODO - check what is using this, probably v8
#define USING_V8_DEBUGGER 1


#endif // JSX_CONFIG_HPP_INCLUDED
