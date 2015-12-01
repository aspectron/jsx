//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var netinfo = require('netinfo');
var os = require('os');

console.log('-------- getNetworkInterfaces --------');
console.dir(os.getNetworkInterfaces(), {colors: true, depth: Infinity});

console.log('-------- getAdapterInfo --------');
console.dir(netinfo.getAdapterInfo(), {colors: true, depth: Infinity});
//console.log(netinfo.getPublicIp());