//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CORE_HPP_INCLUDED
#define JSX_CORE_HPP_INCLUDED

#include "jsx/platform.hpp"
#include "jsx/config.hpp"
#include "jsx/api.hpp"

#include <algorithm>
using std::min; using std::max;

#if ENABLE(MSVC_MEMORY_LEAK_DETECTION) && OS(WINDOWS) && TARGET(DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#if OS(WINDOWS)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// -- global includes
#include <stdlib.h>
#include <signal.h>

#include <vector>
#include <string>

#include <set>
#include <map>
#include <queue>
#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#pragma warning(push)
#pragma warning(disable: 4100 4127)
#include <v8.h>
#pragma warning(pop)

#endif // JSX_CORE_HPP_INCLUDED
