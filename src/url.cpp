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
#include "jsx/url.hpp"

namespace aspect {

static std::string default_port(std::string const& scheme);

std::string url::to_string() const
{
	std::string result;

	result.reserve(scheme().size() + (!scheme().empty()) * 3
		+ userinfo().size() + !userinfo().empty()
		+ host().size()
		+ port().size() + !port().empty()
		+ path().size() + !path().empty()
		+ query().size() + !query().empty()
		+ fragment().size() + !fragment().empty());

	if (!scheme().empty())
	{
		result += scheme();
		result += "://";
	}

	if (!userinfo().empty())
	{
		result += userinfo();
		result += '@';
	}

	if (!host().empty())
	{
		result += host();
	}

	if (!port().empty() && effective_port() != default_port(scheme()))
	{
		result += ':';
		result += port();
	}

	if (!path().empty())
	{
		if (path().front() != '/') result += '/';
		result += path();
	}

	if (!query().empty())
	{
		assert(!path().empty());
		result += '?';
		result += query();
	}

	if (!fragment().empty())
	{
		assert(!query().empty());
		result += '#';
		result += fragment();
	}
	return result;
}

void url::parse(std::string const& str)
{
	clear();

	std::string::size_type pos, off = 0;

	pos = str.find("://", off);
	if (pos != str.npos)
	{
		set_scheme(str.substr(off, pos - off));
		off = pos + 3;
	}

	pos = str.find('@', off);
	if (pos != str.npos)
	{
		set_userinfo(str.substr(off, pos - off));
		off = pos + 1;
	}

	pos = str.find(':', off);
	if (pos != str.npos)
	{
		set_host(str.substr(off, pos - off));
		off = pos + 1;
	}

	pos = str.find('/', off);
	if (pos != str.npos)
	{
		std::string const s = str.substr(off, pos - off);
		host().empty()? set_host(s) : set_port(s);
		off = pos;

		pos = str.find('?', off);
		if (pos != str.npos)
		{
			set_path(str.substr(off, pos - off));
			assert(!path().empty());
			off = pos + 1;

			std::string::size_type fragment_pos = str.find('#', off);
			if (fragment_pos != str.npos)
			{
				set_query(str.substr(off, fragment_pos - off));
				set_fragment(str.substr(fragment_pos + 1));
			}
			else
			{
				set_query(str.substr(off));
			}
		}
		else
		{
			set_path(str.substr(off));
		}
	}
	else
	{
		std::string const s = str.substr(off, pos - off);
		host().empty()? set_host(s) : set_port(s);
	}
}

bool url::is_scheme_secured(std::string const& scheme)
{
	return scheme == "https" || scheme == "wss";
}

unsigned url::default_port(std::string const& scheme)
{
	if (scheme == "http" || scheme == "ws")
	{
		return 80;
	}
	if (scheme == "https" || scheme == "wss")
	{
		return 443;
	}
	return 0;
}

unsigned url::effective_port() const
{
	// atoi usage is acceptable to return 0 on error
	return port().empty()? default_port() : static_cast<unsigned>(atoi(port().c_str()));
}

url url::resolve(std::string const& relative) const
{
	url const rel(relative);

	url result = *this;
	result.set_path(rel.path()).set_query(rel.query()).set_fragment(rel.fragment());
	return result;
}

std::string url::origin() const
{
	url result;

	result.set_scheme(scheme()).set_host(host()).set_port(port());
	return result.to_string();
}

std::string url::hostport() const
{
	url result;

	result.set_host(host()).set_port(port());
	return result.to_string();
}

std::string url::authority() const
{
	url result = *this;

	result.set_path("").set_query("").set_fragment("");
	return result.to_string();
}

std::string url::path_for_request() const
{
	url result = *this;

	result.set_scheme("").set_userinfo("").set_host("").set_port("");
	return result.to_string();
}

} // aspect
