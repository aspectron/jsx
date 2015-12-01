//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

/**
@module netinfo Netinfo
**/

var netinfo = (function()
{
    var netinfo = rt.bindings.netinfo;

    /**
    @function getPublicIp([async_callback])
    @param [async_callback] {Function}
    @return {String} public IP, null on error
    Obtain our public IP if we are behind NAT. If an optional function
    parameter callback is specified, an asynchronous request would be performed
    and the `function async_callback(publicIP)` would be called.
    **/
    netinfo.getPublicIp = function(async_callback)
    {
        is_async = (typeof async_callback === 'function');
        var public_ip = null;

        var http = require("http");
        var http_client = new http.Client();
        http_client.setKeepAlive(false);
        http_client.ajax({
            url : "http://automation.whatismyip.com/n09230945.asp",
            async : is_async,
            success : function(data)
            {
                public_ip = data.string();
                if (is_async)
                    async_callback(public_ip);
            },
            failure : function()
            {
                if (is_async)
                    async_callback(null);
            }
        });

        return public_ip;
    }
	

    return netinfo;

})();

exports.$ = netinfo;