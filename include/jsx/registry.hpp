//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_REGISTRY_HPP_INCLUDED
#define JSX_REGISTRY_HPP_INCLUDED

#include "jsx/v8_core.hpp"

namespace aspect {

class registry : boost::noncopyable
{
public:
	static void setup_bindings(v8pp::module& target);

	explicit registry(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// The Root key is opened by constructor, closed by destructor.
	/// All paths are relative to the root key specified in the constructor.

	registry(HKEY root, std::string const& base_path);
	~registry();

	/// Read registry value; return value type will be String for REG_SZ and UInt32 for REG_DWORD
	void read(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Write registry value; accepted types are String -> REG_SZ and UInt32 -> REG_DWORD
	void write(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Recursively delete a key
	bool erase_branch(std::string const& path);

	/// Delete value
	bool erase_value(std::string const& path);

	/// Returns true if subkey index is valid
	bool enum_children(std::string& str, std::string const& path, unsigned index);
	void enum_children_v8(v8::FunctionCallbackInfo<v8::Value> const& args);

	// return true and name of the value if index is valid
	bool enum_values(std::string& str, std::string const& path, unsigned index);
	void enum_values_v8(v8::FunctionCallbackInfo<v8::Value> const& args);

	/// Advanced Registry access (via HKEYs)
	HKEY open_key(std::string const& path, bool create = true);
	void close_key(HKEY key);

private:
	void open_root(HKEY root, std::string const& base_path);

	char* parse_path(char** pszPath);
	int generic_read(void*& bBuff, int iBuffSize, DWORD dwType, std::string const& path);
	void generic_write(void* pBuff, int iBuffSize, DWORD dwType, std::string const& path);


	HKEY        hkey_;
	std::string root_path_;     // Used for error reporting
	std::string root_sub_path_;
};

} // ::aspect

#endif // JSX_REGISTRY_HPP_INCLUDED
