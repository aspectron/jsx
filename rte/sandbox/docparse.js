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

var log = require("log"),
    fs = require("fs"),
    os = require("os"),
    json = require("json"),
    util = require("util"),
    _ = require('underscore');

require('rte');
var runtime = global.rt;

var Docparse = function() {
	var self = this;
	base_path = runtime.local.rtePath;
	root_path = base_path;

	self.process_annotations = function(anns) {
		var result = {
			library: {}
		};
		_.each(anns, function(a) {			
			var library = {}
			if(!result.library[a.contents.library.name])
				result.library[a.contents.library.name] = library;			
			else
				library = result.library[a.contents.library.name];
			_.extend(library, a.contents.library);
		});
		return result;
	}

	self.process_annotations_file = function(anns, f) {		
		var context={}, result = {
			file: f,
			contents: {}
		};		
		function error_exit(msg,l) {
			log.error(f+'('+l+'): '+msg);
			return result;
		}
		var annotations = {
			'library': {
				single: true,
				contained_in: ['file'],
				contains: ['since', 'author']
			},
			'constant': {		
				contained_in: [],
				contains: ['since', 'author']
			},
			'class': {			
				contained_in: [],	
				contains: ['since', 'author', 'attribute']
			},
			'inherits': {
				single: true,
				contained_in: ['class'],
				contains: []					
			},
			'attribute': {
				contained_in: ['library', 'class'],
				contains: []
			},
			'function': {				
				contains: ['param', 'since', 'author', 'returns']
			},
			'param': {
				contained_in: ['function'],
				contains: []					
			},
			'returns': {
				single: true,
				name_is_description: true,
				contained_in: ['function'],
				contains: []
			},
			'since': {
				single: true,
				name_is_description: true,
				contained_in: ['library', 'constant', 'class', 'attribute', 'function'],
				contains: []
			},
			'author': {
				single: true,
				name_is_description: true,
				contained_in: ['library', 'constant', 'class', 'attribute', 'function'],
				contains: []
			},
			'description': {
				single: true,
				name_is_description: true,
				contained_in: ['library', 'constant', 'class', 'attribute', 'function'],
				contains: []
			},
			'refer': {
				single: true,
				name_is_description: true,
				contained_in: ['library', 'constant', 'class', 'attribute', 'function'],
				contains: []
			},
		}

		var current;
		_.each(anns, function(a) {
			var value = a.value.trim(), name = value.split(' ')[0], 
				inline_description, dash = ' - ', index_of_dash = value.indexOf(dash);
			if(index_of_dash!=-1)
				inline_description = value.substr(index_of_dash+dash.length);
			var item = {
					type: a.name, 
					name: name,
					description: inline_description,
					line_at: a.line_at,				
					file: f					
			};	
			var annotation = annotations[item.type];		
			if(!annotation) {
				error_exit('annotation @'+item.type+' is invalid', item.line_at);
			}
			if(annotation.name_is_description) {
				item.description = value;
				item.name = undefined;
			}

			function insert_annotation() {
				if(!current.contents) 
					current.contents = {};				
				if(annotation.single) {					
					if(current.contents[item.type])
						error_exit('only single @'+item.type+' allowed', item.line_at);					
					current.contents[item.type] = item;
					current.contents[item.type].type = undefined;
				} else {
					if(!current.contents[item.type])
						current.contents[item.type] = [];
					current.contents[item.type].push(item);
					current.contents[item.type][current.contents[item.type].length-1].type = undefined;
					// i know this is ugly, but it's the simpliest
				}
			}
			if(annotation.contains.length  > 0) {
				current = result;							
				insert_annotation();
				current = item;
			} else if(annotation.contained_in.length > 0) {
				if(!current) {
					error_exit('no context for @'+item.type, item.line_at);
				}
				insert_annotation();
			}			
		});
		
		return result;
	}

	self.parse_file_abs = function(abspath) {
		var file_content = fs.readFile(abspath), result = [];
		if (!file_content)
            throw new Error("Unable to read: " + absolute_path);
        var str = file_content.string();
        var comments_re = /(\/\*([^]*?)\*\/|\/\/(.*))/gi, comments_match;

        while((comments_match = comments_re.exec(str))!=null) {
        	var line = str.substr(0,comments_re.lastIndex).split('\n').length;	// UGLY	        	      
	        var c = comments_match[0];
        	var annotation_re = /\@(\w+)(.*)/gi;
        	var annotation, annotations = [];
        	while((annotation = annotation_re.exec(c))!=null) {        		        		
        		result.push({        		
	        		line_at: line-c.split('\n').length+1,
	        		line_after: line+1,	        		
	        		name: annotation[1], 
	        		value: annotation[2]
        		});
        	}        	
    	}

        
        return result;
	}	
	self.parse_file = function(relative_path) {
		log.info("processing " + relative_path);
        var absolute_path = root_path + relative_path;

        var anns = self.parse_file_abs(absolute_path);
        // log.info(anns);
        return self.process_annotations_file(anns, relative_path);
	}	
	self.parse = function(relative_dir, filter, recursive) {				
		function endsWith(s, suffix) {
  		  return s.indexOf(suffix, s.length - suffix.length) !== -1;
		};

		var dir = root_path+relative_dir;
		var parsed_files = [];
		function enumerate(_dir) {
			fs.enumerateDirectory(_dir, function(file) {
				if(fs.isDirectory(file) && recursive) {
					enumerate(file);					
				}
				else {											
					var ext;			
					// TODO: needs a Set instead of a List and a loop
					filter.every(function(suffix) {
						if(endsWith(file.toString(), '.'+suffix))
							ext = suffix;
						else return true;
					});
					if(ext) {						
						var relative_path = file.toString().substr(root_path.length);												
						var ann = self.parse_file(relative_path);						
						parsed_files.push(ann);
					}
				}
			});
		}
		enumerate(dir);
		log.info(parsed_files);
		self.process_annotations(parsed_files);
	}
};

var docparse = new Docparse();
docparse.parse('/tests/doc-test', ['js', 'cpp', 'h'], true);