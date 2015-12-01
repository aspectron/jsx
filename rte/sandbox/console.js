//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
//rt.set_node_lib_path('C:\\Projects\\node-v0.10.22\\lib');

var util = require('util');
var console = require('console');
var v8 = require('v8');
var os = require('os');

/*
var orig_inspect = util.inspect;
function my_inspect(x) { return orig_inspect(x, {colors: true}); }
util.inspect = my_inspect;
util.inspect.styles = orig_inspect.styles;
util.inspect.colors = orig_inspect.colors;
*/

console.log('V8 version: %s', v8.version);
console.info('execute in private context(2+2):', v8.executeInPrivateContext('2+2'));
try { v8.executeInCurrentContext('zz', 'aaa'); }
catch(err) { console.error(err); }

console.warn('warning test');
console.dir(console, {colors: true});

console.time('zz');
console.trace('xx');
console.timeEnd('zz');

try
{
	console.assert(2+2 == 2*2);
	console.assert('aa' === 'bb', 'string equal');
}
catch(err) { console.error(err.stack); }

console.caption = 'new caption';
console.hide();
//os.sleep(3000);
console.show();