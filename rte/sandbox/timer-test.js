//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var timer = require('timer');

dpc(function(start_ts) {
	console.log('dpc() delay %f ms, args:', getTimestamp() - start_ts, arguments);
}, getTimestamp(), 'aa', 3.14, true);

setTimeout(function(start_ts){
	console.log("setTimeout(0) delay %f ms, args:", getTimestamp() - start_ts, arguments);
}, 0, getTimestamp(), [1, 2, 3]);

var count = 0;
var last_ts = getTimestamp();
setInterval(function() {
	if (++count > 65)
	{
		console.log('clearInterval');
		clearInterval(this);
		return;
	}

	var ts = getTimestamp();
	console.log('setInterval(10) delay %f ms, args:', ts - last_ts, arguments);
	last_ts = ts;
}, 10, 99, 'xx');

setImmediate(function(start_ts){
	console.log('setImmediate() delay %f ms, args:', getTimestamp() - start_ts, arguments);
}, getTimestamp(), 'zz', {x:1, y: 'qq'});

var timer_ts = getTimestamp();
var timer_count = 0;
var t = new timer(20, function(){
	var ts = getTimestamp();
	console.log('timer interval: %d, ts: %f ms', this.interval, ts - timer_ts);
	this.interval += 10;
	this.enabled = ++timer_count < 10;
	timer_ts = ts;
});

function stress_test()
{
	var cnt = 0;
	var timers = [];

	for (var i = 0; i < 500; i++)
	{
		timers.push(new timer(0, function(){ console.log('test:', cnt); ++cnt; }, true, true));
	}
}
//stress_test();