//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
pragma("event-queue");

var 
    log = require("log"),
    json = require("json"),
    http = require("http");

var client = new http.Client();
 
client.options.verbose = true;

client.connect("http://www.google.com");

//var status = client.get("/?q=alphabet");
//var status = client.get("/", { "q": "alphabet" });
/*

log.info("STATUS: "+status);
log.info("STATUS MSG: "+client.client.get_status_message());
var headers = client.client.get_response_headers();
var str = json.to_string(headers,null,"\t");
log.info(str);

*/

var requests = 3;

//var _success = function()

var _headers = {

    "Cache-Control": "max-age=0",
    "Accept-Encoding": "gzip,deflate",
    "Accept": "text/html,application/xhtml+xml,application/xml",
    "Accept-Charset": "ISO-8859-1,utf-8",
    "User-Agent": "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/14.0.835.202 Safari/535.1",
    "Accept-Language": "en-US",
    "Connection": "Keep-Alive"
    //                "Accept-Language" : "en-US",
};


function do_request()
{
    if(!requests)
        return;
    requests--;
    log.info("doing request "+(3-requests));
    client.ajax({
        async : false,
        url : "/?q=zzzвыва#99",
        method : "GET",
        headers : _headers,
        success : function()
        {
            console.log("success:", arguments);
            dpc(do_request);
        },
        failure : function(e)
        {
            console.error("failure:", arguments);
        }

    });
}

do_request();

//log.trace("hello...");