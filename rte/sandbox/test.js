//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var log = require("log");

log.info("This is a test script... Hello!... :)");
log.trace('trace');
log.debug('debug');
log.info('info');
log.alert('alert');
log.warning('warning');
log.error('error');
log.panic('panic');

var json = JSON.stringify({a: 1, x: 'str', z: true, b: [{name:'value', name2: {}}, 99.9]});
console.log(json);
console.dir(JSON.parse(json), {depth: Infinity});
