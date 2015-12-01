//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
#import <Cocoa/Cocoa.h>

void cocoa_run()
{
	[NSApplication sharedApplication];
	[NSApp run];
	[NSApp release];
}

void cocoa_stop()
{
	[NSApp terminate:nil];
}