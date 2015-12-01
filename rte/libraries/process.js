//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

var process = (function()
{
    var process = rt.bindings.process;

    process.hx = function(args)
    {
        var cl = rt.execPath + ' ' + args;
        // trace("running: "+cl);
        return this.start(cl,process.PRIORITY_NORMAL);
    }

    process.digest_arguments = function(line)
    {
    //    var line = "c:\\aaa bbb\\ccc \\hx86.exe --parent 012345678  --module main.js --uuid 36095f4e-b9b7-5222-bd06-59babf019d12 alpha   betta gamma extra 123";
        var ln = line.split(' ');

    //     trace(line);
    //     trace("-------");
    //     trace(JSON.stringify(ln));

        var args = {};

        var hx = /hx/i;
        var digest = false;
        while(ln.length)
        {
            var arg = ln.shift();

            if (!digest)
            {
                if(arg.match(hx))
                    digest = true;
            }
            else
            {
                if(arg.indexOf('--') == 0)
                {
                    arg = arg.substring(2);
                    
                    if(arg.indexOf('=') != -1)    // --flag=value
                    {
                        var pair = arg.split('=');
                        args[pair.shift()] = pair[0] || true;
                    }
                    else                        // --flag value
                    if(ln.length && ln[0].indexOf('--') == -1)
                    {
                        args[arg] = ln[0];
                        ln.shift();
                    }
                    else                       // --flag
                        args[arg] = true;
                }
            }
        }

    //     trace("-------");
    //     trace(JSON.stringify(args));
    //     trace("-------");

        return digest ? args : null;
    }

    process.enumerate_hx_processes = function()
    {
        var out = [];    
        
        var list = process.prociface.list();
        for (var i = 0; i < list.length; i++) 
        {
            var info = process.prociface.getInfo(list[i]);
    //        trace(info.name);
      //      trace(info.cmd_line);
            info = process.digest_arguments(info.cmdLine);
            if(info)
                out.push(info);
        }

        return out;    
    }

    return process;
    
})();

exports.$ = process;
