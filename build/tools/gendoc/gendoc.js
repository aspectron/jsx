//
// Copyright (c) 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of Gendoc (https://github.com/aspectron/gendoc) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
/**
@module gendoc - Yet Another Documentation Generator

Generate documentation from source code comments in Markdown format.

Source code documentation should be placed into `/ ** ** /`
comment block. Comment block should be opened and closed on a separate line.

Documentation text may be formatted in Markdown.
Supported tags:
<pre>
  @module name [- Brief description]
  Module description. A separate Markdown document would be created for each
  module with name `module.md`.

  @class name  [- Brief]
  Long class description.

  @function functionName(param1, [param2], [param3=default_value])  [- Brief]
  @param param1 [- Parameter descripton]
  @param [param2] {Type} [- Optional parameter description with type]
  @param [param3=default_value] 
  @param args*  - Argument list args1 .. argN
  @param [args]* - Optional argument list
  @return {Type} - function return descripton

  @property qulified.name {Type} [- Brief]
  Property description. Name may be qualified in scope of module.
</pre>

JavaDoc autobrief is working - if no inline brief with tag, the first sentence
from long description would be used.

For @class and @function additionaly @emit tags may be supplied:

<pre>
  @event eventName(param1, ... paramN) - Describe event, similar to function
  @param param1 {Type} - Parameter description
  @param paramN
</pre>

For @function tag @emit tags describe which events are fired from that function
 
<pre>
  @emit eventNameX
  @emit eventNameY
</pre>
**/

var util = require('util');
var fs = require('fs');
var markdownToHTML = require('./md2html.js');

module.exports = gendoc;
var SCRIPT_PATH;

if (typeof rt === 'undefined' && typeof process !== 'undefined')
{
	node_path = require('path');
	SCRIPT_PATH = node_path.dirname(process.argv[1]);
	fs.Path = function(path)
	{
		self = this;
		self.path = node_path.normalize(path + "");
		self.toString = function() { return self.path; }
		self.empty = function() { return !self.path; }
		self.parentPath = function() { return node_path.dirname(self.path); }
		self.filename = function() { return node_path.basename(self.path); }
		self.extension = function() { return node_path.extname(self.path); }
		self.stem = function() { return node_path.basename(self.path, self.extension()); }
		return this;
	}
	fs.createDirectory = function(path, recurse)
	{
		if (!recurse)
		{
			fs.mkdirSync(path);
			return;
		}
		else
		{
			var mode = 0777 & (~process.umask());
			fs.mkdir(path, mode, function(error)
			{
				if (error)
				{
					if (error.errno === 34)
					{
						//Create all the parents recursively
						fs.createDirectory(node_path.dirname(path), true);
						//And then the directory
						fs.createDirectory(path, false);
					}
					else throw error;
				}
			});
		}
	}
	fs.removeDirectory = function(path)
	{
		if (fs.existsSync(path))
		{
			fs.enumerateDirectory(path, function(name)
			{
				fs.isDirectory(name)? fs.removeDirectory(name) : fs.unlinkSync(name);
			});
			fs.rmdirSync(path);
		}
	}
	fs.enumerateDirectory = function(path, callback)
	{
		var files = fs.readdirSync(path);
		files.forEach(function(name)
		{ 
			callback(node_path.resolve(path, name));
		});
	}
	fs.isDirectory = function(path) { try { return fs.lstatSync(path).isDirectory(); } catch(e) { return false; } }
	fs.isRegularFile = function(path) { try { return fs.lstatSync(path).isFile(); } catch(e) { return false; } }
	fs.writeFile = fs.writeFileSync;
	fs.readFile = fs.readFileSync;
	fs.readLines = function(filename) { return fs.readFileSync(filename).toString().split('\n'); }
	fs.copy = function(src, dst) { fs.writeFileSync(dst, fs.readFileSync(src).toString()); }
}
else
{
	SCRIPT_PATH = rt.local.scriptPath;
	fs.removeDirectory = function(path) { fs.remove(path, true); }
}

/**
@function gendoc(config)
@param config {Object}
@return {Object} Generated documentation object

Generate documentation from source code in Markdown format.

Configuration object `config` should have following options:
  * `input` - array of input directories with source code to scan
  * `output` - result output directory name, would be created on necessary
  * `[cleanup=false]` - cleanup output directory before start
  * `[reporter=console.log]` - function to report work progress
  * `[extension='.md']` - result documents extension
  * `[result]` - write result in JSON format into the output directory, if supplied
  * `[single]` - additionally generate whole documentation as a single document in the specified filename
  * `[index]` - additionally generate index document in the specified filename
  * `[htmlOutput]` - convert generated Markdown to HTML pages in specified output directory
  * `[htmlTitle]` - HTML page title appended to each page
  * `[htmlExtension='.html']` - extension for output HTML files
  * `[htmlTemplate='template.html']` - HTML template filename
  * `[htmlStyle='style.css']` - HTML styles heet filename
  * `[htmlHighliteStyle='highlight.js/styles/default.css']` - HTML style sheet from highlight.js

Input directory may be a String or a RegEx object. For recurse directory scanning
specify `*` as a last but one path element. Examples:

```
    // all files in `dir1` subdirectory
    var input1 = './dir1';

    // files with .js extension in `dir2` subdirectory
    var input2 = './dir2/\\.+[.]js$';  
    
    // files with .hpp or .cpp extension in `dir3` and all its subdirectories
    var input3 = './dir3/* /\\.+[.][hc]pp$'; 

    gendoc({ output: './doc/out', input: [input1, input2, input3] });
```
**/
function gendoc(config)
{
	var globals = generateMarkdown(config);

	if (config.htmlOutput)
	{
		generateHTML(config, globals);
	}

	return globals;
}

function generateMarkdown(config)
{
	config.reporter = config.reporter || console.log;
	config.extension = config.extension || '.md';
	if (!config.output)
	{
		throw new Exception('require output directory');
	}

	if (!config.input || !config.input.length)
	{
		throw new Exception('require input directories');
	}

	if (config.cleanup)
	{
		fs.removeDirectory(config.output);
	}
	fs.createDirectory(config.output, true);

	var globals = {};

	// generate documentation from source files
	var input_files = enumerateInput(config.input);
	input_files.forEach(function(filename)
	{
		config.reporter('Scanning ' + filename);
		process_file(globals, filename);
	});

	if (config.result)
	{
		if (!fs.Path(config.result).extension())
		{
			config.result += '.json';
		}
		// write generated documentation in JSON
		var resultJson = util.inspect(globals, {depth: Infinity});
		fs.writeFile(config.output + '/' + config.result, resultJson);
	}

	var single = ''; //# Documentation\n\n';
	var index = '';//# Index\n\n';

	// generate Markdown documents, each for a module
	var modules = Object.keys(globals);
	modules.forEach(function(module)
	{
		var markdown_file = config.output + '/' + module + config.extension;
		config.reporter('Generating ' + markdown_file);

		var markdown = generate_markdown(globals, module, globals[module], 0);
		fs.writeFile(markdown_file, markdown);

		if (config.single)
		{
			single += '\n\n';
			single += markdown;
		}

		if (config.index)
		{
			index += '  * [' + (globals[module]._doclet_.brief || globals[module]._doclet_.name) + ']';
			index += '(' + globals[module]._doclet_.name + ')\n';
		}
	});

	if (config.single && single)
	{
		// write single-page documentation
		if (!fs.Path(config.single).extension())
		{
			config.single += config.extension;
		}
		fs.writeFile(config.output + '/' + config.single, single);
	}

	if (config.index && index)
	{
		// write index
		if (!fs.Path(config.index).extension())
		{
			config.index += config.extension;
		}
		fs.writeFile(config.output + '/' + config.index, index);
	}

	return globals;
}

function generateHTML(config, globals)
{
	config.reporter = config.reporter || console.log;
	config.extension = config.extension || '.md';
	config.htmlExtension = config.htmlExtension || '.html';
	config.htmlTemplate = config.htmlTemplate || SCRIPT_PATH + '/template.html';
	config.htmlStyle = config.htmlStyle || SCRIPT_PATH + '/style.css';
	config.htmlHighliteStyle = config.htmlHighliteStyle || SCRIPT_PATH + '/highlight.js/styles/default.css';

	if (!config.output)
	{
		throw new Exception('require output directory');
	}

	if (config.cleanup)
	{
		fs.removeDirectory(config.htmlOutput);
	}
	fs.createDirectory(config.htmlOutput, true);

	var md_files;
	if (globals)
	{
		var modules = Object.keys(globals);
		if (config.single) modules.push(config.single);
		if (config.index) modules.push(config.index);

		md_files = [];
		modules.forEach(function(module)
		{
			var md_file = config.output + '/' + module;
			if (!fs.Path(md_file).extension()) md_file += config.extension;
			if (fs.isRegularFile(md_file)) md_files.push(md_file);
		});
	}
	else
	{
		md_files = enumerateInput([config.output + '/.+' + config.extension.replace('.', '[.]') + '$']);
	}

	md_files.forEach(function(markdown)
	{
		var html = config.htmlOutput + '/' + fs.Path(markdown).stem() + config.htmlExtension;
		config.reporter('Generating ' + html);
		markdownToHTML(markdown, html, config.htmlTemplate, config.htmlTitle || '');
	});

	if (md_files.length)
	try
	{
		var assets = config.htmlOutput + '/assets';
		fs.createDirectory(assets, true);
		fs.copy(config.htmlStyle, assets + '/style.css');
		fs.copy(config.htmlHighliteStyle, assets + '/hlstyle.css');
	}
	catch(e)
	{
		// ignore copy errors
	}
}

function enumerateInput(inputs)
{
	var filenames = [];

	function enumerateDir(dirname, filter, recurse)
	{
		fs.enumerateDirectory(dirname, function(name)
		{
			if (fs.isDirectory(name))
			{
				if (recurse)
					enumerateDir(name, filter, recurse);
			}
			else if (!filter || filter.test(name))
			{
				filenames.push(name);
			}
		});
	}

	inputs.forEach(function(input)
	{
		var filter, recurse;

		if (typeof rt !== 'undefined')
		{
			if (rt.platform == 'windows') input = input.replace(/\\/g, '/');
		}
		else if (typeof process !== 'undefined')
		{
			if (process.platform == 'win32') input = input.replace(/\\/g, '/');
		}

		if (fs.isRegularFile(input))
		{
			filenames.push(input);
			return;
		}

		var pos = input.lastIndexOf('/');
		if (pos >= 0)
		{
			filter = new RegExp(input.substr(pos + 1));
			input = input.slice(0, pos);
		}

		recurse = input.endsWith('*');
		if (recurse) input = input.slice(0, -1);

		if (!fs.isDirectory(input))
		{
			throw new Error(input + ' is not a directory');
		}

		enumerateDir(input, filter, recurse);
	});

	return filenames;
}

function process_file(globals, filename)
{
	var lines = fs.readLines(filename);
	if (!lines || !lines.length)
	{
		return;
	}

	globals.docletScope = new Stack();

	var comment_ident = -1;
	lines.forEach(function(line, line_num)
	{
		if (comment_ident < 0)
		{
			comment_ident = line.indexOf('/**');
			if (comment_ident >= 0 && !line.trim(0).endsWith('/**'))
			{
				comment_ident = -1;
			}
		}
		else
		{
			line = line.trim(comment_ident);
			if (line.endsWith('*/'))
			{
				comment_ident = -1;
			}
			else
			{
				process_line(globals, filename, line, line_num + 1);
			}
		}
	});
	delete globals.docletScope;
}

function process_line(globals, file, line, line_num)
{
	function findMatch(line)
	{
		var tags =
		[
			/^@(module)\s+(\w+)(?:\s+-\s+)?\s*(.*)$/,
			/^@(class)\s+([\w.]+)(?:\s+-\s+)?\s*(.*)$/,
			/^@(function)\s+([\w.]+(\(.*?\))?)(?:\s+-\s+)?\s*(.*)?$/,
			/^@(param)\s+([\w\[\]=*]+)?(?:\s+\{([\w.|#]+)\})?(?:\s+-\s+)?\s*(.*)?$/,
			/^@(returns?)(?:\s+\{([\w.|#]+)\})?(?:\s+-\s+)?\s*(.*)?$/,
			/^@(property)\s+([\w.#]+)(?:\s+\{([\w.#]+)\})?(?:\s+-\s+)?\s*(.*)?$/,
			/^@(event)\s+([\w.]+(\(.*?\))?)(?:\s+-\s+)?\s*(.*)?$/,
			/^@(emits?)\s+([\w.#]+)?(?:\s+-\s+)?\s*(.*)?$/,
		];

		for (var i = 0; i < tags.length; ++i)
		{
			var match = line.match(tags[i]);
			if (match)
			{
				return match;
			}
		}
		return null;
	}

	function qualifiedName(tag, name)
	{
		if (tag === 'function' || tag === 'event') name = name.replace(/\(.*\)/, '');
		return name.split('.');
	}

	function addDoclet(container, tag, name, brief, type)
	{
		var orig_name = name;
		name = qualifiedName(tag, name).pop();

		if (container.hasOwnProperty(name))
		{
			throw new SyntaxError(util.format('duplicate %s %s', tag, name), file, line, line_num);
		}

		var doclet = { _doclet_: {} };
		doclet._doclet_.tag = tag;
		doclet._doclet_.name = name;
		if (orig_name !== name)
		{
			doclet._doclet_.orig_name = orig_name;
		}
		if (brief)
		{
			doclet._doclet_.brief = brief;
		}
		if (type)
		{
			doclet._doclet_.type = type;
		}

		container[name] = doclet;
		globals.docletScope.push(doclet);
		return doclet;
	}

	function makeScope(qualified_name)
	{
		var parent = globals.docletScope.items[0];
		if (!parent || parent._doclet_.name != qualified_name[0] || parent._doclet_.tag !== 'module')
		{
			new SyntaxError('invalad qualified name ' + qualified_name.join('.'), file, line, line_num);	
		}

		var newScope = new Stack();
		newScope.push(parent);
		for (var i = 0; i < qualified_name.length-1; ++i)
		{
			var name = qualified_name[i];
			if (!parent.hasOwnProperty(name))
			{
				new SyntaxError('invalad qualified name ' + qualified_name.join('.'), file, line, line_num);
			}
			parent = parent[name];
			newScope.push(parent);
		}
		return newScope;
	}

	function popDoclet(parent_tags, tag, name)
	{
		var qualified_name;

		if (name)
		{
			name = qualifiedName(tag, name);
			if (name.length == 1)
			{
				if (globals.docletScope.top()._doclet_.tag === 'property')
				{
					globals.docletScope.pop();
				}
			}
			else
			{
				globals.docletScope = makeScope(name);
			}
		}

		var parent = globals.docletScope.top();
		while (parent && parent_tags.indexOf(parent._doclet_.tag) === -1)
		{
			globals.docletScope.pop();
			parent = globals.docletScope.top();
		}

		if (!parent)
		{
			throw new SyntaxError(util.format('no encosing scope of %s for %s tag', parent_tags, tag), file, line, line_num);
		}

		return parent;
	}

	function addModule(name, brief)
	{
		var module = globals[name] || addDoclet(globals, 'module', name, brief);
		globals.docletScope = new Stack();
		globals.docletScope.push(module);
		return module;
	}

	function addClass(name, brief)
	{
		var parent = popDoclet(['module'], 'class', name);
		return addDoclet(parent, 'class', name, brief);
	}

	function addFunction(name, brief)
	{
		var parent = popDoclet(['property', 'class', 'module'], 'function', name);
		return addDoclet(parent, 'function', name, brief);
	}

	function addParam(name, type, brief)
	{
		var parent = popDoclet(['function', 'event'], 'param');
		parent._doclet_.params = parent._doclet_.params || {};
		return addDoclet(parent._doclet_.params, 'param', name, brief, type);
	}

	function addReturn(type, brief)
	{
		var parent = popDoclet(['function'], 'return');
		return addDoclet(parent._doclet_, 'return', 'return', brief, type);
	}

	function addProperty(name, type, brief)
	{
		var parent = popDoclet(['property', 'class', 'module'], 'property', name);
		return addDoclet(parent, 'property', name, brief, type);
	}

	function addEvent(name, brief)
	{
		var parent = popDoclet(['property', 'function', 'class', 'module'], 'event');
		parent._doclet_.events = parent._doclet_.events || {};
		return addDoclet(parent._doclet_.events, 'event', name, brief);
	}

	function addEmit(name)
	{
		var parent = popDoclet(['function', 'class', 'module'], 'emit');
		parent._doclet_.emits = parent._doclet_.emits || {};
		return addDoclet(parent._doclet_.emits, 'emit', name);
	}

	function addText(line)
	{
		var doclet = globals.docletScope.top();
		if (!doclet)
		{
			return;//throw new SyntaxError('text before tag', file, line, line_num);
		}

		var doc = doclet._doclet_;
		switch (doc.tag)
		{
		case 'param': case 'return': case 'emit':
			if (!line || !globals.docletScope.top(-1)._doclet_.brief)
			{
				var parent_tags = ['function'];
				if (doc.tag == 'param') parent_tags.push('event');
				doclet = popDoclet(parent_tags, doc.tag);
			}
			break;
		}

		doc = doclet._doclet_;
		if (!doc.brief)
		{
			var pos = line.indexOf('. ');
			if (pos === -1)
			{
				doc.brief = line;
				return;
			}
			if (pos > 0)
			{
				doc.brief = line.substr(0, pos);
				line = line.substr(pos + 2);
			}
		}

		if (doc.full)
		{
			doc.full += '\n' + line;
		}
		else
		{
			doc.full = line;
		}
	}

	var match = findMatch(line);
	if (match)
	{
		var tag = match[1], name = match[2], type, brief;
		if (!name)
		{
			throw new SyntaxError('No name for tag ' + tag, file, line, line_num);
		}

		switch (tag)
		{
		case 'module':
			brief = match[3];
			doclet = addModule(name, brief);
			break;
		case 'class':
			brief = match[3];
			doclet = addClass(name, brief);
			break; 
		case 'function':
			//match[3] == function arguments
			brief = match[4];
			addFunction(name, brief);
			break;
		case 'param':
			type = match[3];
			brief = match[4];
			doclet = addParam(name, type, brief);
			break;
		case 'returns':
			tag = 'return';
		case 'return':
			type = match[3];
			brief = match[4];
			doclet = addReturn(name, type, brief);
			break;
		case 'property':
			type = match[3];
			brief = match[4];
			doclet = addProperty(name, type, brief);
			break;
		case 'event':
			//match[3] == event arguments
			brief = match[4];
			doclet = addEvent(name, brief);
			break;
		case 'emits':
			tag = 'emit';
		case 'emit':
			doclet = addEmit(name);
			break;
		default:
			throw new SyntaxError('unknown tag ' + tag, file, line, line_num);
		}
	}
	else if (line.startsWith('@'))
	{
		throw new SyntaxError('unknown tag', file, line, line_num);
	}
	else
	{
		addText(line);
	}
}

function generate_markdown(globals, module, doclet, level)
{
	function generate_id(text)
	{
		text = text.trim();
		text = text.toLowerCase();
		text = text.replace(/\(.*\)/, '');
		text = text.replace(/[^a-z0-9_.]+/g, '_');
		return text;
	}

	function transform_links(text, is_type_spec)
	{
		var link_regexp = /([\w.]+)?#([\w.]+)?/gm;
		text = text.replace(link_regexp, function(str, link, id)
			{
				if (!link && !id) return str;
				if (id && id.endsWith('.')) id = id.slice(0, -1); // chomp last dot

				var is_extern_link = link && id;

				link = link || module;
				var doclet = globals[link];
				if (!doclet)
				{
					throw new Error('Unknown module ' + link + ' in link `' + str + '` when generating ' + module + '.md');
				}

				if (id)
				{
					var name = id.split('.');
					for (var i = 0; i < name.length; ++i)
					{
						doclet = doclet[name[i]];
						if (!doclet)
						{
							throw new Error('Unknown #' + id + ' in link `' + str + '` when generating ' + module + '.md');
						}
					}
				}

				doclet = doclet['_doclet_'];
				if (doclet)
				{
					str = '[';
					if (!is_type_spec) str += '`';
					if (is_extern_link && doclet.tag === 'function') str += link + '.';
					str += (doclet.orig_name && doclet.tag === 'property'? doclet.orig_name : doclet.name);
					if (!is_type_spec) str += '`';
					str += ']';

					str += '(' + link;
					if (id) str += '#' + generate_id(id);

					var title = '';
					if (doclet.orig_name) title += doclet.orig_name;
					if (doclet.brief)
					{
						if (title) title += ' - ';
						title += doclet.brief;
					}
					if (title)
					{
						str += ' "' + title + '"';
					}
					str += ')';
				}
				return str;
			});
		return text;
	}

	var markdown = '';

	if (doclet.hasOwnProperty('_doclet_'))
	{
		var doc = doclet._doclet_;

		if (doc.tag === 'param' || doc.tag === 'return'
			|| doc.tag === 'event' || doc.tag === 'emit')
		{
			markdown += new Array(level + 2).join(' ') + '* ';
			switch (doc.tag)
			{
			case 'return':
				markdown += 'Return:';
				break;
			case 'emit':
				markdown += transform_links(doc.orig_name || doc.name);
				break;
			default:
				markdown += (doc.orig_name || doc.name);
				break;
			}
			if (doc.type)
			{
				markdown += ' `' + transform_links(doc.type, true) + '`';
			}
			if (doc.brief)
			{
				markdown += ' ' + transform_links(doc.brief);
			}
			markdown += '\n';
			if (doc.params)
			{
				markdown += generate_markdown(globals, module, doc.params, level + 1);
			}
			return markdown;
		}

		// heading
		markdown += new Array(level+2).join('#') + ' ';
		markdown += (doc.tag === 'module'? doc.brief : doc.orig_name) || doc.name;
		markdown += '\n\n';

		switch (doc.tag)
		{
		case 'function':
			if (doc.params)
			{
				markdown += generate_markdown(globals, module, doc.params, 0);
			}
			if (doc.return)
			{
				markdown += generate_markdown(globals, module, doc.return, 1);
			}
			markdown += '\n';
			break;
		case 'property':
			if (doc.type)
			{
				markdown += ' * `' + transform_links(doc.type, true) + '`';
			}
			markdown += '\n\n';
			break;
		}

		if (doc.brief && doc.tag !== 'module')
		{
			markdown += transform_links(doc.brief);
			markdown += '\n\n';
		}

		if (doc.full)
		{
			markdown += transform_links(doc.full);
			markdown += '\n\n';
		}

		if (doc.events)
		{
			markdown += '__Events__:\n';
			markdown += generate_markdown(globals, module, doc.events, 1);
			markdown += '\n';
		}

		if (doc.emits)
		{
			markdown += '__Emits__:\n';
			markdown += generate_markdown(globals, module, doc.emits, 1);
			markdown += '\n';
		}
	}

	for (name in doclet)
	{
		if (doclet.hasOwnProperty(name) && name !== '_doclet_')
		{
			markdown += generate_markdown(globals, module, doclet[name], level + 1);
		}

	}
	return markdown;
}

/////////////////////////////////////////////////////////////////////
//
// Utilty functions
//
function Stack()
{
	this.items = [];
	return this;
}

Stack.prototype.top = function(offset)
{
	offset = (offset < 0? -offset : 0);
	return this.items[this.items.length-1 - offset];
}

Stack.prototype.push = function(item)
{
	this.items.push(item);
}

Stack.prototype.pop = function()
{
	return this.items.pop();
}

function SyntaxError(message, file, line, line_num)
{
	this.name = 'SyntaxError';
	this.message = util.format('%s at %s(%d): %s', message, file, line_num, line);
}
util.inherits(SyntaxError, Error);

// trim spaces in str with maxLeft and maxRight limits from left and right
String.prototype.trim = function(maxLeft, maxRight)
{
	maxLeft = Math.min(maxLeft === undefined? Infinity : maxLeft, this.length);
	maxRight = Math.min(maxRight === undefined? Infinity : maxRight, this.length);

	var spaces = ' \f\n\r\t\v\u00A0\u2028\u2029';

	var left = 0;
	while (left < maxLeft && spaces.indexOf(this.charAt(left)) >= 0)
	{
		++left;
	}

	var right = this.length-1;
	while (left <= right && maxRight > 0 && spaces.indexOf(this.charAt(right)) >= 0)
	{
		--right;
		--maxRight;
	}

	return this.slice(left, right + 1);
}

String.prototype.startsWith = function(prefix)
{
	return this.indexOf(prefix, 0) === 0;
};

String.prototype.endsWith = function(suffix)
{
	return this.indexOf(suffix, this.length - suffix.length) !== -1;
};

