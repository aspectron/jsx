//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var process = require("process");

var procs = process.list();
console.log("pid list:", procs);

var curr = process.current();
console.log("current pid:", curr);
console.log("is running:", process.isRunning(curr));

var info = process.getInfo(curr);
console.log("info:", info);
if (info.cpu.start) console.log("started at", new Date(info.cpu.start));
if (info.cpu.exit) console.log("exited  at", new Date(info.cpu.exit));

process.setPriority(curr, process.PRIORITY_HIGH);

console.log('start calc')
var calc = process.start("calc.exe");
console.log("calc pid:", calc);
console.log("calc is running:", process.isRunning(calc));
//console.log("calc info:", process.getInfo(calc));

console.log('kill calc');
process.kill(calc);
console.log("calc is running:", process.isRunning(calc));

var calcInfo = process.getInfo(calc);
console.log("calc info:", calcInfo);

if (calcInfo.cpu.start) console.log('calc started at', new Date(calcInfo.cpu.start));
if (calcInfo.cpu.exit)  console.log('calc exited at', new Date(calcInfo.cpu.exit));

console.log("system cpu usage:", process.getCpuUsage());
