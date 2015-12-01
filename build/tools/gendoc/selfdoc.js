//
// Copyright (c) 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of Gendoc (https://github.com/aspectron/gendoc) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
// generate documentation for itself

var gendoc = require('./gendoc.js');

gendoc({ input: [rt.local.scriptPath + '/gendoc[.]js$'],
	output: rt.local.scriptPath,
	htmlOutput: rt.local.scriptPath });