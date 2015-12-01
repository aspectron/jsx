//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
/*
var NODE_LIB = 'C:\\Projects\\node-v0.10.22\\lib';

//node_libraries = require('fs').locate_files(NODE_LIB);

rt.setNodeLibPath(NODE_LIB);

//var b = new Buffer();
//console.dir(b);

/*
var util = require('util');

function dir(name, obj) { util.puts(name + ": " + util.inspect(obj))}

var node_libraries = [
  'assert', 'buffer', 'console', 'constants', 'crypto', //'child_process', 'cluster',  'dgram', 'dns', 'domain',
//  'events', 'freelist', 'fs', 'http', 'https', 'module', 'net', 'os', 'path', 'punycode', 'querystring',
//  'readline', 'repl', 'stream', 'string_decoder', 'sys', 'timers', 'tls', 'tty', 'url', 'util', 'vm', 'zlib'
  ];

for (i in node_libraries)
//var i = 6;
{
  var name = node_libraries[i];
  var lib = require(name);
  dir(name, lib);
}
*/


/*
console.log('cwd: ' + process.cwd());
process.chdir(process.env.HOMEPATH);
console.log('cwd: ' + process.cwd());

process.env.ZZ = 99;
console.dir(process.env);
delete process.env.ZZ;
console.dir(process.env);
*/

/*
console.log('uptime: ' + process.uptime());
var hr = process.hrtime();

var count = 0;

setInterval(function()
	{
		if (++count > 10)
		{
			clearInterval(this);
			return;
		}

		//var new_hr = process.hrtime();
		//console.log('uptime: ' + process.uptime());
		//console.log('hrtime: ' + new_hr);
		console.log('hr diff:' + process.hrtime(hr));
		//console.log('--------------------------------');
		//hr = new_hr;
	}, 10);
*/

/*
process.maxTickDepth = 3;
console.log('max callbacks: ' + process.maxTickDepth);

var tick_cnt = 0, tack_cnt = 0;
function tick() { ++tick_cnt; console.log('tick ' + tick_cnt); process.nextTick(tack); }
function tack() { ++tack_cnt; console.log('tack ' + tack_cnt); process.nextTick(tick); }

process.nextTick(tick);
*/

var colors = require('colors');

console.log('Абв'.yellow.bold);
var test = colors.red("hopefully colorless output");
console.log('Rainbows are fun!'.rainbow);
console.log('So '.italic + 'are'.underline + ' styles! '.bold + 'inverse'.inverse); // styles not widely supported
console.log('Chains are also cool.'.bold.italic.underline.red); // styles not widely supported
//console.log('zalgo time!'.zalgo);
console.log(test.stripColors);
console.log("a".grey + " b".black);
console.log("Zebras are so fun!".zebra);
console.log('background color attack!'.black.whiteBG)

//
// Remark: .strikethrough may not work with Mac OS Terminal App
//
console.log("This is " + "not".strikethrough + " fun.");
console.log(colors.rainbow('Rainbows are fun!'));
console.log(colors.italic('So ') + colors.underline('are') + colors.bold(' styles! ') + colors.inverse('inverse')); // styles not widely supported
console.log(colors.bold(colors.italic(colors.underline(colors.red('Chains are also cool.'))))); // styles not widely supported
//console.log(colors.zalgo('zalgo time!'));
console.log(colors.stripColors(test));
console.log(colors.grey("a") + colors.black(" b"));

colors.addSequencer("america", function(letter, i, exploded) {
  if(letter === " ") return letter;
  switch(i%3) {
    case 0: return letter.red;
    case 1: return letter.white;
    case 2: return letter.blue;
  }
});

colors.addSequencer("random", (function() {
  var available = ['bold', 'underline', 'italic', 'inverse', 'grey', 'yellow', 'red', 'green', 'blue', 'white', 'cyan', 'magenta'];

  return function(letter, i, exploded) {
    return letter === " " ? letter : letter[available[Math.round(Math.random() * (available.length - 1))]];
  };
})());

console.log("AMERICA! F--K YEAH!".america);
console.log("So apparently I've been to Mars, with all the little green men. But you know, I don't recall.".random);

//
// Custom themes
//

// Load theme with JSON literal
colors.setTheme({
  silly: 'rainbow',
  input: 'grey',
  verbose: 'cyan',
  prompt: 'grey',
  info: 'green',
  data: 'grey',
  help: 'cyan',
  warn: 'yellow',
  debug: 'blue',
  error: 'red'
});

// outputs red text
console.log("this is an error".error);

// outputs yellow text
console.log("this is a warning".warn);

// outputs grey text
console.log("this is an input".input);

// Load a theme from file
theme = 'colors/themes/winston-dark.js';
console.log('set theme ' + theme)
colors.setTheme(theme);

console.log("this is an error".error);
console.log("this is a warning".warn);
console.log("this is an input".input);
