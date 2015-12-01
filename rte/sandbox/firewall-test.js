//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
if (rt.platform == 'windows')
{
	var firewall = require('firewall');
	firewall.register('jsx', rt.local.execPath);
	firewall.unregister(rt.local.execPath);
}