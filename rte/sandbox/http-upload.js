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

var http = require("http");

// create server
var http_server = new http.Server(7777);
http_server.start();

// server this folder as http /
var fs_service = new http.FilesystemService("/sandbox");
fs_service.setOption("writable", "true");
http_server.digest("/", fs_service);

trace("Server started");