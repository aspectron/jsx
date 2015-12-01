//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

/**
@module xml XML
XML support
**/

var xml = (function()
{
    var xml = rt.bindings.xml;
    
    /**
    @function toObject(xml)
    @param str {String}
    @return {Object}
    Parse XML `str` and return it as a JavaScript Object
    **/
    xml.toObject = function(str)
    {
        var p = new xml.Parser();
        p.root = null;
        p.stack = [];
        p.cur = null;
        p.start_node = function(name, attr) 
        {
            var ud = this;
            var o = { tag: name };
            if (!ud.root) ud.root = o;
            if (ud.cur) 
            {
                if (!ud.cur.children) ud.cur.children = [];
                ud.cur.children.push(o);
            }
            for (var k in attr) 
            {
                if (!o.attr) o.attr = {};
                o.attr[k] = attr[k];
            }
            ud.stack.push(o);
            ud.cur = o;
        };
        p.end_node = function(name) 
        {
            var ud = this;
            ud.stack.pop();
            ud.cur = ud.stack.length ? ud.stack[ud.stack.length - 1] : null;
        };
        p.char_handler = function(text) 
        {
            var ud = this;
            if (!ud.cur.cdata) ud.cur.cdata = "";
            ud.cur.cdata += text;
        };
        try 
        {
            p.parse(str, true);
        }
        finally 
        {
            //        p.destroy();
        }
        return p.root;
    }
    
    return xml;

})();

exports.$ = xml;
