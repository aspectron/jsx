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
#include "jsx/v8_core.hpp"

#include <iphlpapi.h>

namespace aspect {

static void get_adapter_info(v8::FunctionCallbackInfo<v8::Value> const& args);

void setup_netinfo_bindings(v8pp::module& target)
{
	/**
	@module netinfo Netinfo
	Network adapters information, Windows only

	@function getAdapterInfo()
	@return {Array}
	Get network adapters information. Return an array of #AdapterInfo objects.

	@class AdapterInfo - Network adapter information
	@property name {String} Network adpater name
	@property alias {String} Network adpater alias
	@property ipList {Array} Adapter IP addresses
	Each entry in the IP list consists of `ip` and `mask` attributes.

	@property gatewayList {Array} Adapter gateways with `ip` and `mask` attributes
	@property dhcpList {Array} Adapter DHCP servers with `ip` and `mask` attributes
	@property dhcpEnabled {Boolean} Is DHCP enabled for the adapter
	@property mtu {Number} MTU size in bytes

	@property adminStatus {String} Adapter administrative status
	Could be one of the following values:
	  * `up`
	  * `down`
	  * `test`
	  * `unknown`
	@property connectState {String} Media connect state
	Could be one of the following values:
	  * `connected`
	  * `disconnected`
	  * `unknown`

	@property txSpeed              The speed of transmit link, bits per second
	@property rxSpeed              The speed of receive link, bits per second

	@property inOctets             The number of octets received without errors
	@property inUnicastPackets     The number of unicast packets received without errors
	@property inNonUnicastPackets  The number of non-unicast packets received without errors
	@property inDiscards           The number of incoming packets that were discarded even they didn't have errors
	@property inErrors             The number of incoming packets that were discarded because of errors
	@property inUnknownProtocols   The number of incoming packets that were discarded because of unknown protocol
	@property inUnicastOctets      The number of data octets received in unicast packets without errors
	@property inMulticastOctets    The number of data octets received in multicast packets without errors
	@property inBroadcastOctets    The number of data octets received in broadcast packets without errors

	@property outOctets            The number of octets transmitted without errors
	@property outUnicastPackets    The number of unicast packets transmitted without errors
	@property outNonUnicastPackets The number of non-unicast packets transmitted without errors
	@property outDiscards          The number of outgoing packets that were discarded even they didn't have errors
	@property outErrors            The number of outgoing packets that were discarded because of errors
	@property outUnknownProtocols  The number of outgoing packets that were discarded because of unknown protocol
	@property outUnicastOctets     The number of data octets transmitted in unicast packets without errors
	@property outMulticastOctets   The number of data octets transmitted in multicast packets without errors
	@property outBroadcastOctets   The number of data octets transmitted in broadcast packets without errors

	See also os#getNetworkInterfaces
	**/
	v8pp::module netinfo_module(target.isolate());
	netinfo_module.set("getAdapterInfo", get_adapter_info);
	target.set("netinfo", netinfo_module.new_instance());
}

static v8::Handle<v8::Value> ip_addr_string_to_array(v8::Isolate* isolate, IP_ADDR_STRING const* ip_str)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Array> ip_array = v8::Array::New(isolate);

	for (uint32_t ip_count = 0; ip_str != NULL; ++ip_count, ip_str = ip_str->Next)
	{
		v8::Local<v8::Object> ip_object = v8::Object::New(isolate);
		ip_array->Set(ip_count, ip_object);
		set_option(isolate, ip_object, "ip", ip_str->IpAddress.String);
		set_option(isolate, ip_object, "mask", ip_str->IpMask.String);
	}

	return scope.Escape(ip_array);
}

static void get_adapter_info(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);

	ULONG adapters_size = 0;
	GetAdaptersInfo(nullptr, &adapters_size);

	std::vector<char> buf(adapters_size);
	IP_ADAPTER_INFO* adapters = reinterpret_cast<IP_ADAPTER_INFO*>(buf.empty()? nullptr : &buf[0]);
	if ( GetAdaptersInfo(adapters, &adapters_size) != NO_ERROR )
	{
		throw new std::runtime_error("GetAdaptersInfo failed");
	}

	v8::Local<v8::Array> info = v8::Array::New(isolate);
	uint32_t idx = 0;
	for (IP_ADAPTER_INFO* adapter = adapters; adapter; adapter = adapter->Next)
	{
		v8::Local<v8::Object> o = v8::Object::New(isolate);
		info->Set(idx++, o);

		// digest interface info
		MIB_IF_ROW2 if_row = {};
		if_row.InterfaceIndex = adapter->Index;
		ULONG const ret = GetIfEntry2(&if_row);

		set_option(isolate, o, "name", adapter->Description);
		set_option(isolate, o, "alias", if_row.Alias);
		set_option(isolate, o, "ipList", ip_addr_string_to_array(isolate, &adapter->IpAddressList));
		set_option(isolate, o, "gatewayList", ip_addr_string_to_array(isolate, &adapter->GatewayList));
		set_option(isolate, o, "dhcpList", ip_addr_string_to_array(isolate, &adapter->DhcpServer));
		set_option(isolate, o, "dhcpEnabled", adapter->DhcpEnabled != 0);

		// TODO - populate additional data

		if ( ret == NO_ERROR )
		{
			set_option(isolate, o, "mtu", static_cast<uint32_t>(if_row.Mtu));

			const char* admin_status = "";
			switch ( if_row.AdminStatus )
			{
				case NET_IF_ADMIN_STATUS_UP:
					admin_status = "up";
					break;
				case NET_IF_ADMIN_STATUS_DOWN:
					admin_status = "down";
					break;
				case NET_IF_ADMIN_STATUS_TESTING:
					admin_status = "test";
					break;
				default:
					admin_status = "unknown";
					break;
			}
			set_option(isolate, o, "adminStatus", admin_status);

			const char* media_connect_state = "";
			switch (if_row.MediaConnectState)
			{
				case MediaConnectStateUnknown:
					media_connect_state = "unknown";
					break;
				case MediaConnectStateConnected:
					media_connect_state = "connected";
					break;
				case MediaConnectStateDisconnected:
					media_connect_state = "disconnected";
					break;
				default:
					media_connect_state = "unknown";
					break;
			}
			set_option(isolate, o, "connectState", media_connect_state);

			set_option(isolate, o, "txSpeed", if_row.TransmitLinkSpeed);
			set_option(isolate, o, "rxSpeed", if_row.ReceiveLinkSpeed);
			set_option(isolate, o, "inOctets", if_row.InOctets);
			set_option(isolate, o, "inUnicastPackets", if_row.InUcastPkts);
			set_option(isolate, o, "inNonUnicastPackets", if_row.InNUcastPkts);
			set_option(isolate, o, "inDiscards", if_row.InDiscards);
			set_option(isolate, o, "inErrors", if_row.InErrors);
			set_option(isolate, o, "inUnknownProtocols", if_row.InUnknownProtos);
			set_option(isolate, o, "inUnicastOctets", if_row.InUcastOctets);
			set_option(isolate, o, "inMulticastOctets", if_row.InMulticastOctets);
			set_option(isolate, o, "inBroadcastOctets", if_row.InBroadcastOctets);
			set_option(isolate, o, "outOctets", if_row.OutOctets);
			set_option(isolate, o, "outUnicastPackets", if_row.OutUcastPkts);
			set_option(isolate, o, "outNonUnicastPackets", if_row.OutNUcastPkts);
			set_option(isolate, o, "outDiscards", if_row.OutDiscards);
			set_option(isolate, o, "outErrors", if_row.OutErrors);
			set_option(isolate, o, "outUnicastOctets", if_row.OutUcastOctets);
			set_option(isolate, o, "outMulticastOctets", if_row.OutMulticastOctets);
			set_option(isolate, o, "outBroacastOctets", if_row.OutBroadcastOctets);
		}
	}

	args.GetReturnValue().Set(scope.Escape(info));
}

} // namespace aspect
