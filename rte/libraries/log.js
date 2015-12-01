//
// Copyright (c) 2011 - 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of JSX (https://github.com/aspectron/jsx) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//

var log = (function()
{
    var runtime = global.rt;
    if (runtime.logger_)
        return runtime.logger_;

    var json = require("json");

    var dumpof = function(jsobj, jsdescr, shift) 
    {
		var MAX_SHIFT = new String('                                                                                                  ');
		
		var cur_shift = shift || (jsdescr ? 1 : 0)
			,key_shift = MAX_SHIFT.substring(0, cur_shift) 
		;
		if (typeof jsobj == 'string') {
			log.trace('\nDUMP OF \'' + jsdescr + '\' : \'' + jsobj + '\'');
		} else {
			if (jsdescr) {
				log.trace('\nDUMP OF \'' + jsdescr + '\' >>>>>>>>');
			}		
			if (jsobj instanceof Array){
	//			log.trace(key_shift + '[');
				for (var i = 0; i < jsobj.length; i++) {
					log.trace (key_shift + i + ': ');
					this.dumpof(jsobj[i], null, cur_shift + 1);
    			}
	//			log.trace(key_shift + ']');
			} else {
				for (var key in jsobj) {
					var brace_shift = MAX_SHIFT.substring(0, cur_shift + key.length);
						
					if (typeof jsobj[key] == 'object') {
						var isArray = jsobj[key] instanceof Array;
	
						log.trace (key_shift + key + ' : '+ (isArray ? '[':'{') );
						this.dumpof(jsobj[key], null, cur_shift + 1);
						log.trace (key_shift + (isArray ? ']':'}') );
					} else {
						log.trace (key_shift + key + ' :' + (typeof jsobj[key] == 'null' ? '[null]' : jsobj[key]));
					}
	        
				}
			}
			if (jsdescr) {
				log.trace('<<<<<<<< ' + jsdescr);
    		}		
		}
	}
	

    // Log type is split into two 16bit words.
    // XXYYYYYY where X are flags and Y is type
    // To obtain type do: level & 0xffff
    
    // Please note that all internal HTTP server messages originated form PION are enforced to be local

    var LOG_LEVEL_TRACE     = 0;
    var LOG_LEVEL_DEBUG     = 1;
    var LOG_LEVEL_INFO      = 2;
    var LOG_LEVEL_ALERT     = 3;
    var LOG_LEVEL_WARNING   = 4;
    var LOG_LEVEL_ERROR     = 5;
    var LOG_LEVEL_PANIC     = 6;
    var LOG_LEVEL_NOTIFY    = 7;
    var LOG_LEVEL_EXECUTE   = 8;
    
    var FLAG_USER    = 0x00010000;
    var FLAG_LOCAL   = 0x00020000;
    var FLAG_REMOTE  = 0x00040000;
    var FLAG_OBJECT  = 0x01000000;

    if(!rt.bindings.logger.prototype.log_impl_)
    {
        rt.bindings.logger.prototype.log_impl_ = function(level, args)
        {
            var ts = "";//Date().today().toString("yyyy-MM-dd HH:mm:ss")+"  ";

            if(this.user_mode || (args.length > 1 && args[0] === true))
                level |= FLAG_USER;
                
            var msg = args.length > 1 ? args[1] : args[0];
        
            // this.output(LOG_LEVEL_DEBUG,"--- log processing object of type: "+typeof(msg));

            if(msg === null)
            {
                this.output(level, msg);
            }
            else
            if(typeof(msg) == 'object')
            {
                this.output(level | FLAG_OBJECT | FLAG_REMOTE, json.to_string(msg));
                
                if(msg.stack && msg.message)
                {
                    this.output(LOG_LEVEL_ERROR | FLAG_LOCAL,msg.message);
                    this.output(LOG_LEVEL_DEBUG | FLAG_LOCAL,msg.stack);
                }
                else
                    this.output(level | FLAG_LOCAL, json.to_string(msg,null,"\t"));
//                     this.output(level | FLAG_LOCAL,json.to_string(msg,function(key,obj) 
//                     {
//                         if(typeof(obj) == 'function')
//                             return obj.toString();
//                         if(typeof(obj) == 'undefined')
//                             return ""+obj;
//                         return obj;
//                     },"\t"));
            }
            else
                this.output(level,ts + msg);

/*            if (level == LOG_LEVEL_TRACE)
            {
                var err = new Error;
                err.name = 'Trace';
                err.message = '^^^^^';
                Error.captureStackTrace(err, arguments.callee);
                this.output(level, err.stack);
            }
*/
        }

        rt.bindings.logger.prototype.trace = function() { this.log_impl_(LOG_LEVEL_TRACE, arguments); }
        rt.bindings.logger.prototype.debug = function() { this.log_impl_(LOG_LEVEL_DEBUG, arguments); }
        rt.bindings.logger.prototype.info = function() { this.log_impl_(LOG_LEVEL_INFO, arguments); }
        rt.bindings.logger.prototype.warn = function() { this.log_impl_(LOG_LEVEL_WARNING, arguments); }
        rt.bindings.logger.prototype.alert = function() { this.log_impl_(LOG_LEVEL_ALERT, arguments); }
        rt.bindings.logger.prototype.error = function() { this.log_impl_(LOG_LEVEL_ERROR, arguments); }
        rt.bindings.logger.prototype.panic = function() { this.log_impl_(LOG_LEVEL_PANIC, arguments); }
        rt.bindings.logger.prototype.notify = function() { this.log_impl_(LOG_LEVEL_NOTIFY, arguments); }
        rt.bindings.logger.prototype.execute = function() { this.log_impl_(LOG_LEVEL_EXECUTE, arguments); }

        rt.bindings.logger.prototype.warning = function() { this.log_impl_(LOG_LEVEL_WARNING, arguments); }
        rt.bindings.logger.prototype.fatal = function() { this.log_impl_(LOG_LEVEL_PANIC, arguments); }

        rt.bindings.logger.prototype.set_user_mode = function(mode)
        {
            this.user_mode = mode;
        }
    }

    rt.bindings.logger.prototype.dumpof = dumpof;

    var log = new rt.bindings.logger();
    
    log.user_mode = false;

    runtime.logger_ = log;
		
    return log;
    


})();

exports.$ = log;