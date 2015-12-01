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
#include "jsx/firewall.hpp"

#include <comutil.h>
#include <comdef.h>

#include <netfw.h>

namespace aspect { namespace firewall {

using _com_util::CheckError;

void setup_bindings(v8pp::module& target)
{
	CheckError(::CoInitialize(nullptr));

	/**
	@module firewall Firewall
	Firewall rules management, Windows only
	**/
	v8pp::module firewall_module(target.isolate());
	firewall_module
		/**
		@function register(rule_name, path)
		@param rule_name {String}
		@param path {String}
		Add Windows Firewall exception with `rule_name` for process at `path`
		**/
		.set("register", add_rule)

		/**
		@function unregister(rule_name, path)
		@param rule_name {String}
		@param path {String}
		Remove Windows Firewall exception `rule_name` for process at `path`
		**/
		.set("unregister", remove_rule)
		;
	target.set("firewall", firewall_module.new_instance());
}

_COM_SMARTPTR_TYPEDEF(INetFwMgr, __uuidof(INetFwMgr));
_COM_SMARTPTR_TYPEDEF(INetFwPolicy, __uuidof(INetFwPolicy));
_COM_SMARTPTR_TYPEDEF(INetFwProfile, __uuidof(INetFwProfile));
_COM_SMARTPTR_TYPEDEF(INetFwAuthorizedApplications, __uuidof(INetFwAuthorizedApplications));
_COM_SMARTPTR_TYPEDEF(INetFwAuthorizedApplication, __uuidof(INetFwAuthorizedApplication));

static INetFwAuthorizedApplicationsPtr firewall_apps()
{
	INetFwMgrPtr fwMgr(L"HNetCfg.FwMgr");

	INetFwPolicyPtr policy;
	CheckError(fwMgr->get_LocalPolicy(&policy));

	INetFwProfilePtr profile;
	CheckError(policy->get_CurrentProfile(&profile));

	INetFwAuthorizedApplicationsPtr apps;
	CheckError(profile->get_AuthorizedApplications(&apps));
	return apps;
}

void add_rule(std::wstring const& name, boost::filesystem::path const& program)
try
{
	INetFwAuthorizedApplicationPtr app(L"HNetCfg.FwAuthorizedApplication");
	CheckError(app->put_ProcessImageFileName(_bstr_t(program.native().c_str())));
	CheckError(app->put_Name(_bstr_t(name.c_str())));
	CheckError(app->put_Scope(NET_FW_SCOPE_ALL));
	CheckError(app->put_IpVersion(NET_FW_IP_VERSION_ANY));
	CheckError(app->put_Enabled(VARIANT_TRUE));
		
	CheckError(firewall_apps()->Add(app));
}
catch (_com_error const& e)
{
	std::string const err = _bstr_t(e.ErrorMessage());
	throw std::runtime_error("Unable to perform firewall exception registration for " + program.string() + ": " + err);
}

void remove_rule(boost::filesystem::path const& program)
try
{
	CheckError(firewall_apps()->Remove(_bstr_t(program.native().c_str())));
}
catch (_com_error const& e)
{
	std::string const err = _bstr_t(e.ErrorMessage());
	throw std::runtime_error("Unable to unregister target binary from firewall for " + program.string() + ": " + err);
}

}} // namespace aspect::filrewall
