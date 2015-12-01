//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
pragma("event-queue");

require("scripts/extern/base64.js")
require("scripts/extern/json.js")
require("scripts/extern/utils.js")

// require("scripts/process.js")


var logger = new library("log");
var log = new logger.instance();

// log.trace("trace");
// log.debug("debug");
// log.info("info");
// log.warn("warning");
// log.error("error");
// log.panic("panic");



var fs = new library("fs");




if(0)
{


var interfaces = {};

interfaces.http = 
{
    server_start : { noblock : true },
    server_stop : {}
};

var module = 
{
    dispatch : function(args, opts)
    {
        trace(json.to_string(opts));
        trace(json.to_string(args));
    }
};

function build_interfaces(module, iface)
{
//    var iface = {};
    
    for(var i in iface)
    {
    
        trace("processing: "+i);
    
        module[i] = function(args) 
        {
            args.instr = i.toString();
            module.dispatch(args, iface[i]);
            
//             var name=/\W*function\s+([\w\$]+)\(/.exec(fn);
//             if(!name)
//                 throw "interface digest error";
//             
//             trace(name[1]);

        }
    }
}


build_interfaces(module,interfaces.http);

module.server_start({alpha:"hello world"});
module.server_stop({beta:"goodbye world"});

rt.halt();

}

/*
 *	
 
 TODO - Implement module objects that get registered with the system and activate the current module...
 
 
 */
 
 /*
 
 var modules = {};
 
 modules.main = 
 {
    value : 12345,
 
    run : function()
    {
        trace(this.value+"\n");
        
//        modules.alpha
    },
 
//     custom_event_1 : function()
//     {
//         var socket = zmq_create_socket("tcp://localhost", function(e){
//         
//         // messages?
//         
//         });
//         
//     }
 
 };
 
 modules.main.run();

 trace("HELLO V8 WORLD!!!\n");


//////////////////////////////////////////////////////////////////////////
// 
// var a = new zmq_context_t(4);
// 
// trace(a);
// 
// a = null;

//var b = new Buffer(5);
var b = new Buffer("hello world...");

//b.test("hello world");

trace("### buffer length is "+b.length);
//b.set(0,123);
//var c = b.get(0);

trace_buffer(b);

b[0] = 65;
b[1] = 66;
b[2] = 67;

// this becomes a property 
b[100] = 123;


var c = b[1];
trace("c is "+c+"\n");

trace_buffer(b);

trace("done...\n");

*/

//trace("zmq.socket is: ")
//trace(JSON.stringify(this)+"\n");

//trace("zmq.socket is: "+zmq.socket);

//zmq_socket.prototype.ttt = function() { trace("foobar"); }

//var xxx = new Buffer("yay");

// trace("STARTING\n");
// var alpha = new zmq_socket(7);
// 
// //var aa = new zmq_socket(7);//ZMQ_PULL);
// trace("DONE\n");

if(0)
{
    var bb = null;
    for(var i = 0; i < 1024; i++)
    {
      bb = new Buffer(1024*1024);
      trace(".");
    }
}
/*
var aa = new zmq_socket(ZMQ_PULL);
var bb = new zmq_socket(ZMQ_PUSH);
aa.bind("inproc://xtest");
bb.connect("inproc://xtest");

aa.digest(null,function(buffer)
{
    var str = buffer.to_string();
    trace("received:"+str+"\n");
});

//bb.digest(null,function(){});

bb.send("hello world one", 0); //ZMQ_NOBLOCK);
bb.send("hello world two", 0); //ZMQ_NOBLOCK);
bb.send("hello world three", 0); //ZMQ_NOBLOCK);
bb.send("hello world four", 0); //ZMQ_NOBLOCK);
*/

// TODO - CHECK THAT DIGEST CAN NOT BE CALLED ON PUSH SOCKETS !!!
// TODO - CHECK THAT DIGEST CAN NOT BE CALLED ON PUSH SOCKETS !!!
// TODO - CHECK THAT DIGEST CAN NOT BE CALLED ON PUSH SOCKETS !!!
// TODO - CHECK THAT DIGEST CAN NOT BE CALLED ON PUSH SOCKETS !!!



// trace(uuid()+"\n");
// trace(uuid()+"\n");
// trace(uuid()+"\n");
// trace(uuid("abc")+"\n");
// trace(uuid("abc")+"\n");
// trace(uuid("abc")+"\n");

/*
var aspect = 
{
    libraries : {}
};


aspect.libraries.process = new library("process");
var pi = new aspect.libraries.process.prociface();

if(1)
{
    var list = pi.list();
    //trace(list);
    for(var i = 0; i < list.length; i++)
    {
        var info = pi.get_info(list[i]);
        trace(info.name);
        trace(info.cmd_line);
    }

    // test - create N calcs and kill them after 3 seconds...

    var calcs = [];

    for(var i = 0; i < 6; i++)
        calcs.push(pi.start("calc.exe", pi.PRIORITY_NORMAL));
        
    trace("sleeping...");
    sleep(3000);

    for(var i = 0; i < calcs.length; i++)
        pi.kill(calcs[i]);
        
    trace("done...");


    //trace("priorities: "+JSON.stringify(pi.PRIORITY_NORMAL)+"\n");

    //trace("priorities: "+JSON.stringify(aspect.libraries.process)+"\n");
}
*/

/*
var line = "c:\\aaa bbb\\ccc \\hx86.exe --parent 012345678  --module main.js --uuid 36095f4e-b9b7-5222-bd06-59babf019d12 alpha   betta gamma extra 123";

var args = rtl.libraries.process.digest_arguments(line);
trace("-------");
trace(JSON.stringify(args));
trace("-------");
trace("-------");

function abc_def(a,b,c)
{
//    trace(json.to_string(arguments.callee));
    trace(arguments.callee);
}

abc_def(1,2,3);

// rt.halt();
*/

if(0)
{

var refresh_frequency = 0;

var ts_delta = 0.0;
var ts_last = getTimestamp();
var requests_per_second_acc = 0;
var requests_per_second = 0;

var counter = 0;

var http = require("http");

var http_server = new http.Server(1234);
http_server.start();

// filesystem handler
//var handler_fs = new http.service_handler_fs();
http_server.digest("/root", new http.FilesystemService());

http_server.digest("/", function(request, writer) {

writer.write("<html><head><meta http-equiv='refresh' content='"+refresh_frequency+";'/></head><body> hello web browser world! :)");
writer.write("<p/>Counter value is: "+counter+"<p/>");
writer.write("Current TS: " + getTimestamp() + "<p/>");  // todo - make this take a string directly
writer.write("Requests / Sec: " + requests_per_second + "<p/>");  // todo - make this take a string directly
writer.write("T Per Request: " + 1.0/requests_per_second + "sec <p/>");  // todo - make this take a string directly

var info = json.to_string(request);
info = info.split(',');
info = info.join(',<br/>')
writer.write(info);

// writer.write("<p/>");
// info = json.to_string(hrt.process.enumerate_hx_processes());
// writer.write(info);

writer.write("</body></html>");
writer.send();

counter++;

requests_per_second_acc++;
var ts = getTimestamp();
if(ts - ts_last > 1.0)
{
    requests_per_second = requests_per_second_acc;
    requests_per_second_acc = 0;
    trace("requests per second: " + requests_per_second + "\n");
    ts_last = ts;
}

});


trace("\n\nPLEASE POINT YOUR BROWSER TO http://localhost:1234\n\n");

}


if(0)
{

var t1 = new timer(1000,function()
{ 

var d = new Date();
trace("tick - "+d.toString());

var ts0 = getTimestamp();
for (var i = 0; i < 5; i++)
    dpc(function() 
    {

        var ts1 = getTimestamp();

        trace("TS = " + (ts1 - ts0));

    });

});

}


if(0)
{

//var uuid_gen = new uuid_generator("{6ba7b810-9dad-11d1-80b4-00c04fd430c8}");
var uuid_gen = new uuid_generator("{00000000-0000-0000-0000-000000000000}");

var suid = get_unique_local_system_identifier();
var system_uuid = uuid_gen.name(suid);

trace("suid: " + suid + " sys uuid: "+system_uuid);

trace(uuid_gen.name("alpha"));
trace(uuid_gen.name("alpha"));
trace(uuid_gen.name("alpha"));
trace(uuid_gen.name("xalpha"));
trace(uuid_gen.name("yalpha"));
trace(uuid_gen.name("zalpha"));

trace("---");
for(var i = 0; i < 10; i++)
trace(uuid_gen.random());

}

// var application

var utils = 
{
    generic_uuid_generator : new uuid_generator("{00000000-0000-0000-0000-000000000000}"),
};


var rt = {};

rt.local = {};
rt.local.uuid = utils.generic_uuid_generator.name(get_unique_local_system_identifier());
rt.local.uuid_generator = new uuid_generator(rt.local.uuid);

rt.modules
{

};

rt.applications = 
{
    list : {},
    
    register : function(info)
    {
        this.list[info.alias] = this.persistent(info);        
    },
    
    persistent : function(info)
    {
        // TODO - retreive from storage
        
        // create application uuid
        info.uuid = rt.local.uuid_generator.name(info.alias);
    
        return info;        
    }
};
// 
// rt.modules = 
// {
//     SHARED : 0,
//     EXTERNAL : 1
// }

// rt.applications.register({
// 
//     alias: "scheduler",
//     name: "Scheduler",
//     modules:
// {
//     "http": { options: "SHARED|EXTERNAL" },
//     "scheduler": { options: "SHARED|EXTERNAL" }
// }
// 
// });

// TODO - 
//          read_json to load application data
//          generate sha1 checksum
//          see if there is available cache
//          load cache if available
//          otherwise process object, create uids and everything needed
//          store cache
//          start processing modules.
//
//      TODO - PUBLISH GLOBAL ZMQ SOCKET IN EACH MODULE!
//          



// trace(extract_file_alias("application.alpha.js","application","js"));

if(0)
{

fs.enumerateDirectory(rt.local.rtePath + "/scripts", function(path){

var filename = path.filename();
trace("filename: "+filename);

if(filename.toLowerCase().indexOf("application.") != -1)
{
    var str = fs.readFile(path).string();  // TODO - REDO USING ONLY BUFFERS - ADD PROPER ESCAPING TO STRINGS!!!
    
    trace("read: "+str);
    
    trace("parsing...");
    
    var obj = eval("("+str+")");

//    var filename = path_string.filename();    
    obj.alias = extract_file_alias(filename,"application","js");
    
    trace("parsed...");
    
    trace(obj.alias);
    rt.applications.register(obj);

    trace("registered...");
}
    
});

trace(json.to_string(rt, null, '\t'));
}


/*



var application_files = [];

var path = rt.local.rte_path + "/scripts";

fs.enumerateDirectory(path,function(path){

var filename = path.string();

if(filename.toLowerCase().indexOf(".js") > 0)
    application_files.push(filename);

});

for(var i = 0; i < application_files.length; i++)
    trace(application_files[i]);
    
*/ 

if(0)
{

var ret = 0;
    
var responder = zmq.socket(zmq.REP);

log.trace("binding...");
ret = responder.bind("tcp://*:5678");
log.trace("bind - ret: "+ret);

responder.digest(function(msg)
{
    log.trace("RESPONDER RECV: "+msg.string());
    
    responder.send("BACK TO YOU! => "+msg.string(),0);
//    log.trace("RESPONDER SENT REPLY...");
});
    
var requester = zmq.socket(zmq.REQ);

var ret = requester.connect("tcp://localhost:5678");
log.trace("connected - ret: "+ret);


// requester.digest(null,function(msg)
// {
//     log.trace("REQUESTER RECV: "+msg.string());
// });


for(var i = 0; i < 10; i++)
{
    requester.send("HELLO WORLD #"+i,0);//,zmq.NOBLOCK);
    var msg = requester.recv(0);
    
    log.trace("$$$ SEND #"+i);
    log.trace("$$$ RECV => "+msg.string());
}

//  var msg = requester.recv(0);
//  log.trace("REQUESTER RECV: "+msg.string());




}

// var ts1 = getTimestamp();
// 
// dpc(1000,function(){
// var ts2 = getTimestamp();
// log.trace("recv dpc() in "+(ts2-ts1));
//  var msg = requester.recv(0);
//  log.trace("REQUESTER RECV: "+msg.string());
// });



// ===============================================================================


var ping = 
{
    main : function()
    {
        this.t = new timer(5000, function() { log.info("# ping"); }, true, false);        
    }
};

var pong = 
{
    main : function()
    {
        this.t = new timer(5000, function() { log.info("# ping"); }, true, false);    
    }
}

modules.register(ping);
modules.register(pong);
modules.run();



