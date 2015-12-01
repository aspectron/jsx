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

var v8 = require("v8");
var log = require("log");
var process = require("process");
var timer = require("timer");
var pid = process.current();

function mem_usage()
{
	return process.getInfo(pid).memory.workingSetSize / (1024*1024);
}

var t = new timer(1000, function()
	{
		log.trace("memory usage: " +  mem_usage());
	});

var t2 = new timer(50, function()
	{
		var b = new Buffer(1024*1024);

//		var mem1 = mem_usage();
		delete b;

//		var mem2 = mem_usage();
		v8.runGC();
//		var mem3 = mem_usage();

//		log.trace("memory usage before buffer delete: " + mem1);
//		log.trace("memory usage before GC: " +  mem_usage());
//		log.trace("memory usage after GC: " +  mem_usage());
	});
