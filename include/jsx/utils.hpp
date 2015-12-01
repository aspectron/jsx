//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_UTILS_HPP_INCLUDED
#define JSX_UTILS_HPP_INCLUDED

#include <boost/filesystem/path.hpp>

namespace aspect { namespace utils {

#if OS(WINDOWS)
	#if TARGET(DEBUG)
		#include "crtdbg.h"
		#define _aspect_assert(expr)  { bool _asrt_expr = (expr) ? true : false; if (!(_asrt_expr) && (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, "%s\n\nFunction: %s\n",#expr,__FUNCTION__))) _CrtDbgBreak(); }
	#else
		#define _aspect_assert(expr) assert(expr)
	#endif
#else
	#include <assert.h>
	#define _aspect_assert(expr) {  assert(expr); if(!(expr)) printf("--- assertion failure: %s\n",#expr); }
#endif

#if COMPILER(MSVC)

#define strcasecmp     _stricmp
#define strncasecmp    _strnicmp
#define snprintf       _snprintf
#define vsnprintf      _vsnprintf

#endif


CORE_API std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
CORE_API std::vector<std::string> split(const std::string &s, char delim);

CORE_API std::string get_unique_local_system_identifier();
CORE_API std::string get_unique_local_system_uuid();

// timestamp in millseconds
inline double get_ts()
{
	using namespace boost::chrono;
	typedef high_resolution_clock clock;

	clock::duration const since_epoch = clock::now().time_since_epoch();
	return duration_cast<duration<double>>(since_epoch).count() * 1000.0;
}

CORE_API std::string wstring_to_string(wchar_t const* src, size_t len = (size_t)-1);
inline CORE_API std::string wstring_to_string(std::wstring const& src)
{
	return wstring_to_string(src.data(), src.size());
}

CORE_API std::wstring string_to_wstring(char const* src, size_t len = (size_t)-1);
inline CORE_API std::wstring string_to_wstring(std::string const& src)
{
	return string_to_wstring(src.data(), src.size());
}

/// Convert string to lower case
CORE_API std::string to_lower(std::string const& src);

/// Convert buffer data[size] into hex string
CORE_API std::string hex_str(void const* data, size_t size);

/// Convert hex string to byte buffer data
CORE_API void from_hex_str(std::vector<char>& result, void const* data, size_t size);

/// Encode buffer data[size] to Base64
CORE_API void encode_base64(std::vector<char>& result, void const* data, size_t size);
CORE_API std::string encode_base64(void const* data, size_t size);

/// Decode buffer data[size] from Base64
CORE_API void decode_base64(std::vector<char>& result, void const* data, size_t size);
CORE_API std::string decode_base64(void const* data, size_t size);

/// Convert input sequence [begin, end) from UCS-4 to UTF-8 output iterator out
template<class InIt, class OutIt>
inline OutIt to_utf8(InIt const begin, InIt const end, OutIt out)
{
	for (InIt in = begin; in != end;)
	{
		uint32_t wc = static_cast<uint32_t>(*in); ++in;
	over:
		if (wc < 0x80)
		{
			*out = static_cast<uint8_t>(wc); ++out;
			continue;
		}

		if (sizeof(wchar_t) == 2 && wc >= 0xD800 && wc < 0xE000)
		{
			// handle surrogates for UTF-16
			if (wc >= 0xDC00) { wc = '?'; goto over; }
			if( in == end ) break;
			uint32_t const lo = static_cast<uint32_t>(*in); ++in;
			if (lo >= 0xDC00 && wc < 0xE000)
			{
				wc  = 0x10000 + ((wc & 0x3FF) << 10 | (lo & 0x3FF));
			}
			else
			{
				*out = '?'; ++out; wc = lo;
				goto over;
			}
		}

		int shift;
		if      (wc < 0x800)     { shift = 6;  }
		else if (wc < 0x10000)   { shift = 12; }
		else if (wc < 0x200000)  { shift = 18; }
		else if (wc < 0x4000000) { shift = 24; }
		else                     { shift = 30; }

		uint8_t c = 0xFF << (7 - shift / 6);
		do
		{
			c |= (wc >> shift) & 0x3f;
			*out = c; ++out;
			c = 0x80; shift -= 6;
		} while(shift >= 0);
	}
	return out;
}

/// Convert input sequence [begin, end) from UTF-8 to UCS-4 output iterator out
template<class InIt, class OutIt>
inline OutIt from_utf8(InIt const begin, InIt const end, OutIt out)
{
	for(InIt in = begin; in != end; ++in)
	{
		uint32_t  wc = static_cast<uint8_t>(*in);
	over:
		if (wc & 0x80)
		{
			int cnt;

			if      ((wc & 0xE0) == 0xC0) { cnt = 1; wc &= ~0xE0; }
			else if ((wc & 0xF0) == 0xE0) { cnt = 2; wc &= ~0xF0; }
			else if ((wc & 0xF8) == 0xF0) { cnt = 3; wc &= ~0xF8; }
			else if ((wc & 0xFC) == 0xF8) { cnt = 4; wc &= ~0xFC; }
			else if ((wc & 0xFE) == 0xFC) { cnt = 5; wc &= ~0xFE; }
			else
			{
				// invalid start code
				*out = wchar_t('?'); ++out;
				continue;
			}

			if (wc == 0) wc = ~0u; //codepoint encoded with overlong sequence

			do
			{
				if (++in == end) return out;
				uint8_t const c = static_cast<uint8_t>(*in);
				if ((c & 0xC0) != 0x80)
				{
					*out = static_cast<wchar_t>(wc); ++out;
					wc = c;
					goto over;
				}
				wc <<= 6; wc |= c & ~0xC0;
			} while (--cnt);

			if (wc & 0x80000000) wc = '?'; // codepoint exceeds unicode range
			if (sizeof(wchar_t) == 2 && wc > 0xFFFF)
			{
				//handle surrogates for UTF-16
				wc -= 0x10000;
				*out = static_cast<wchar_t>(0xD800 | ((wc >> 10) & 0x3FF)); ++out;
				*out = static_cast<wchar_t>(0xDC00 | (wc & 0x3FF)); ++out;
				continue;
			}
		}
		*out = static_cast<wchar_t>(wc); ++out;
	}
	return out;
}

}} // ::aspect::utils

#endif // JSX_UTILS_HPP_INCLUDED
