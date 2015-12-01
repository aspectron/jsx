//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
var xml = require("xml");
var fs = require("fs");
var json = require("json");

var path = rt.local.scriptPath + "/shortcuts_cfg.xml";
var data = fs.readFile(path);
if(!data)
    throw "unable to read file" + path;
var shortcut_xml = data.toString();

function generate_shortcut_list(str)
{
    var parser = new xml.Parser();
    
    var root = {};
    var current_category = {};
    var group;

    function digest_shortcut_attributes(attr)
    {
        var o = 
        {
            shortcut_type : attr["shortcut_type"],
            state_type : attr["state_type"]
        };
        
        if(attr["min"])
            o.min = attr["min"];
        if(attr["max"])
            o.max = attr["max"];
            
        return o;
    }

    parser.start_node = function(node, attr)
    {
        if(node == "category")
        {
            var category_name = attr["name"];
            current_category = {};
            root[category_name] = current_category;
        }
        else
        if(node == "shortcut")
        {
            var shortcut_name = attr["name"];
            current_category[shortcut_name] = digest_shortcut_attributes(attr);
        }
        else
        if(node == "group")
        {
            group = { prefixes: [], suffixes: {}};        
        }
        else
        if(node == "prefix")
        {
            var prefix_name = attr["name"];
            group.prefixes.push(prefix_name);
        }
        else
        if(node == "suffix")
        {
            var suffix_name = attr["name"];
            group.suffixes[suffix_name] = digest_shortcut_attributes(attr);
        }
        
    }

    parser.end_node = function(node)
    {
//    console.log("STOPPING NODE: "+node);
        if(node == "group")
        {
            for(var i = 0; i < group.prefixes.length; i++)
            {
                var prefix = group.prefixes[i];
                for(var suffix in group.suffixes)
                {
                    var suffix_object = group.suffixes[suffix];
                    var shortcut_name = prefix+suffix;
                    current_category[shortcut_name] = suffix_object;
                }
            }
            
            group = null;
        }
    }
    
    parser.parse(str, true);
    
    return root;
}

var ts1 = getTimestamp();
var obj = generate_shortcut_list(shortcut_xml);
var ts2 = getTimestamp();

console.log("TIME IT TOOK: %f ms", ts2-ts1);

var xmlar = [
             "<root a='b' c='d'><sub \n",
             "name='child'>subnode ",
             "text</sub></root>"
             ];

var banner = {
    start:'>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>',
      end:'<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<'
        };

function testOne()
{
    console.log(banner.start);
    console.log( arguments.callee.name+"()" );
    var ex = new xml.Parser();
    ex.start_node = function start_node(name,attr)
    {
        console.log("start_node:",name);
        for( var k in attr )
        {
            console.log("\tattr ["+k+"] =",attr[k]);
        }
    };

    ex.end_node = function end_node(name)
    {
        console.log("end_node:",name,'userData =');
    };

    ex.char_handler = function char_handler(text,len)
    {
        console.log("char_handler: ["+text+"]");
    };
    //ex.char_handler = "should disable char_handler";
    ex.userData = "hi, world!";
    console.log('ex =',ex);
    var rc;
    if(0)
    { // parse all in one go
        rc = ex.parse( xmlar.join(""), true );
    }
    else
    { // parse incrementally
        for( var i = 0; i < xmlar.length; ++i )
        {
            rc = ex.parse( xmlar[i], i == (xmlar.length-1));
        }
    }
    console.log( "parse rc =",rc);
//    ex.destroy();
    console.log(banner.end);
};


function testTwo()
{
    var x = // some ancient libs11n test code...
        "<!DOCTYPE s11n::io::expat_serializer>"
        +"<somenode class='NoClass'>"
 	+"<a>()\\b</a>"
 	+"<foo>bar</foo>"
 	+"<empty />"
 	+"<long>this is a long property</long>"
 	+"<dino class='DinoClass' />"
 	+"<fred class='FredClass'>"
        +"<key>value</key>"
 	+"</fred>"
 	+"<wilma class='WilmaClass'>"
        +"<the>lovely wife</the>"
 	+"</wilma>"
 	+"<betty class='BettyClass'>"
        +"<value>the apple of Barney&apos;s eye</value>"
 	+"</betty>"
 	+"<deep class='Foo'>"
        +"<deeper class='Foo'>"
        +"<how_deep>really deep!</how_deep>"
        +"<and_deeper class='Ouch'>"
        +"</and_deeper>"
        +"</deeper>"
 	+"</deep>"
        +"</somenode>"
        ;
    x = xmlar.join("");
    trace(banner.start);
    trace( arguments.callee.name+"()" );
    var xo = xml.toObject(x);
    trace('xml.toObject ==',JSON.stringify(xo,undefined,4));
    trace(banner.end);
}
testTwo()
testOne();
