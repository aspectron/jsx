//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_PLATFORM_HPP_INCLUDED
#define JSX_PLATFORM_HPP_INCLUDED

// ----------------------

// MSVC GCC INTEL
#define COMPILER(_FEATURE)	(defined COMPILER_##_FEATURE && COMPILER_##_FEATURE)

// X86 X64 ARM
#define CPU(_FEATURE)		(defined CPU_##_FEATURE && CPU_##_FEATURE)

// WINDOWS LINUX DARWIN IOS MAX_OS_X ANDROID FREEBSD UNIX
#define OS(_FEATURE)		(defined OS_##_FEATURE && OS_##_FEATURE)

// DEBUG RELEASE
#define TARGET(_FEATURE)	(defined TARGET_##_FEATURE && TARGET_##_FEATURE)

// ---
#define HAVE(_FEATURE)		(defined HAVE_##_FEATURE && HAVE_##_FEATURE)

// ---
#define USING(_FEATURE)		(defined USING_##_FEATURE && USING_##_FEATURE)

// ---
#define FORCE(_FEATURE)		(defined FORCE_##_FEATURE && FORCE_##_FEATURE)

// ---
#define ENABLE(_FEATURE)		(defined ENABLE_##_FEATURE && ENABLE_##_FEATURE)

// ----------------------

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(__INTEL_COMPILER)
#define COMPILER_INTEL 1
#endif

// CPU(X86) - i386 / x86 32-bit
#if   defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL)
#define CPU_X86 1 // x86 32-bit
#define CURRENT_CPU_STRING "x86"
#elif defined(__x86_64__) || defined(_M_X64) || defined(_WIN64)
#define CPU_X64 1 // x86 64-bit
#define CURRENT_CPU_STRING "x64"
#elif defined(arm) || defined(__arm__) || defined(ARM) || defined(_ARM_)
#define CPU_ARM 1
#define CURRENT_CPU_STRING "ARM"
#endif

#if defined(_WIN32) || defined(WIN32)
#define OS_WINDOWS 1
#define _OS_NAME "windows"
#elif defined(__linux__)
#define OS_LINUX 1
#define _OS_NAME "linux"
#elif defined(__APPLE__)
#define OS_DARWIN 1
	#if ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)	|| (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
	#define OS_IOS 1
	#define _OS_NAME "ios"
//	#elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
	#else
	#define OS_MAC_OS_X 1
	#define _OS_NAME "osx"
	#endif
#elif defined(ANDROID)
#define OS_ANDROID 1
#define _OS_NAME "android"
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#define _OS_NAME "freebsd"
#endif

#if OS(LINUX) || OS(FREEBSD) || OS(DARWIN)
#define OS_UNIX 1
#endif

#if defined(_DEBUG)
#define TARGET_DEBUG 1
#else
#define TARGET_RELEASE 1
#ifndef NDEBUG
#define NDEBUG // for gcc compatibility
#endif
#endif

#if !COMPILER(MSVC)
#define __forceinline inline
#endif

//#if __cplusplus > 199711L
#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ >= 5))
# define HACK_GCC_ITS_CPP0X 1
#endif

#if OS(DARWIN)
//#if OS(LINUX) //defined(nullptr_t) || (__cplusplus > 199711L) || defined(HACK_GCC_ITS_CPP0X)
	#define HAVE_CPP0X 0
#else
	#define HAVE_CPP0X 1
#endif

#endif // JSX_PLATFORM_HPP_INCLUDED
