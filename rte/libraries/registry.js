//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

var registry = (function()
{
    var log = require("log");
    var _registry = new rt.bindings.library("registry");

    function Registry(hkey_root_string, path)
    {
        var self = this;
        self.__proto__ = new _registry.Registry(hkey_root_string, path);
        return self;
    }

    function split_path(full_path)  // HKEY\\...\\VALUE
    {
        full_path = full_path.replace(/\//g, "\\");
            
        var list = full_path.split('\\');
        if(list.length < 2)
            throw new Error("Invalid registry path supplied: "+full_path);
        var hkey = list[0];
        var filename = list[list.length-1];

        var sub_path_start = hkey.length+1;
        var path_to_value = full_path.substring(sub_path_start);//, full_path.length-sub_path_start);
        var sub_path_length = full_path.length-sub_path_start-(filename.length+1);
        var sub_path = path_to_value.substring(0, sub_path_length);

        var result = 
        {
            hkey : hkey,                // HKEY
            filename: filename,         // VALUE
            sub_path : sub_path,        // HKEY/[SUB_PATH]/VALUE
            path : path_to_value,   // SUB_PATH/VALUE
            full_path : full_path       // HKEY/SUB_PATH/VALUE
        }

        return result;
    }


    function registry_iface()
    {
        var self = this;

        self.Registry = Registry;   // Registry class for creation

        self.write = function(path, value)
        {
            var target = split_path(path);
            //log.info(target);
            var inst = new Registry(target.hkey); // , target.path);

            inst.write(target.path,value);
        }

        self.enumerate_values = function(path, cb_fn)
        {
            var target = split_path(path);
            // log.info(target);
            var inst = new Registry(target.hkey);//, target.path);

            var index = 0;
            while(true)
            {
                var filename = inst.enum_values(target.path,index++);
                if(!filename)
                    break;
                cb_fn.call(filename,filename);
            }
        }

        self.erase_values = function(path, cb_fn)
        {
            var target = split_path(path);
            var inst = new Registry(target.hkey);

            var values = [];
            var index = 0;
            while(true)
            {
                var filename = inst.enum_values(target.path,index++);
                if(!filename)
                    break;
                values.push(filename);
            }

            for(var i = 0; i < values.length; i++)
            {
                if(cb_fn.call(values[i]))
                    inst.erase_value(target.path+'\\'+values[i]);

            }
        }

        return self;
    }


    return new registry_iface();

})();

exports.$ = registry;