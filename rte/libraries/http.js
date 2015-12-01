//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

//
//  HTTP library wrapper for PION server & client
//
//  PLEASE NOTE - current version of the client [2011/09/20] follows 302/REDIRECT and causes client to
//  establish a permanent connection with a redirected server.  This may not be a desired behavior.
//  TODO - enable/disable this feature via options?
//

var http = (function()
{
    var http = rt.bindings.http;

    var json = require("json");
    var log = require("log");

    http.Websocket.prototype.dispatch = function(msg)
    {
        if(typeof(msg) != 'string')
            http.Websocket.prototype.send.call(this,json.to_string(msg));
        else
            http.Websocket.prototype.send.call(this, msg);
    }

    http.__client__ = http.Client;

    http.Client = function()
    {
        var self = this;

        self.options = 
        {
            default_retries : 3,
            verbose : false
        }

        self.client = new http.__client__();
        self.prototype = self.client;


        // by default, each http connection is keep-alive
        self.client.setKeepAlive(true);

//         self.clear = function()
//         {
//             self.client.clear();
//         }

//        self.connect_requested = false;

        // accepts address in the following format: https://user:pass@mail.google.com
        // http/https determines SSL use; user:pass supplies optional authentication
        self.connect = function(address, retries)
        {
 //           self.connect_requested = true;
            self.client.connect(address, retries || self.options.default_retries);
            return self;
        }

//         self.header = function(arg, v)
//         {
//             if(typeof(arg) == 'object')
//             {
//                 for(var name in arg)
//                     self.client.header(name, arg[name]);
//             }
//             else
//             if(typeof (arg) == 'string' && typeof (v) == 'string')
//             {
//                 self.client.header(arg,v);                
//             }
//             else
//                 throw new Error("http_client::header() requires object or name/value string pair as argument");
// 
//             return self;
//         }
// 
//         self.get = function(resource, query)
//         {
//             // self.client.clear();
// 
//             if(typeof(query) == 'string' )
//                 self.client.set_query_string(query);
//             else
//             if(typeof(query) == 'object')
//             {
//                 for(name in query)
//                     self.client.add_query(name, query[name]);
// 
//                 self.client.use_query_params_for_query_string();
//             }
// 
//             if(typeof(resource) == 'undefined')
//                 resource = "/";
// 
//             // handle url redirect (302)
//             // WARNING! in this implementation, following a redirection switches client connection to the redirected host!
//             // ... I am not sure this is correct behavior...
//             // also.. not sure how to handle possible redirect for POST if it has a query string in the URL...
//             var status = status = self.client.get(resource, self.options.default_retries);
//             if(status == 302)
//             {
//                 var redirect_location = self.client.get_response_header("Location");
// 
//                 if(redirect_location.length)
//                 {
//                     if(self.options.verbose)
//                         log.warn("http client redirecting to: "+redirect_location);
// 
//                     var target = new url(redirect_location);
//                     var target_url = target.get_with_empty_path();
//                     var target_path = target.path();
//                     var target_query = target.query();
// 
//                     // recurse into ourselves, changing connectivity url
//                     return self.connect(target_url).get(target_path, target_query);
//                 }
//             }
//             
//             return status;
//         }
// 
//         self.post = function(path, query)
//         {
//             // self.client.clear();
// 
//             if (typeof (query) == 'string')
//                 self.client.set_post_content(query);
//             else
//             if (typeof (query) == 'object')
//             {
//                 for (name in query)
//                 {
//                     self.client.add_query(name, query[name]);
//                     //trace("POST: "+name+" => "+query[name]);
//                 }
// 
//                 self.client.use_query_params_for_post_content();
//             }
//             
//             if(typeof(resource) == 'undefined')
//                 resource = "/";
// 
//             var status = self.client.post(path, self.options.default_retries);
//             if (status == 302)
//             {
//                 var redirect_location = self.client.get_response_header("Location");
//                 var redirect_location = self.client.get_response_header("Location");
// 
//                 if(redirect_location.length)
//                 {
//                     if(self.options.verbose)
//                         log.warn("http client redirecting to: " + redirect_location);
// 
//                     var target = new url(redirect_location);
//                     var target_url = target.get_with_empty_path();
//                     var target_path = target.path();
//                     var target_query = target.query();
// 
//                     // recurse into ourselves, changing connectivity url
//                     return self.connect(target_url).post(target_path, target_query);
//                 }
//             }
//             
//             return status;
//         }

//         self.content_length = function()
//         {
//             return self.client.content_length();
//         }
// 
//         self.content = function()
//         {
//             return self.client.content_as_buffer();
//         }

        self.ajax = function(o)
        {
            // by default, all requests are GET

            // log.trace(o);

            if (typeof (o.get) == 'object')
            {
                o.type = "GET";
                o.data = http.make_query_string(o.get);
            }
            else
            if (typeof (o.get) == 'string')
            {
                o.type = "GET";
                o.data = o.get;
            }
            else
            if (typeof(o.post) == 'object') {
                o.type = "POST";
                o.data = http.make_query_string(o.post);
            }
            else
            if(typeof(o.post) == 'string')
            {
                o.type = "POST";
                o.data = o.post;
            }
//             else
//             if(typeof(o.data) == 'object')
//             {
//                 if((typeof(o.type) != 'undefined' && o.type == "POST"))
//                 {
//                     o.data = http.make_query_string(o.data);
//                 }
//                 else
//                     o.data = json.to_string(o.data);                
//             }
                
            if(typeof(o.headers) == 'undefined')
                o.headers = 
                {
                    "Cache-Control": "max-age=0",
                    "Accept-Encoding": "gzip,deflate",
                    "Accept": "text/html,application/xhtml+xml,application/xml",
                    "Accept-Charset": "ISO-8859-1,utf-8",
                    "User-Agent": "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/14.0.835.202 Safari/535.1",
                    "Accept-Language": "en-US",
                    "Connection": "Keep-Alive"
                }

//             log.debug(o);

//             if(!self.connect_requested)
//                 self.connect(o.url);

            self.client.ajax(o);

            return self;
        }
        
//         self.get = function(_path)
//         {
//             self.ajax({
//                 method : "GET",
//                 url : _path,
//                 async : 
//                 
//             });
//         }

        return self;
    }

    return http;

})();

exports.$ = http;