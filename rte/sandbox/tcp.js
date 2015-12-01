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

var tcp = require("tcp");
var timer = require("timer");
var v8 = require("v8");

function test_sync_client()
{
    var socket = new tcp.Socket();

    socket.connect("google.com", 80);

    console.log("send " + socket.send("GET / HTTP/1.0\n\n") + " bytes");
    
    var buf = new Buffer;
    while (socket.receive(buf))
    {
        console.log("received: " + buf.string());
    }

    socket.close();
}

function test_async_client()
{
    v8.runGC();
    var socket = new tcp.Socket();

    socket
		.on('error', function(error) { console.error("socket error: " + error); })
		.on('connect', function() { this.asyncSend("GET / HTTP/1.0\n\n"); })
		.on('send', function(bytes_sent) { this.asyncReceive(500); })
		.on('data', function(data) { console.log(data.string()); this.close(); })
        .on('close', function(){ delete this; test_async_client(); });
	socket.asyncConnect("google.com", 80);
}

function test_server()
{
    trace("running tcp server on 3344");

    var rt = global.rt;

    var server = new tcp.Server;

    server.run(3344,
        function(socket)
        {
            trace("server handler");

            var buf = new Buffer;
            socket.receive(buf);
            trace("client sent " + buf.string());

            var http = new tcp.Socket();
            http.connect(buf.string(), 80);
            http.send("GET / HTTP/1.0\n\n");
            http.receive(buf, 1500);
            http.close();

            socket.send(buf);
            socket.close();
        });

    var client = new tcp.Socket;
    client.connect("localhost:3344");
    console.log("client connected");

    client.send("ya.ru");
    console.log("request sent");

    var buf = new Buffer;
    client.t = new timer(1000,
        function()
        {
            trace("timer...");
            while (client.receive(buf))
            {
                console.log("received: " + buf.string());
            }
        });
}

//test_sync_client();
test_async_client();
//test_server();
