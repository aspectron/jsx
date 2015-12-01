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
#include "jsx/cpus.hpp"

#if OS(UNIX)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include "jsx/os.hpp"

namespace aspect {

unsigned cpu_count()
{
	static unsigned const result = max(1U, boost::thread::hardware_concurrency());
	return result;
}

std::string cpu_type()
{
	std::string result;

#if OS(WINDOWS)
	char const* const key_name = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
	char const* const value_name = "ProcessorNameString";
	result = os::read_reg_value(HKEY_LOCAL_MACHINE, key_name, value_name);
#elif OS(LINUX)
	std::ifstream cpuinfo("/proc/cpuinfo");
	for (std::string s; cpuinfo;)
	{
		std::getline(cpuinfo, s);

		if (boost::algorithm::starts_with(s, "model name"))
		{
			result = s.substr(s.find(':') + 1);
			boost::algorithm::trim(result);
			break;
		}
	}
#elif OS(DARWIN)
	result.resize(2048);
	size_t size = result.size();
	if ( sysctlbyname("machdep.cpu.brand_string", &result[0], &size, nullptr, 0) == -1)
	{
		result.resize(size + 1);
		sysctlbyname("machdep.cpu.brand_string", &result[0], &size, nullptr, 0);
	}
	result.resize(size);
	result = result.c_str();
	boost::algorithm::trim(result);
#elif OS(FREEBSD)
	int mib[2];
	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_;
	result.resize(2048);
	size_t size = result.size();
	if (sysctl(mib, 2, &result[0], &size, NULL, 0) == -1)
	{
		result.resize(size + 1);
		sysctl(mib, 2, &result[0], &size, NULL, 0);
	}
	result.resize(size);
	result = result.c_str();
	result = result.substr(result.find(':') + 1);
	boost::algorithm::trim(result);
#else
#error "Unsupported OS"
#endif

	return result;
}

void cpuid(uint32_t op, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
{
	eax = ebx = ecx = edx = 0;
#if COMPILER(GCC)

#if CPU(X86)
	// GCC won't allow us to clobber EBX since its used to store the GOT. So we need to
	// lie to GCC and backup/restore EBX without declaring it as clobbered.
	asm volatile("pushl %%ebx   \n\t"
		"cpuid         \n\t"
		"movl %%ebx, %1\n\t"
		"popl %%ebx    \n\t"
		: "=a"(eax), "=r"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(op)
		: "cc");
#elif CPU(X64)
	asm volatile("cpuid         \n\t"
		: "=a"(eax), "=r"(ebx), "=c"(ecx), "=d"(edx)
		: "a"(op)
		: "cc" );
#else
#warning "TODO - IMPLEMENT CPUID!"
#endif

#elif COMPILER(MSVC)
	// MSVC provides a __cpuid function
	int regs[4];
	__cpuid( regs, op );
	eax = regs[0];
	ebx = regs[1];
	ecx = regs[2];
	edx = regs[3];
#else
#warning "TODO - IMPLEMENT CPUID!"
#endif
}

bool is_SSE2_supported()
{
	static bool checked = false;
	static bool support = false;

	if (!checked)
	{
		uint32_t eax, ebx, ecx, edx;

		cpuid(0, eax, ebx, ecx, edx);
		if ( eax >= 1 )
		{
			cpuid(1, eax, ebx, ecx, edx);
			support = (edx >> 26) & 1;
		}
		checked = true;
	}

	return support;
}

} // ::aspect
