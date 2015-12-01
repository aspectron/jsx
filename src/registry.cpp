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
#include "jsx/registry.hpp"

namespace aspect {

#define HKEY_STR(root_name) std::make_pair(#root_name, root_name)
static std::pair<char const*, HKEY> const ROOT_KEYS[] =
{
	HKEY_STR(HKEY_CLASSES_ROOT),
	HKEY_STR(HKEY_CURRENT_CONFIG),
	HKEY_STR(HKEY_CURRENT_USER),
	HKEY_STR(HKEY_LOCAL_MACHINE),
	HKEY_STR(HKEY_USERS),
	HKEY_STR(HKEY_PERFORMANCE_DATA),
	HKEY_STR(HKEY_DYN_DATA),
};
#undef HKEY_STR
static size_t const ROOT_KEYS_COUNT = sizeof(ROOT_KEYS) / sizeof(*ROOT_KEYS);

static void throw_error(LRESULT lres, std::string const& what)
{
	if (lres != ERROR_SUCCESS)
	{
		boost::system::error_code const ec(static_cast<int>(lres), boost::system::get_system_category());
		throw boost::system::system_error(ec, what);
	}
}

static char const* key_to_string(HKEY key)
{
	for (size_t i = 0; i != ROOT_KEYS_COUNT; ++i)
	{
		if ( ROOT_KEYS[i].second == key )
		{
			return ROOT_KEYS[i].first;
		}
	}
	return NULL;
}

static HKEY string_to_key(std::string const& str)
{
	for (size_t i = 0; i != ROOT_KEYS_COUNT; ++i)
	{
		if ( ROOT_KEYS[i].first == str )
		{
			return ROOT_KEYS[i].second;
		}
	}
	return NULL;
}

void registry::setup_bindings(v8pp::module& target)
{
	/**
	@module registry Registry
	**/
	v8pp::module registry_module(target.isolate());

	/**
	@class Registry
	Registry class, Windows only

	@function Registry(root, path)
	@param root - One of the root keys, see #roots
	@param path {String} key path
	Open a registry key at `root\path`
	**/
	v8pp::class_<registry> registry_class(target.isolate(), v8pp::v8_args_ctor);
	registry_class
		/**
		@function eraseBranch(path)
		@param path {String}
		Recursively delete a key branch at `path`
		**/
		.set("eraseBranch", &registry::erase_branch)

		/**
		@function eraseValue(path)
		@param path {String}
		Recursively delete a value at `path`
		**/
		.set("eraseValue", &registry::erase_value)

		/**
		@function read(path)
		@param path {String}
		@return {Number|String}
		Read a value at `path`
		**/
		.set("read", &registry::read)

		/**
		@function write(path, value)
		@param path {String}
		@param value {Number|String}
		Write a `value` at `path`
		**/
		.set("write", &registry::write)

		/**
		@function enumChildren(path, index)
		@param path {String}
		@param index {Number}
		@return {String}
		Enumerate chidren keys at `path`
		**/
		.set("enumChildren", &registry::enum_children_v8)

		/**
		@function enumValues(path, index)
		@param path {String}
		@param index {Number}
		@return {String}
		Enumerate chidren values at `path`
		**/
		.set("enumValues", &registry::enum_values_v8)
		;

	v8pp::module roots(target.isolate());
	for (size_t i = 0; i != ROOT_KEYS_COUNT; ++i)
	{
		roots.set_const(ROOT_KEYS[i].first, ROOT_KEYS[i].second);
	}

	registry_module
		/**
		@module registry
		@property roots {Array}
		Root keys. One of the following values:
		  * `HKEY_CLASSES_ROOT`
		  * `HKEY_CURRENT_CONFIG`
		  * `HKEY_CURRENT_USER`
		  * `HKEY_LOCAL_MACHINE`
		  * `HKEY_USERS`
		  * `HKEY_PERFORMANCE_DATA`
		  * `HKEY_DYN_DATA`
		**/
		.set("roots", roots)
		.set("Registry", registry_class);
	target.set("registry", registry_module.new_instance());
}

aspect::registry::registry(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if ( args.Length() < 1 || !args[0]->IsString() )
	{
		throw std::runtime_error("registry() constructor requires at least one argument (root key constant)");
	}

	HKEY const hkey_root = string_to_key(v8pp::from_v8<std::string>(isolate, args[0]));
	if ( !hkey_root )
	{
		throw std::runtime_error("registry() constructor - invalid first argument not an HKEY ROOT string constant");
	}

	if ( args.Length() > 1 && !args[1]->IsString() && !args[1]->IsUndefined() && !args[1]->IsNull())
	{
		throw std::runtime_error("registry() constructor - second argument must be a string (base path)");
	}

	std::string base_path;
	if ( args.Length() > 1 && args[1]->IsString() )
	{
		base_path = v8pp::from_v8<std::string>(isolate, args[1]);
	}

	open_root(hkey_root, base_path);
}

registry::registry(HKEY root, std::string const& base_path)
{
	open_root(root, base_path);
}

registry::~registry()
{
	close_key(hkey_);
}

void registry::open_root(HKEY root, std::string const& root_path)
{
	char const* root_str = key_to_string(root);
	root_path_ = (root_str? root_str : "...");

	hkey_ = root;
	if ( !root_path.empty() )
	{
		hkey_ = open_key(root_path, true);
		root_path_ += "\\";
		root_path_ += root_path;
	}
}

void registry::close_key(HKEY key)
{
	if ( key != NULL )
	{
		::RegCloseKey(key);
	}
}

// Note: pszPath gets modified (the pointer and the contents!)
char* registry::parse_path(char** pszPath)
{
	char* szPath = *pszPath;

	// Chop leading delimiters and whitespace
	while(*szPath && (*szPath == '\\' || isspace(*szPath))) ++szPath;

	// Chop trailing delimiters and whitespace
	char* sz = szPath + strlen(szPath) - 1;
	while (sz >= szPath && (*sz == '\\' || isspace(*sz))) *sz-- = '\0';

	// Locate last occurrence of '\'
	while (sz > szPath && *sz != '\\') --sz;
	char* szFile = sz;
	if (*szFile == '\\') *szFile++ = '\0';

	if (szFile <= szPath)
		szPath = szFile + strlen(szFile);	// Empty path
	else
	{
		// Chop trailing delimiters and whitespace from path
		sz = szPath + strlen(szPath) - 1;
		while (sz >= szPath && (*sz == '\\' || isspace(*sz))) *sz-- = '\0';
	}
	*pszPath = szPath;
	return szFile;
}

HKEY registry::open_key(std::string const& path, bool create)
{
	REGSAM const REG_OPEN_ADDITIONAL_FLAGS = KEY_WOW64_64KEY;

	if ( hkey_ == NULL )
	{
		return NULL;
	}

	HKEY hkey = NULL;

	// Try to open it.
	LONG lResult = ::RegOpenKeyExA(hkey_, path.c_str(), 0, KEY_ALL_ACCESS | REG_OPEN_ADDITIONAL_FLAGS, &hkey);

	// If we couldn't open it, try to open the parent and create the child
	if ( lResult != ERROR_SUCCESS && create )
	{
		std::string strPath(path);
		char* szPath = (char* )strPath.c_str();
		char* szFile = parse_path(&szPath);

		HKEY hkey_parent = *szPath ? open_key(szPath, true) : hkey_;
		if ( hkey_parent )
		{
			// Try to create the child key
			DWORD dwDisposition;
			lResult = ::RegCreateKeyExA(hkey_parent,				// key
				szFile,					// subkey
				0,						// reserved
				NULL,					// class
				REG_OPTION_NON_VOLATILE,	// options
				KEY_ALL_ACCESS | REG_OPEN_ADDITIONAL_FLAGS,			// SAM desired
				NULL,					// lpSecurityAttributes
				&hkey, &dwDisposition);

			if (lResult != ERROR_SUCCESS) {
				hkey = NULL;	// make sure
				throw_error(lResult, "Unable to create registry key " + path);
			}
		}

		if ( hkey_parent != hkey_ )
		{
			close_key(hkey_parent);
		}
	}
	else
	if ( lResult != ERROR_SUCCESS )
	{
		throw_error(lResult, "Unable to open registry key " + path);
	}
	return hkey;
}

// Recursively delete a key branch
bool registry::erase_branch(std::string const& path)
{
	if ( hkey_ == NULL )
	{
		return false;
	}

	bool bOK = true;

	// Delete the subkeys
	std::string str;
	while ( bOK && enum_children(str, path, 0) )
	{
		std::string subkey;
		subkey = path + "\\" + str;
		bOK = erase_branch(subkey);
	}

	// Delete the key
	if ( bOK )
	{
		LONG lResult = ::RegDeleteKeyA(hkey_, path.c_str());
		if (lResult != ERROR_SUCCESS)
		{
			bOK = false;
			throw_error(lResult, "Unable to delete registry key " + path);
		}
	}
	return bOK;
}

bool registry::erase_value(std::string const& src_path)
{
	if ( hkey_ == NULL )
	{
		return false;
	}

	std::string path(src_path);
	char* szPath = (char*)path.c_str();
	char* szFile = parse_path(&szPath);

	HKEY hkey = open_key(szPath, false);

	// Delete the value
	LONG lResult = ::RegDeleteValueA(hkey, szFile);

	if ( hkey != hkey_ )
	{
		close_key(hkey);
	}

	if ( lResult != ERROR_SUCCESS )
	{
		throw_error(lResult, "Unable to delete registry key");
	}
	return true;
}

void registry::enum_children_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string const path = v8pp::from_v8<std::string>(isolate, args[0]);
	unsigned const index = v8pp::from_v8<unsigned>(isolate, args[1]);

	std::string child;
	if (enum_children(child, path, index))
	{
		args.GetReturnValue().Set(v8pp::to_v8(isolate, child));
	}
}

bool registry::enum_children(std::string& str, std::string const& path, unsigned index)
{
	bool result = false;
	HKEY hkey = open_key(path.c_str(), false);
	if ( hkey != NULL )
	{
		char szName[_MAX_PATH+1];
		DWORD dwNameLength = sizeof(szName);
		LONG lResult = ::RegEnumKeyExA(hkey, index, szName, &dwNameLength, NULL, NULL, NULL, NULL);
		if ( lResult == ERROR_SUCCESS )
		{
			str = szName;
			result = true;
		}
		if ( hkey != hkey_ )
		{
			close_key(hkey);
		}
	}
	return result;
}

// adaptor function for v8
void registry::enum_values_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string const path = v8pp::from_v8<std::string>(isolate, args[0]);
	unsigned const index = v8pp::from_v8<unsigned>(isolate, args[1]);

	std::string child;
	if (enum_values(child, path, index))
	{
		args.GetReturnValue().Set(v8pp::to_v8(isolate, child));
	}
}

bool registry::enum_values(std::string& str, std::string const& path, unsigned index)
{
	bool result = false;
	HKEY hkey = open_key(path, false);
	if ( hkey != NULL )
	{
		char szName[_MAX_PATH+1];
		DWORD dwNameLength = sizeof(szName);
		LONG lResult = ::RegEnumValueA(hkey, index, szName,&dwNameLength, NULL,NULL,NULL,NULL);
		if ( lResult == ERROR_SUCCESS )
		{
			str = szName;
			result = true;
		}

		if ( hkey != hkey_ )
		{
			close_key(hkey);
		}
	}
	return result;
}

void registry::read(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	std::string path = v8pp::from_v8<std::string>(isolate, args[0]);

	char* szPath = (char*)path.c_str();
	char* szFile = parse_path(&szPath);

	DWORD size = 0;
	std::vector<char> buf;
	DWORD type = REG_NONE;

	HKEY hkey = *szPath ? open_key(path, true) : hkey_;
	if ( hkey )
	{
		// Query the type and size
		LONG lResult = ::RegQueryValueExA(hkey, szFile, NULL, &type, NULL, &size);
		if ( lResult != ERROR_SUCCESS )
		{
			size = 0; // Not present (this is not an error)
		}
		// Allocate the buffer and read the value
		if ( size )
		{
			buf.resize(size);
			lResult = ::RegQueryValueExA(hkey, szFile, NULL, &type, (LPBYTE)&buf[0], &size);
			if ( lResult != ERROR_SUCCESS )
			{
				if ( hkey != hkey_ )
				{
					close_key(hkey);
				}
				throw_error(lResult, "Unable to read registry value " + path);
			}
		}
		if ( hkey != hkey_ )
		{
			close_key(hkey);
		}
	}

	if ( !buf.empty() )
	{
		switch ( type )
		{
		case REG_SZ:
		case REG_EXPAND_SZ:
			{
				std::string const str(buf.begin(), buf.end());
				args.GetReturnValue().Set(v8pp::to_v8(isolate, str));
			}
			break;
		case REG_DWORD:
			{
				uint32_t const val = *((uint32_t*)&buf[0]);
				args.GetReturnValue().Set(val);
			}
			break;
		default:
			{
				args.GetReturnValue().Set(v8pp::throw_ex(isolate, "Unsupported registry value type"));
			}
			break;
		}
	}
}

void registry::write(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	if (args.Length() != 2 )
	{
		throw std::invalid_argument("registry::write(path, value) requires two arguments");
	}

	if ( !args[0]->IsString() )
	{
		throw std::invalid_argument("registry::write(path, value) - first argument (path) must be a string");
	}

	if ( !(args[1]->IsString() || args[1]->IsUint32()) )
	{
		throw std::invalid_argument("registry::write(path, value) - value must be a string or an integer (DWORD) - unsupported type supplied");
	}

	std::string strPath = v8pp::from_v8<std::string>(isolate, args[0]);
	char* szPath = (char*)strPath.c_str();
	char* szFile = parse_path(&szPath);

	HKEY hkey = *szPath ? open_key(szPath, true) : hkey_;
	if ( hkey )
	{
		LONG lResult = 0;
		if( args[1]->IsString() ) // REG_SZ
		{
			std::string const str = v8pp::from_v8<std::string>(isolate, args[1]);
			lResult = ::RegSetValueExA(hkey, szFile, 0, REG_SZ, (LPBYTE)str.c_str(), (DWORD)str.length());
		}
		else
		if ( args[1]->IsUint32() ) // REG_DWORD
		{
			uint32_t const value = v8pp::from_v8<uint32_t>(isolate, args[1]);
			lResult = ::RegSetValueExA(hkey, szFile, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
		}
		else
		{
			args.GetReturnValue().Set(v8pp::throw_ex(isolate, "Unsupported registry value type"));
		}

		if ( hkey != hkey_ )
		{
			close_key(hkey);
		}

		if ( lResult != ERROR_SUCCESS )
		{
			throw_error(lResult, "Unable to write registry value " + strPath);
		}
	}
}

} // aspect
