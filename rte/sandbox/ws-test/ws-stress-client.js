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

var client_connections = 0;

var client_links = {};
var seq = 0;


// TODO - after initial problems are fixed, reduce timer to 0mset (heavier load)
// and create multiple connections per iteration
var test_iteration = new timer(00, function () {

    log.info("connecting...");

    var ws = new http.Websocket("ws://127.0.0.1:7777/ws-stress");
    if(!ws)
        throw new Error("Unable to connect to localhost for stress testing");

// ########################
    ws.id = seq++;
    client_links[ws.id] = ws;
// ########################

    ws
    .on('connect', function()
    {
		log.info("connected...");
		client_connections++;
		this.send("test request");
    })
    .on('close', function()
    {
        log.info("disconnected...");
        client_connections--;

// ASY TESTING - RELEASE REFERENCE TO WS...
// ########################
	  delete client_links[this.id];
// ########################
    })
    .on('error', function(error)
    {
        log.warn("error: " + error);
    })
    .on('data', function(msg, is_binary){

        if(msg != "test response")
            log.warn("data is not a valid test response: " + msg);

        log.info("closing...");
        this.close();
//	  delete client_links[this.id];

		if(!client_links[this.id])
			log.error("no such websocket");

    });

});


var info_iteration = new timer(3000, function () {
    log.info("client connections: "+client_connections);
});