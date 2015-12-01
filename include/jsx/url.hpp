//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#ifndef JSX_URL_HPP_INCLUDED
#define JSX_URL_HPP_INCLUDED

#include "jsx/http.hpp"

namespace aspect {

/// URL class
class CORE_API url
{
public:
	/// URL parts
	enum part_type
	{
		SCHEME, USERINFO, HOST, PORT, PATH, QUERY, FRAGMENT,
		PART_COUNT
	};

	url() {}

	/// Create an URL from string
	explicit url(std::string const& str)
	{
		parse(str);
	}

	/// Create an URL from optional V8 string
	explicit url(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		parse(v8pp::from_v8<std::string>(args.GetIsolate(), args[0], ""));
	}

	bool operator==(url const& rhs) const
	{
		return std::equal(parts_, parts_ + PART_COUNT, rhs.parts_);
	}

	bool operator!=(url const& rhs) const
	{
		return !(*this == rhs);
	}

	/// Is URL empty
	bool empty() const
	{
		for (size_t i = 0; i != PART_COUNT; ++i)
		{
			if (!parts_[i].empty()) return false;
		}
		return true;
	}

	/// Clear URL
	void clear()
	{
		for (size_t i = 0; i != PART_COUNT; ++i)
		{
			parts_[i].clear();
		}
	}

	/// Parse URL from string
	void parse(std::string const& str);

	/// Convert URL to string
	std::string to_string() const;

	/// Get URL part
	std::string const& part(part_type type) const { return parts_[type]; }
	url& set_part(part_type type, std::string const& value) { parts_[type] = value; return *this; }

	/// URL scheme, empty if not present
	std::string const& scheme() const { return part(SCHEME); }
	url& set_scheme(std::string const& value) { return set_part(SCHEME, value); }

	/// URL user information, empty if not present
	std::string const& userinfo() const { return part(USERINFO); }
	url& set_userinfo(std::string const& value) { return set_part(USERINFO, value); }

	/// URL host, empty if not present
	std::string const& host() const { return part(HOST); }
	url& set_host(std::string const& value) { return set_part(HOST, value); }

	/// URL port, empty if not present
	std::string const& port() const { return part(PORT); }
	url& set_port(std::string const& value) { return set_part(PORT, value); }

	/// URL path, empty if not present
	std::string const& path() const { return part(PATH); }
	url& set_path(std::string const& value) { return set_part(PATH, value); }

	/// URL query, empty if not present
	std::string const& query() const { return part(QUERY); }
	url& set_query(std::string const& value) { return set_part(QUERY, value); }

	/// URL fragment, empty if not present
	std::string const& fragment() const { return part(FRAGMENT); }
	url& set_fragment(std::string const& value) { return set_part(FRAGMENT, value); }

	// Is scheme secured
	static bool is_scheme_secured(std::string const& scheme);
	bool is_scheme_secured() const { return is_scheme_secured(scheme()); }

	/// Default port for specified scheme or 0 for unknown scheme
	static unsigned default_port(std::string const& scheme);
	unsigned default_port() const { return default_port(scheme()); }

	/// Default port, if no port provided
	unsigned effective_port() const;

	/// Resolve a relative reference against the given URL
	url resolve(std::string const& relative) const;

	/// Get URL origin (scheme, host and port parts)
	std::string origin() const;

	/// Get URL host and port parts
	std::string hostport() const;

	/// Get URL without path, query and fragment parts
	std::string authority() const;

	/// Get URL without scheme, userinfo, host and port parts
	std::string path_for_request() const;

private:
	std::string parts_[PART_COUNT];
};

} // ::aspect

#endif //JSX_GURL_HPP_INCLUDED
