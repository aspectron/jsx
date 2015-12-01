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
@module db Database
Database native bindings
**/
var db = (function()
{
    var log = require("log");
    var json = require("json");

    var db = rt.bindings.db;

    /**
    @property storageLocation {String} Database storage location
    Used to open SQLite database when no config string is provided for
    #Session constructor.
    Default value is runtime#rt.local.rootPath
    **/
    db.storageLocation = rt.local.rootPath;

    db.Session = function(config)
    {
        if (!config)
            return new db._session();
        else if(config.indexOf("db=") != -1)
            return new db._session(config);
        else
            return new db._session("sqlite3:db='" + db.storageLocation + '/' + config + ".db'");
    }
    
    /**
    @class ObjectStore

    @function ObjectStore(alias)
    @param alias {String}
    @return {ObjectStore}
    Create a new ObjectStore instance for `alias`
    **/
    db.ObjectStore = function(alias)
    {
        var self = this;
        
        self.options = 
        {
            verbose: false
        }
        
        self.alias = alias;
        self.session = new db.Session("sqlite3:db='" + db.database_storage_location + '/' + alias+".db'");
        
        self.session.exec("CREATE TABLE IF NOT EXISTS '"+self.alias+"' (uuid TEXT PRIMARY KEY, object TEXT);");
        
        /**
        @function getList()
        @return {Array}
        Get array of UUIDs stored
        **/
        self.getList = function()
        {
            var select = self.session.prepare("SELECT uuid FROM '"+self.alias+"'");
            var result = select.query();
            
            var out = [];
            while(result.next())
            {
                var uuid = result.value(0);
//                var name = result.value(1);
                
                out.push(uuid);// = name;
            }
            
            return out;
        }
        
        /**
        @function getAll()
        @return {Object}
        Get all contents of the storage
        **/
        self.getAll = function()
        {
            var select = self.session.prepare("SELECT uuid, object FROM '"+self.alias+"'");
            var result = select.query();
            
            var out = {};
            while(result.next())
            {
                var uuid = result.value(0);
//                var name = result.value(1);
                var _o = result.value(1);
                var o = null;
                
                try
                {
                    o = json.parse(_o);
                }
                catch(ex)
                {
                    log.error("Unable to fetch json object from database");
                    log.error(ex);
                }
                
//                 var record = 
//                 {
//                     uuid : uuid,
//                     name : name,
//                     object : o
//                 }
                
                out[uuid] = o; //record;
            }
            
            return out;
        }
        
        /**
        @function get(uuid)
        @param uuid {String}
        @return {Object}
        Get object by UUID
        **/
        self.get = function(uuid)
        {
            var select = self.session.prepare("SELECT object FROM '" + self.alias + "' WHERE uuid='"+uuid+"'");
            var result = select.query();
            if(!result.next())
                return null;
            
            var _o = result.value(0);
            var o = null;

            try 
            {
                o = json.parse(_o);
            }
            catch (ex) 
            {
                log.error("Unable to fetch json object from database");
                log.error(ex);
            }           
            
            return o;                 
        }
        
        /**
        @function set(uuid, o)
        @param uuid {String}
        @param o {Object}
        Store object `o` for `uuid` key
        **/
        self.set = function(uuid, o)
        {
            var _o = json.to_string(o);
            
//             var name = "";
//             if(o.name)
//                 name = o.name;
                    
            var insert = self.session.prepare("INSERT OR REPLACE INTO '"+self.alias+"' (uuid, object) values(?,?)");
            self.session.begin();
            insert.bind(uuid).bind(_o);
            insert.exec();
            self.session.commit();        
        }
        
        /**
        @function erase(uuid)
        @param uuid {String}
        Erase object by `uuid`
        **/
        self.erase = function(uuid)
        {
            var query_str = "DELETE FROM '"+self.alias+"' WHERE uuid=?";
            if (self.options.verbose)
                log.trace(query_str);
            
            var op = self.session.prepare(query_str);
            self.session.begin();
            op.bind(uuid);
            op.exec();
            self.session.commit();
//            self.session.exec("DELETE FROM "+self.alias+" WHERE uuid='"+uuid+"'");
        }
        
        /**
        @function purge()
        Erase all data in the storage
        **/
        self.purge = function()
        {
            var query_str = "DELETE FROM '"+self.alias+"' WHERE 1";
            if(self.options.verbose)
                log.trace(query_str);
            self.session.exec(query_str);
        }
        
        return self;
    }
    
    return db;

})();

exports.$ = db;