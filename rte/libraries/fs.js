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
@module fs Filesystem
Filesystem operations.
**/
var fs = (function()
{
    var log = require("log");
    var json = require("json");

    var fs = rt.bindings.fs;

    function normalize(path)
    {
        return path.replace(/\\/g,"/");
    }

    /**
    @function setSearchPaths(paths)
    **/
    fs.__search_paths = null;
    fs.setSearchPaths = function(paths)
    {
        fs.__search_paths = path;
    }

    /**
    @function setSearchIgnoreFolders(folders)
    **/
    fs.search_ignore_folders_ = [];
    fs.setSearchIgnoreFolders = function(folders)
    {
        fs.search_ignore_folders_ = [];
        folders.forEach(function(folder)
        {
            fs.search_ignore_folders_.push(normalize(folder).toLowerCase()); 
        });
    }

    function matches_search_ignore_folders(path)
    {
        return fs.search_ignore_folders_.indexOf(normalize(path).toLowerCase()) >= 0;
    }

    function extract_file_alias(str,type,ext)
    {
        var reg = new RegExp(type+'.(.*?).'+ext, 'gi');
        str = str.replace(reg,"$1");
        return str;
        
    }
    
    /**
    @function directoryToObject(directory_path, file_filter_fn)
    @param directory_path {String|Path}
    @param file_filter_fn {Function}
    @return {Object}
        **/
    fs.directoryToObject = function(directory_path, file_filter_fn)
    {
        // return null if folder does not exists
        if(!fs.__search_paths && !fs.isDirectory(directory_path))
            return null;

        var root = { files: [], folders: {} };
        var current = root;
        var recursion = [];

        function digest_file(path) 
        {
            //////////////////////////////////////////////////////////////////////////
            //         var dash = "";
            //         for(var i = 0; i < recursion.length; i++)
            //             dash += "--";
            //         log.debug(dash+"> "+path);
            //////////////////////////////////////////////////////////////////////////

            if (fs.isDirectory(path)) 
            {
                recursion.push(current);
                var sub = { files: [], folders: {} };
                current.folders[normalize(path)] = sub;
                current = sub;

                if (file_filter_fn && typeof (file_filter_fn) == 'function')
                    if (!file_filter_fn(normalize(path), true))
                        return;

                fs.enumerateDirectory(path, digest_file);
                current = recursion.pop();
            }
            else 
            {
                // if user provided file filtering callback, return if it returns false
                if(file_filter_fn && typeof(file_filter_fn) == 'function')
                    if(!file_filter_fn(normalize(path), false))
                        return;
                        
                current.files.push(normalize(path));
            }
        }

        digest_file(directory_path, true);
        
        return root;

    }

    /**
    @function locateFiles(directory_path, pattern, exclude_paths)
    @param directory_path {String|Path}
    @param pattern {String|RegExp}
    @param  exclude_paths {Boolean}
    @return {Array}
    **/
    fs.locateFiles = function(directory_path, pattern, exclude_paths)
    {
        // return null if folder does not exists
        if (!fs.isDirectory(directory_path))
            throw new Error("Supplied directory path is not a directory: "+directory_path);

        var files_found = [];

        var is_pattern = (typeof(pattern) != 'string');
        if(!is_pattern)
            pattern = pattern.toLowerCase();

        function digest_file(path)
        {
            if(matches_search_ignore_folders(path))
                return;
//////////////////////////////
            if (fs.isDirectory(path))
                fs.enumerateDirectory(path, digest_file);
            else
            {
//                log.trace(pattern + " -> " + path + " => " + path.filename());

                if(!is_pattern)
                {
                    if(path.filename().toLowerCase() == pattern)
                    {
                        files_found.push(normalize(exclude_paths? path.filename() : path));
                    }
                }
                else
                {
                    if(path.filename().toLowerCase().match(pattern))
                    {
                        files_found.push(normalize(exclude_paths? path.filename() : path));
                    }
                }
            }
        }

        digest_file(directory_path);
//        log.trace(files_found);
        return files_found;
    }



//     fs.enumerate_directory_recursive = function(directory_path, file_filter_fn) 
//     {
//         // return null if folder does not exists
//         if (!fs.isDirectory(directory_path))
//             return null;
// 
//         var files = [];
// 
//         var root = { files: [], folders: {} };
//         var current = root;
//         var recursion = [];
// 
//         function digest_file(path) {
//             //////////////////////////////////////////////////////////////////////////
//             //         var dash = "";
//             //         for(var i = 0; i < recursion.length; i++)
//             //             dash += "--";
//             //         log.debug(dash+"> "+path);
//             //////////////////////////////////////////////////////////////////////////
// 
//             if (fs.isDirectory(path)) {
//                 recursion.push(current);
//                 var sub = { files: [], folders: {} };
//                 current.folders[path] = sub;
//                 current = sub;
//                 fs.enumerateDirectory(path, digest_file);
//                 current = recursion.pop();
//             }
//             else {
//                 // if user provided file filtering callback, return if it returns false
//                 if (file_filter_fn && typeof (file_filter_fn) == 'function')
//                     if (!file_filter_fn(path))
//                     return;
// 
//                 files.push(path);
//             }
//         }
// 
//         digest_file(directory_path);
// 
//         return files;
//     }

        

    /**
    @function directoryToTreeStore(base_scan_path, file_object_fn)
    @param base_scan_path {String|Path}
    @param file_object_fn {Function}
    @return  {Object}
    **/
    fs.directoryToTreeStore = function(base_scan_path, file_object_fn)
    {
        // return null if folder does not exists
        if(!fs.isDirectory(base_scan_path))
            return null;
            
        if(!file_object_fn || typeof(file_object_fn) != 'function')
            file_object_fn = function(path, is_folder, base_path) 
            { 
                return { name: path.stem(), real_path: path };
            } 
            

        var root_path = new fs.Path(base_scan_path);
        var root = file_object_fn(root_path, true, base_scan_path);
//        var root = { name : root_path.stem(), real_path: base_scan_path };
        var current = root;
        var recursion = [];

        function dump_recursion()
        {
            for(var i = 0; i < recursion.length; i++)
                log.trace(recursion[i].real_path);
        }

        function digest_file(path) 
        {
        
//        log.debug("called with path: "+path);
            //////////////////////////////////////////////////////////////////////////
            //         var dash = "";
            //         for(var i = 0; i < recursion.length; i++)
            //             dash += "--";
            //         log.debug(dash+"> "+path);
            //////////////////////////////////////////////////////////////////////////
            
            if(path.stem() == ".")
            {
                if(current.real_path == root.real_path)
                    fs.enumerateDirectory(path, digest_file);
            }
            else
            if (fs.isDirectory(path))// || path.stem() == ".") 
            {

                var folder = file_object_fn(path, true, base_scan_path); //{ name: path.stem(), real_path: path  };
                if (!folder)
                    return;

                recursion.push(current);
                if(!current.children)
                    current.children = [];
                current.children.push(folder);
                current = folder;
                current.children = [];
                fs.enumerateDirectory(path, digest_file);
                current = recursion.pop();
            }
            else 
            {
                var file = file_object_fn(path, false, base_scan_path);
                if(!file)
                    return;
            
                if(!current.children)
                    current.children = [];
                current.children.push(file);
            }
        }
        
        log.info(root);

        digest_file(new fs.path(base_scan_path));
        
        return root;

    }


    /**
    @class FileStore

    @function FileStore(base_path, file_object_fn)
    @param base_path {String|Path}
    @param file_object_fn {Function}
    **/
    fs.FileStore = function(base_path, file_object_fn)
    {
        if(!fs.isDirectory(base_path))
        {
            log.error("unable to init data store: '"+base_path+"' is not a valid directory");
            throw new Error("unable to init data store: '"+base_path+"' is not a valid directory");
        }
    
        var self = this;
        
        self.base_path = base_path;                  // root path on which we will operate
        self.file_object_fn = file_object_fn;   // file filter callback (allows user to filter files by rejection)
        self.storage_cache = {};                // storage of objects by uuid
        
        /**
        @function path(relative_path)
        @param relative_path {String}
        @return {String}
        return FileStore `base_base/relative_path`
        **/
        function path(relative_path)
        {
            return self.base_path + "/" + relative_path;
        }
        
        /**
        @function readAll()
        @return {Object}
        **/
        self.readAll = function()
        {
            // TODO - MAKE THIS COMPATIBLE WITH ExtJS TREE STORE?
            var root_object = fs.directory_to_tree_store(self.base_path, self.file_object_fn)
            return root_object;
        }
        
        /**
        @function read(relative_path)
        @param relative_path {String}
        @return {String}
        **/
        self.read = function(relative_path)
        {
            // TODO - GET SCRIPT TEXT
            
            // return cache if available
            if(self.storage_cache[relative_path])
                return self.storage_cache[relative_path];
                
            try
            {
                var contents = fs.readFile(path(relative_path));
                if(!contents)
                    throw new Error("file is 'null'");
                self.storage_cache[relative_path] = contents.string();
                return self.storage_cache[relative_path];
            }
            catch(ex)
            {
                log.error("Error reading file: "+path(relative_path));
                log.error(ex);
            }
            
            return null;
        }
        
        /**
        @function write(relative_path, file_contents)
        @param relative_path {String}
        @param file_contents {String}
        **/
        self.write = function(relative_path, file_contents)
        {
            // TODO - store o as text file - file_contents must be text string (file contents)
            self.storage_cache[relative_path] = file_contents;
            
            log.trace("fs file store - storing to path: "+path(relative_path));
            
            fs.writeFile(path(relative_path), file_contents);
        }


        /**
        @function copy(source_file, new_file)
        @param source_file {String}
        @param new_file {String}
        **/
        self.copy = function(source_file,new_file){

                var content = self.get(source_file);
                self.set(new_file,content);

        }
        
        /**
        @function destroy(relative_path)
        @param relative_path {String}
        **/
        self.destroy = function(relative_path)
        {
            // TODO - delete file or folder with files recursively
            delete self.storage_cache(relative_path);
        }
        
        /**
        @function purge()
        **/
        self.purge = function()
        {
            // WARNING! - BE CAREFUL WITH THIS FUNCTION!
            // ?? - TODO - TEST: DOES THIS REMOVE THE BASE FOLDER AS WELL?
            fs.removeAll(self.base_path);
        }
        
        /**
        @function isDirectory(relative_path)
        @param relative_path {String}
        **/
        self.isDirectory = function(relative_path)
        {
            return fs.isDirectory(path(relative_path));
        }
        
        /**
        @function createDirectory(relative_path)
        @param relative_path {String}
        **/
        self.createDirectory = function(relative_path)
        {
            fs.createDirectory(path(relative_path));
        }
        
        /**
        @function createDirectoryRecursive(relative_path)
        @param relative_path {String}
        **/
        self.createDirectoryRecursive = function(relative_path)
        {
            return fs.createDirectory(path(relative_path), true);
        }

        /**
        @function rename(relative_path_src, relative_path_dst)
        @param relative_path_src {String}
        @param relative_path_dst {String} 
        **/
        self.rename = function(relative_path_src, relative_path_dst)
        {
            fs.rename(path(relative_path_src), path(relative_path_dst));
        }
        
        /**
        @function remove(relative_path)
        @param relative_path {String}
        **/
        self.remove = function(relative_path)
        {
            fs.remove(path(relative_path));
        }

        /**
        @function removeAll(relative_path)
        @param relative_path {String}
        **/
        self.removeAll = function(relative_path) {
            fs.remove(path(relative_path), true);
        }
        
        /**
        @function copyFile(relative_path_src, relative_path_dst)
        @param relative_path_src {String}
        @param relative_path_dst {String}
        **/
        self.copyFile = function(relative_path_src, relative_path_dst)
        {
            fs.copy_file(path(relative_path_src),path(relative_path_dst));
        }
        
        self.rpc_digest = function(msg)
        {
            if(!msg.request.match(/filestore/ig))
                return false;
                
            var fn = msg.request.substring(10);
            if(self[fn] && typeof(self[fn]) == 'function')
            {
                try
                {
                    self[fn](msg.arg || msg.arg1, msg.arg2);
                    this.send(null);
                }
                catch(ex)
                {
                    this.send({error : ex.message, exception : ex});
                }
                
                return true;
            }
            
            return false;                       
        }
    
        return self;
    }

    /**
    @module fs
    **/
    fs.createDirectoryRecursive = function(path)
    {
        fs.createDirectory(path, true);
    }

    /**
    @function readJSON(path)
    @param path {String|Path}
    @return {Object}
    **/
    fs.readJSON = function(path)
    {
        var buf = fs.readFile(path);
        if(!buf) return null;
        var str = buf.string();
        
        try
        {
            var o = eval("("+str+")");
            return o;
        }
        catch(ex)
        {
            log.error(ex);
        }
        
        return null;        
    }

    /**
    @function writeJSON(path, o)
    @param path {String|Path}
    @param o {Object}
    **/
    fs.writeJSON = function(path, o)
    {        
        fs.writeFile(path,json.to_string(o,null,"\t"));
    }

    /**
    @function readConfig(filename)
    @param filename {String}
    @return {Object}
    Read configuration file from runtime#rt.local.rootPath`/config/filename.cfg`
    **/
    fs.readConfig = function(filename)
    {
        if(filename.indexOf('.') == -1) filename += '.cfg';
        return fs.readJSON(rt.local.rootPath + '/config/' + filename);
    }

    /**
    @function readRteConfig(filename)
    @param filename {String}
    @return {Object}
    Read configuration file from runtime#rt.local.rtePath`/filename.cfg`
    **/
    fs.readRteConfig = function(filename)
    {
        if(filename.indexOf('.') == -1) filename += '.cfg';
        return fs.readJSON(rt.local.rtePath + '/' + filename);
    }

    /**
    @function locateConfigFile(filename)
    @param filename {String}
    @return {String}
    Locate configuration file starting from runtime#rt.local.scriptPath
    **/
    fs.locateConfigFile = function(filename)
    {
        if(filename.indexOf('.') == -1) filename += '.cfg';
        var p = rt.local.scriptPath.replace(/\\/g,'/').split('/');
        var cfg_file = '';
        var found = false;
        while(!found && p.length)
        {
            cfg_file = p.join('/') + '/config/'+filename;
            if(fs.exists(cfg_file))
                return cfg_file;
            p.pop();
        }

        return null;
    }

    return fs;

})();

// export(fs);

exports.$ = fs;