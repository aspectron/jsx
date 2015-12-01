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

var midi = require('midi'),
    json = require('json')
	log = require('log');

log.trace("Starting...");

var list = midi.enumerate_devices();

for(var i = 0; i < list.length; i++)
    log.trace(json.to_string(list[i]));

var input = new midi.input(1);
var output = new midi.output(3);

input.open();
output.open();

input.digest(function(event)
{
    log.trace(" RECEIVED MIDI EVENT: "+event);

    var val = event[2];

    for(var i = 0; i < 7; i++)    
    {
        event[1]++;
        event[2] = i & 1 ? val : 127-val;
        output.send(event);
    }
    
}); //.open();



// 
// 
// var str = "hello world";
// str.test = "something";
// 
// trace(str);
// trace(str.test);
// 
// trace(json.to_string(str,0,"\t"));