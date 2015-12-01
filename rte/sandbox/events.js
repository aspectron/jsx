//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var NODE_LIB = 'C:\\Projects\\node-v0.10.22\\lib';

rt.setNodeLibPath(NODE_LIB);

//var stream = require('stream');
var events = require('events');

var ee = new events.EventEmitter();

ee.on('newListener', function(name, listener){console.log('add on ' + name + '=' + listener);});
ee.on('removeListener', function(name, listener){console.log('del on ' + name + '=' + listener);});

function f() {}
ee.once('test', f);
ee.on('test', f);
ee.once('test', function(){console.log('once test1');});
ee.on('test', function(){console.log('test ' + arguments[0]);});
ee.once('test', function(){console.log('once test2');});
ee.once('once', function(){console.log('once ' + arguments[0]);});
ee.on('test', function(){console.log('test2 ' + arguments[0] + arguments[1]);});
ee.removeListener('test', f);

console.log('test listeners ' + ee.listeners('test'));
console.log('once listeners ' + ee.listeners('once'));

console.log('test count: ' + events.EventEmitter.listenerCount(ee, 'test'));
console.log('once count: ' + events.EventEmitter.listenerCount(ee, 'once'));

ee.emit('test', 'zz', 22);
ee.emit('once', true);
ee.emit('once', 0);
ee.emit('test', 'aa', 11);

console.log('test listeners ' + ee.listeners('test'));
console.log('once listeners ' + ee.listeners('once'));

if (ee.emit(99))
{
	console.error('emitted nonexistent type');
}

ee.removeAllListeners('once');
ee.removeAllListeners();

console.log('test count: ' + events.EventEmitter.listenerCount(ee, 'test'));
console.log('once count: ' + events.EventEmitter.listenerCount(ee, 'once'));
console.log('zz count: ' + events.EventEmitter.listenerCount(ee, 'zz'));