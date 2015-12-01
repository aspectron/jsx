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

var tcp = require("tcp");
var timer = require("timer");

/*
var socket = new tcp.Socket();
console.dir(socket);
socket.connect("google.com", 80);
console.log("send " + socket.send("GET / HTTP/1.0\n\n") + " bytes");
var buf = new Buffer;

socket.t = new timer(1000,function(){
console.log("timer...");
while (socket.receive(buf)) {
    console.log("received: " + buf.string());
}
});
//socket.close();
*/

trace("running tcp server on 3344");

var rt = global.rt;

var server = new tcp.Server;


server.recieve_processor = function(data)
{
    console.log("recv from client ("+data.size()+"): " + data.string());

    this.send("YES\n");

//    this.asyncReceive(1, server.recieve_processor);
}

server.run(3344, function(socket)
    {
        console.log("server handler");
        console.log(socket.localEndpoint);
        console.log(socket.remoteEndpoint);
        rt.mysock = socket;
        socket.asyncReceive(server.recieve_processor);

    socket.on_error(function(){ console.error("error!"); });

    });


/*
var socket = new tcp.Socket();

socket.on_error(function(error)
{
    console.error("something has gone horribly wrong! :)");
});

socket.on_error(function(error)
{
    console.error("socket error: " + error);
});

socket.asyncConnect("google.com", 80,
    function()
    {
        this.asyncSend("GET / HTTP/1.0\n\n",
            function(bytes_sent)
            {
                this.asyncReceive(
                    function(data)
                    {
                        trace(data.string());
                        this.close();
                    }, 1500);
            });
    });
*/

//tcp.wait()
