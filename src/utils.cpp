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
#include "jsx/utils.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#if OS(WINDOWS) && CPU(X86) && ENABLE(STACK_WALKER)
void aspect_show_stack(HANDLE hThread, CONTEXT &c);
#endif

#if OS(LINUX)
#include <pthread.h>
#endif

#if OS(MAC_OS_X)
#include <pthread.h>
#include <mach-o/dyld.h>
#endif

#if COMPILER(MSVC) && !defined(PRIu64)
#define PRIu64 "I64u"
#elif COMPILER(GCC) && !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif


namespace aspect { namespace utils {

std::vector<std::string>& split(std::string const& s, char delim, std::vector<std::string>& elems)
{
	boost::algorithm::split(elems, s, boost::algorithm::is_from_range(delim, delim));
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	return split(s, delim, elems);
}

// ----------------------------------------------------

// NOTE - the goal of this function is to establish
// a custom ID of the system and store it as a local
// file beside the executable so that the entire
// setup can survive migration between computer...

// ^^^ The above is no longer true. I ran into a problem
// across multiple replicated virtual machines that have
// the same HD serial number :(  Hence the new approach
// is to abandon the requirement for the application
// to be compatible with migration and establish a new
// global id accessible by all applications installed
// on the system.

std::string get_unique_local_system_identifier()
{
	using namespace boost::uuids;

	boost::filesystem::path id_file_path;
#if OS(WINDOWS)
//	id_file_path = root_path();
	char sys_dir[MAX_PATH + 1];
	::GetSystemWindowsDirectoryA(sys_dir, MAX_PATH);
	id_file_path = sys_dir;
#else
	id_file_path = "/usr/local/etc/harmony";
#endif
	id_file_path /= "id.local";

	std::string id;

	// try to read ID
	{
		boost::filesystem::ifstream id_file(id_file_path);
		id_file >> id;
		if ( !id.empty() )
		{
			return id;
		}
	}

	// generate an ID

	uuid u = random_generator()();
	id = to_string(u);

/*

We are now generating a global system random uuid

#if OS(WINDOWS)
	char sys_dir[MAX_PATH + 1];
	if ( ::GetSystemWindowsDirectoryA(sys_dir, MAX_PATH) >= 3 )
	{
		sys_dir[3] = 0;
	}

	DWORD ui = 0;
	::GetVolumeInformationA(sys_dir, NULL, NULL, &ui, NULL, NULL, NULL, NULL);
	if ( ui )
	{
		id = hex_str(&ui, sizeof(ui));
	}
#elif OS(UNIX)
	long const ui = gethostid();
	id = hex_str(&ui, sizeof(ui));
#else
	#error Not implemented yet
#endif
*/

	// save that new ID
	if ( !id.empty() )
	{
		boost::filesystem::ofstream id_file(id_file_path, std::ios::trunc);
		id_file << id;
	}
	return id;
}

std::string get_unique_local_system_uuid()
{
	using namespace boost::uuids;

	string_generator plain_gen;
	uuid const plain_uuid = plain_gen("{00000000-0000-0000-0000-000000000000}");

	name_generator name_gen(plain_uuid);
	uuid const uuid_system = name_gen(utils::get_unique_local_system_identifier());
	return to_string(uuid_system);
}

#if OS(WINDOWS)

std::string wstring_to_string(wchar_t const* src, size_t len)
{
	if ( len == (size_t)-1 )
	{
		len = wcslen(src);
	}

	if ( len )
	{
		static size_t const INITIAL_LEN = 256;
		std::string result(INITIAL_LEN, 0);
		result.resize(::WideCharToMultiByte(CP_UTF8, 0, src, static_cast<int>(len),
			&result[0], static_cast<int>(result.size()), NULL, NULL));
		if ( result.size() >= INITIAL_LEN )
		{
			::WideCharToMultiByte(CP_UTF8, 0, src, static_cast<int>(len), &result[0],
				static_cast<int>(result.size()), NULL, NULL);
		}
		return result;
	}
	else
	{
		return std::string();
	}
}

std::wstring string_to_wstring(char const* src, size_t len)
{
	if ( len == (size_t)-1 )
	{
		len = strlen(src);
	}

	if ( len )
	{
		static size_t const INITIAL_LEN = 256;
		std::wstring result(INITIAL_LEN, 0);
		result.resize(::MultiByteToWideChar(CP_UTF8, 0, src, static_cast<int>(len),
			&result[0], static_cast<int>(result.size())));
		if ( result.size() >= INITIAL_LEN )
		{
			::MultiByteToWideChar(CP_UTF8, 0, src, static_cast<int>(len), &result[0], static_cast<int>(result.size()));
		}
		return result;
	}
	else
	{
		return std::wstring();
	}
}

#else

std::string wstring_to_string(wchar_t const* src, size_t len)
{
	(void)len; // ingore

	static size_t const INITIAL_LEN = 256;
	std::string result(INITIAL_LEN, 0);
	size_t new_len = std::wcstombs(&result[0], src, result.size());
	if ( new_len == (size_t)-1 )
	{
		result.clear();
	}
	else
	{
		result.resize(new_len);
		if ( result.size() >= INITIAL_LEN )
		{
			std::wcstombs(&result[0], src, result.size());
		}
	}
	return result;
}

std::wstring string_to_wstring(char const* src, size_t len)
{
	(void)len; // ingore

	static size_t const INITIAL_LEN = 256;
	std::wstring result(INITIAL_LEN, 0);
	size_t new_len = std::mbstowcs(&result[0], src, result.size());
	if ( new_len == (size_t)-1 )
	{
		result.clear();
	}
	else
	{
		result.resize(new_len);
		if ( result.size() >= INITIAL_LEN )
		{
			std::mbstowcs(&result[0], src, result.size());
		}
	}
	return result;
}

#endif

std::string to_lower(std::string const& src)
{
	std::string dst(src.size(), 0);
	std::transform(src.begin(), src.end(), dst.begin(), ::tolower);
	return dst;
}

std::string hex_str(void const* data, size_t size)
{
	if ( size == 0 )
	{
		return "";
	}
	assert(data);

	std::string result(size * 2, 0);

	uint8_t const* src = reinterpret_cast<uint8_t const*>(data);
	for (size_t i = 0; i < size; ++i)
	{
		sprintf(&result[i * 2], "%02x", src[i]);
	}
	return result;
}

inline static char from_hex_digit(char digit)
{
	if ('0' <= digit && digit <= '9') return digit - '0';
	if ('A' <= digit && digit <= 'F') return digit - 'A';
	if ('a' <= digit && digit <= 'f') return digit - 'a';

	throw std::invalid_argument(std::string("invalid hex digit ") + digit);
}

void from_hex_str(std::vector<char>& result, void const* data, size_t size)
{
	result.clear();
	result.reserve(size / 2 + (size & 1));

	char const* str = reinterpret_cast<char const*>(data);
	for (size_t i = 0, count = size / 2; i < count; ++i)
	{
		char const b = (from_hex_digit(str[i * 2]) << 4) | from_hex_digit(str[i * 2 + 1]);
		result.push_back(b);
	}
	if (size & 1)
	{
		result.push_back(from_hex_digit(str[size - 1]));
	}
}

template<typename RandomAccessIterator, typename OutputIterator>
void encode_base64(RandomAccessIterator begin, RandomAccessIterator end, OutputIterator dest)
{
	using namespace boost::archive::iterators;

	typedef base64_from_binary<transform_width<RandomAccessIterator, 6, 8>> encoder;

	size_t const size = end - begin;
	size_t const pad_count = ((3 - size) % 3) % 3;

	encoder enc_begin(begin), enc_end(end);
	dest = std::copy(enc_begin, enc_end, dest);
	std::fill_n(dest, pad_count, '=');
}

template<typename BidirectionalIterator, typename OutputIterator>
void decode_base64(BidirectionalIterator begin, BidirectionalIterator end, OutputIterator dest)
{
	using namespace boost::archive::iterators;

	typedef transform_width<binary_from_base64< remove_whitespace<BidirectionalIterator> >, 8, 6> decoder;

	std::reverse_iterator<BidirectionalIterator> rbegin(end), rend(begin);
	end = std::find_if(rbegin, rend, [](char ch) { return ch != '='; }).base();

	decoder dec_begin(begin), dec_end(end);
	std::copy(dec_begin, dec_end, dest);
}

void encode_base64(std::vector<char>& result, void const* data, size_t size)
{
	result.clear();
	result.reserve(size * 8 / 6 + 4);

	typedef char const* const_iterator;
	const_iterator begin = const_iterator(data), end = begin + size;

	encode_base64(begin, end, std::back_inserter(result));
}

std::string encode_base64(void const* data, size_t size)
{
	std::string result;
	result.reserve(size * 8 / 6 + 4);

	typedef char const* const_iterator;
	const_iterator begin = const_iterator(data), end = begin + size;

	encode_base64(begin, end, std::back_inserter(result));
	return result;
}

void decode_base64(std::vector<char>& result, void const* data, size_t size)
{
	result.clear();
	result.reserve(size / 8 * 6 + 1);

	typedef char const* const_iterator;

	const_iterator begin = const_iterator(data), end = begin + size;
	decode_base64(begin, end, std::back_inserter(result));
}

std::string decode_base64(void const* data, size_t size)
{
	std::string result;
	result.reserve(size / 8 * 6 + 1);

	typedef char const* const_iterator;

	const_iterator begin = const_iterator(data), end = begin + size;
	decode_base64(begin, end, std::back_inserter(result));

	return result;
}

}} // aspect::utils
