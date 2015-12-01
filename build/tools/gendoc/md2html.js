//
// Copyright (c) 2015 ASPECTRON Inc.
// All Rights Reserved.
//
// This file is part of Gendoc (https://github.com/aspectron/gendoc) project.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE
//
// Generate HTML document from Markdown

var util = require('util');
var fs = require('fs');

var marked = require('./marked');
var hljs = require('./highlight.js');

module.exports = md2html;

function md2html(input, output, template, title)
{
	var input_path = fs.Path(input).parentPath();
	//var filename = fs.Path(input).filename();

	input = fs.readFile(input).toString();
	if (!input.length)
	{
		throw new Error('Empty input');
	}

	template = fs.readFile(template).toString();
	if (!template.length)
	{
		throw new Error('Empty template', template);
	}

	// process @include directives
	input = process_includes(input, input_path);

	// parse input Markdown
	var lexed = marked.lexer(input);

	// get section name before tranformations
	var section = first_heading(lexed);

	// transform parsed Markdown
	//lexed = transform_lists(lexed);
	lexed = transform_links(lexed);

	// generate table of contents, tranform Markdonw
	var toc = generate_toc(lexed);

	// use custom renderer for headings
	var renderer = new marked.Renderer();
	renderer.heading = function(text, level)
	{
		var id = generate_id(text);
		return util.format('<h%d id="%s">%s</h%d>', level, id, text, level);
	};

	// render parsed Markdown to HTML
	var options = 
	{
		renderer : renderer,
		highlight : function(code) { return hljs.highlightAuto(code).value; }
	};
	var content = marked.parser(lexed, options);

	// replace placeholders in HTML template
	template = template.replace(/__SECTION__/g, section);
	template = template.replace(/__TITLE__/g, title || '');
	template = template.replace(/__TOC__/g, toc);
	template = template.replace(/__CONTENT__/g, content);

	fs.writeFile(output, template);
}

function process_includes(input, input_path)
{
	var include_regexp = /^@include\s+([A-Za-z0-9-_]+)(?:\.)?([a-zA-Z]*)$/gmi;

	var include_data = {};

	return input.replace(include_regexp, function(include)
		{
			if (!include_data.hasOwnProperty(include))
			{
				var filename = include.replace(/^@include\s+/, '');
				if (!fs.path(filename).extension()) filename = filename + '.md';
				filename = input_path + '/' + filename;

				include_data[include] = fs.readFile(filename).toString();
			}
			return include_data[include];
		});
}

function first_heading(lexed)
{
	for (var i = 0, len = lexed.length; i < len; ++i)
	{
    	var token = lexed[i];
    	if (token.type === 'heading') return token.text;
  	}
  	return '';
}

function transform_lists(lexed)
{
	var state = null;
	var depth = 0;

	var output = [];
	output.links = lexed.links;

	lexed.forEach(function(token)
	{
		switch (state)
		{
		case null:
			if (token.type === 'heading')
			{
				state = 'AFTERHEADING';
			}
			output.push(token);
			return;
		case 'AFTERHEADING':
			if (token.type === 'list_start')
			{
				state = 'LIST';
				if (depth === 0)
				{
          			output.push({ type:'html', text: '<div class="signature">' });
        		}
        		++depth;
        		output.push(token);
        		return;
      		}
      		state = null;
      		output.push(token);
      		return;
		case 'LIST':
			switch (token.type)
			{
			case 'list_start':
				++depth;
        		output.push(token);
        		return;
			case 'list_end':
				--depth;
				if (depth === 0)
				{
					state = null;
					output.push({ type:'html', text: '</div>' });
				}
				output.push(token);
				return;
			default:
	      		if (token.text)
	      		{
	        		token.text = token.text.replace(/\{([^\}]+)\}/, '<span class="type">$1</span>');
	      		}
				output.push(token);
				return;
			}
			return;
		}
	});

	return output;
}

function transform_links(lexed)
{
	var link_regexp = /(\[.*?\])\((.*?)(#.*?)?(\s+".*?")?\)/gm;

	lexed.forEach(function(token)
	{
		if (token.type === 'text' || token.type == 'paragraph')
		{
			token.text = token.text.replace(link_regexp, function(str, text, link, id, title)
			{
				if (link || id)
				{
					str = text;
					link = link.replace(/\.md$/, '.html');
					if (link.lastIndexOf('.') === -1)
					{
						link += '.html';
					}
					
					str += '(' + link;
					if (id)
					{
						str += id;
					}
					if (title)
					{
						str += title;
					}
					str += ')';
				}
				return str;
			});
		}
	});
	return lexed;
}

function generate_toc(lexed)
{
	var toc = [];
	var depth = 0;

	lexed.forEach(function(token)
	{
		if (token.type !== 'heading') return;
		if (token.depth > depth + 1)
		{
			throw new Error('Inappropriate heading level\n' + JSON.stringify(token));
		}

		depth = token.depth;
		var id = generate_id(token.text);
		var ident = new Array((token.depth - 1) * 2 + 1).join(' ');

		toc.push(ident + util.format('* <a href="#%s">%s</a>', id, token.text));
	});

	return marked.parse(toc.join('\n'));
}

function generate_id(text)
{
	text = text.trim();
	text = text.toLowerCase();
	text = text.replace(/\(.*\)/, '');
	text = text.replace(/[^a-z0-9_.]+/g, '_');
	return text;
}
