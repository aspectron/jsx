//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_CPU_HPP_INCLUDED
#define JSX_CPU_HPP_INCLUDED

#include "jsx/api.hpp"

namespace aspect {

/// Return number of CPU cores
CORE_API unsigned cpu_count();

/// Return CPU type
CORE_API std::string cpu_type();

/**
 * Calls cpuid with op and store results of eax,ebx,ecx,edx
 * \param op cpuid function (eax input)
 * \param eax content of eax after the call to cpuid
 * \param ebx content of ebx after the call to cpuid
 * \param ecx content of ecx after the call to cpuid
 * \param edx content of edx after the call to cpuid
 */
CORE_API void cpuid(uint32_t op, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);

CORE_API bool is_SSE2_supported();

} // ::aspect

#endif // JSX_CPU_HPP_INCLUDED
