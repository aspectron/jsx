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
    fs = require("fs"),
    http = require("http"),
    timer = require("timer"),
    json = require("json");

//
//  USE THIS TO SWITCH BETWEEN SSL (https) AND PLAIN (http)
//  THEN ACCESS BOTH HTTP AND HTTPS ON PORT 7777
//  https://localhost:7777/index.html
//  http://localhost:7777/index.html
//
var enable_ssl = true;

var self = { }

// create server
self.http_server = new http.Server(7777);
self.http_server.start();

var url = 'http';
if (enable_ssl) url += 's';

console.log('HTTP server started at ' + (enable_ssl? 'https' : 'http') + '://localhost:7777/');

// enable ssl
if(enable_ssl)
    self.http_server.setSslKeyfile(rt.local.rootPath + "/config/ssl.pem")

// server this folder as http /
self.http_server.digest("/", new http.FilesystemService(rt.local.scriptPath + "/index.html"));

// create websocket service
self.ws_service = new http.WebsocketService();

self.http_server.digest("/ws-ssl-tester", self.ws_service

    .on('connect', function(resource) {
        log.info("on_connect()")
        this.send("hello world #1 (in on_connect)!");
    })
    .on('close', function()
    {
        log.info("on_close()")
    })
    .on('error', function(error)
    {
        log.info("on_error()")
    })
    .on('data', function(_msg, is_binary)
    {
        log.info("on_data()")

        log.debug(_msg);

        this.send("hello world #2 (response)!");
    }));

