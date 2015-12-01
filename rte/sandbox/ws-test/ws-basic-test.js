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

var log = require("log"),
    json = require("json"),
    http_rpc = require("http-rpc"),
    fs = require("fs"),
    db = require("db"),
    timer = require("timer"),
    _uuid_generator = require("uuid");
	
var uuid_generator = new _uuid_generator();
var local_uuid = uuid_generator.name("origin");
function generateUUID() { return uuid_generator.random(); }
    
include("libraries/datejs/date.js");


var config = 
{
    http_port : 1976,
    origin_address_pub : "tcp://127.0.0.1:2000",
    origin_address_sub : "tcp://127.0.0.1:2002"
//    origin_address_pub : 2000,
//    origin_address_sub : 2002
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

var ctx = 
{
    tio : { },
    //des : { },
    nav : { },
    origin : { }
};

var http = new http_rpc(config.http_port, {
    "/" : "/actinium/http",
    "/libraries" : "/libraries"
});


// http.http_iface.set_log_level(0);

http.rpc["fetch-updates"] = function(msg)
{
    var resp = { };
    return resp;
}


log.info("Creating Web Socket Server");

ctx.tio.clients = { }
ctx.tio.ws = new http.http_iface.websocket_service();

ctx.tio.ws
	.on('connect', function(resource) 
    {
        this.uuid = generateUUID();
        ctx.tio.clients[this.uuid] = this;

    })
    .on('close', function() 
    {
        log.error("tio closed");
        delete ctx.tio.clients[this.uuid];
    })
    .on('error', function(err) 
    {
        log.error("tio error");
        delete ctx.tio.clients[this.uuid];
    })
    .on('data', function(data)
    {
        log.info("Data received: " + data);
        this.write("SERVER RESPONDING TO DATA: [" + data + "]");
        this.close();
    });
http.digest("/ws-nav", ctx.tio.ws);
http.http_server.start();



