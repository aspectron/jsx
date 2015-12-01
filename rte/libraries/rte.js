//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

var fs = require('fs');
//	json = require('json');

var log = require('log');

var runtime = global.rt;
//log.debug(runtime.local);

//
// iterate up in script path, looking for config/cfg-file match
var p = runtime.local.scriptPath.replace(/\\/g,'/').split('/');
var cfg_file = '';
var found = false;
while(!found && p.length)
{
	cfg_file = p.join('/') + '/config/'+runtime.local.cfgFile;
	if(fs.exists(cfg_file))
	{
		found = true;
		break;
	}

	p.pop();
}


//var cfg_file = rt.local.rootPath + '/config/'+rt.local.cfgFile;
if(found)//fs.exists(cfg_file))
{
	try 
	{
		var cfg = fs.readFile(cfg_file);
		// log.info('config file contents:'+cfg.string());
		if(cfg) cfg = eval('('+cfg.string()+')');
		if(cfg.include && typeof(cfg.include) == 'string')
			cfg.include = cfg.include.split(';');

//		log.debug(cfg.include);

		runtime.resetRteIncludePaths();
		for(var i = 0; i < cfg.include.length; i++)
		{
			// log.trace('adding path:'+cfg.include[i]);
			runtime.addRteIncludePath(cfg.include[i]);
		}
	}
	catch(ex)
	{
//		console.log("rte.js - config file error:",ex);
		log.error("rte.js - config file error:",ex);
	}
}
else
{
//	log.warn('unable to find '+'~/config/'+runtime.local.cfgFile);
}