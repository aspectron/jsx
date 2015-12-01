//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var os = require('os');

console.log('runtime args:', rt.args);
console.log('exec_path:', rt.local.execPath);
console.log('script:', rt.local.script);
console.log('script args:', rt.local.scriptArgs);

messageBox("привет!", "мир");

rt.__atExit(function(){trace('bye!');});
rt.exit();

os.chdir('/');

os.exec('"' + rt.local.execPath + '"');

console.log('info: ', os.info);
console.log('memory: ', os.getMemoryInfo());
console.log('storage: ', os.getStorageSpace('/'));
console.log('network: ', os.getNetworkInterfaces());
