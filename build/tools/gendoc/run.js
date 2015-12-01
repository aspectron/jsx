//
// Copyright (c) 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of Gendoc (https://github.com/aspectron/gendoc) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
// run gendoc.js from command line

var gendoc = require('./gendoc.js');

var title;
var input_dirs;
var output_dir;

if (typeof rt !== 'undefined')
{
	if (rt.local.scriptArgs.length == 0)
	{
		console.error('Usage: %s <output dir> <input dir/filter regex> ... <recurse input dir/*/filter regex>', rt.local.scriptName);
		rt.exit(-1);
	}
	title = 'JSX ' + rt.version + ' Documentation';
	output_dir = rt.local.scriptArgs[0];
	input_dirs = rt.local.scriptArgs.slice(1);
}
else if (typeof process !== 'undefined')
{
	if (process.argv.length < 5)
	{
		console.error('Usage: %s <title> <output dir> <input dir/filter regex> ... <recurse input dir/*/filter regex>', process.argv[1]);
		process.exit(-1);
	}
	title = process.argv[2];
	output_dir = process.argv[3];
	input_dirs = process.argv.slice(4);
}
var config = {
	input: input_dirs,
	output: output_dir,
	htmlOutput: output_dir + '/html',
	htmlTitle: title || '',
	cleanup: true,
//	reporter: function() {}, // null reporter speeds up gendoc() execution
	result: 'gendoc',
	index: 'index',
	single: 'all',
};

gendoc(config);
