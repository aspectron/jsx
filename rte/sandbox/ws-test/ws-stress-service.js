//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
pragma("event-queue gc-notify");

var log = require("log"),
    http = require("http"),
    timer = require("timer");

log.info(rt.local.rootPath);

var self = { }

var service_connection_id = 0;
var service_connections = 0;

self.http_server = new http.Server(7777);
self.http_server.start();
self.ws_service = new http.WebsocketService();
self.http_server.digest("/ws-stress", self.ws_service
    .on('connect', function(resource) 
    {
        service_connections++;
        log.info("Connection to resource:"+resource);

        if(this.test_field)
            log.warn("test field already exists in this websocket object!");
        this.test_field = getTimestamp();

    //    this.uuid = service_connection_id++;
    })
    .on('close', function() 
    {
        service_connections--;
    })
    .on('error', function(error) 
    {
        log.warn("error: " + error);
    })
    .on('data', function(_msg, is_binary)
    {
        if(_msg != "test request")
            log.warn("data is not a valid test request: " + _msg);

        this.send("test response");
        this.close();
    }));

// report status every 3 seconds
var info_iteration = new timer(3000, function () {
    log.info("active websocket connections: "+service_connections);
});